#include <iostream>
#include <string>
extern "C" {
	#include <stdio.h>
	#include <event2/event.h>
	#include <event2/listener.h>
	#include <event2/bufferevent.h>
	#include <event2/bufferevent_compat.h>
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <syslog.h>
	#include <string.h>
	#include <unistd.h>
	#include <pthread.h>
}
#include "imredis.h"
#include "immessage.pb.h"


#define PORT 8000
#define IP "0.0.0.0"
#define LOG_BUF_MAX_SIZE 1024
#define DATA_BUF_MAX_SIZE 1024
#define TOKEN_LEN 64

//redis key
const std::string REDIS_KEY_ORIGINAL_MSG	= "original-msg";
const std::string REDIS_CHANNEL_LOGIN_SEND	= "login-send";
const std::string REDIS_CHANNEL_LOGIN_RECV	= "login-recv";

enum msg_type{
	LOGIN = 1,
};

void listener_cb(struct evconnlistener *
	, evutil_socket_t
	, struct sockaddr *
	, int socklen, void *);

void read_cb(struct bufferevent *bev, void *ctx);
void write_cb(struct bufferevent *bev, void *ctx);
void event_cb(struct bufferevent *bev, short what
	, void *ctx);

void login_cb(redisReply *reply);
void* login_cb_proc(void *data);

int main(int argc, char *argv[])
{
	int sock	= 0;
	int pid		= 0;
	struct sockaddr_in 		addr;
	struct event_base 		*base = NULL;
	struct evconnlistener 	*listener = NULL;
	
	/* config */
	openlog(argv[0], LOG_CONS|LOG_PERROR|LOG_PID, LOG_USER);
	event_enable_debug_mode();

	pid = fork();
	if (pid < 0) {
		syslog(LOG_ERR, "%s, %d, errno[%d], fork error.\n"
			,__func__, __LINE__, errno);
		return -1;
	} 
	else if (0 == pid) {
		//child
		Redis redis;

		syslog(LOG_DEBUG, "creat child process success.\n");
	
		if (!redis.Subscribe(REDIS_CHANNEL_LOGIN_RECV, login_cb)){
			syslog(LOG_ERR, "%s, %d, errno[%d], subscribe error.\n"
			,__func__, __LINE__, errno);
			return -1;
		}
		return 0;
	}

	/* socket */
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (-1 == sock){
		syslog(LOG_ERR, "FUCTION:%s, LINE:%d, errno:%d.\n"
			,__func__, __LINE__, errno);
		goto out;
	}

	syslog(LOG_DEBUG, "server socket:%d.\n", sock);

	/* sockaddr */
	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	if (inet_pton(AF_INET, IP, &addr.sin_addr) != 1){
		syslog(LOG_ERR, "FUNCTION:%s, LINE:%d, errno:%d.\n"
			, __func__, __LINE__, errno);
		goto out;
	}
	addr.sin_port = htons(PORT);

	/* base */
	base = event_base_new();
	if (NULL == base){
		syslog(LOG_ERR, "FUNCTION:%s, LINE:%d.\n"
			, __func__, __LINE__);
		goto out;
	}

	/* listen */
	listener = evconnlistener_new_bind(base, listener_cb, base
		, LEV_OPT_REUSEABLE|LEV_OPT_CLOSE_ON_FREE, -1, (struct sockaddr *)&addr
		, sizeof(struct sockaddr_in));
	if (!listener){
		syslog(LOG_ERR, "FUNCTION:%s, LINE:%d, errno:%d.\n"
			, __func__, __LINE__, errno);
		goto out;
	}

	if (event_base_dispatch(base) == -1){
		syslog(LOG_ERR, "FUNCTION:%s, LINE:%d.\n"
				, __func__, __LINE__);
		goto out;
	}

	out:
		close(sock);
		event_base_free(base);
		evconnlistener_free(listener);
		closelog();
		return 0;
}

void listener_cb(struct evconnlistener *_listener
	, evutil_socket_t _fd
	, struct sockaddr *_addr
	, int socklen, void *_data)
{
	socklen_t len;
	struct sockaddr_in addr;
	struct event_base *base;
	struct bufferevent *buffer;

	syslog(LOG_DEBUG, "client fd:%d\n", _fd);

	len = sizeof(struct sockaddr_in);
	if (getpeername(_fd, (struct sockaddr *)&addr, &len) == -1)
	{
		syslog(LOG_INFO, "FUNCTION:%s, LINE:%d, errno:%d\n"
			, __func__, __LINE__, errno);
		goto out;
	}

	syslog(LOG_DEBUG, "client connected:ip[%s], port[%d]\n"
		, inet_ntoa(addr.sin_addr)
		, ntohs(addr.sin_port));

	base = (struct event_base *)_data;

	buffer = bufferevent_socket_new(base, _fd, BEV_OPT_CLOSE_ON_FREE);
	if (!buffer){
		syslog(LOG_INFO, "FUNCTION:%s, LINE:%d\n"
			, __func__, __LINE__);
		goto out;
	}

	bufferevent_setcb(buffer, read_cb, write_cb, event_cb, base);
	if (bufferevent_enable(buffer, EV_READ | EV_WRITE) == -1){
		syslog(LOG_INFO, "FUNCTION:%s, LIEV_READNE:%d\n"
			, __func__, __LINE__);
		goto out;
	}

out:
	//close(_fd); /* why remove? 应该在哪里释放？*/
	return;
}

void read_cb(struct bufferevent *bev, void *ctx)
{
	int fd = 0;							//客户端socket
	char data[DATA_BUF_MAX_SIZE] = {0};	//消息缓冲区
	improto::IMProto imp;			//
	Redis redis;

	/* 有问题，万一读不完呢？ */
	bufferevent_read(bev, data, sizeof(data));

	if (!imp.ParseFromString(data)) {
		syslog(LOG_INFO, "%s, %d, parse to improto error.\n"
			, __func__, __LINE__);
		goto out;
	}

	/* 目前单独仅处理登录消息 */
	switch (imp.type())
	{
		case improto::LOGIN:
			fd = bufferevent_getfd(bev);
			// size of array only test
			char arr_fd[10];
			sprintf(arr_fd, "%d", fd);
			
			if (!redis.SetNX(arr_fd, data)) {
				syslog(LOG_WARNING, "%s, %d, key[%s], fd[%d], SETNX error, maybe already exist.\n"
					, __func__, __LINE__, arr_fd, fd);
				goto out;
			}
			
			//通过发布-订阅通知登录处理端
			if (redis.Publish(REDIS_CHANNEL_LOGIN_SEND, arr_fd) < 0){
				syslog(LOG_WARNING, "%s, %d, channel[%s], fd[%d], PUBLISH error.\n"
					, __func__, __LINE__, REDIS_CHANNEL_LOGIN_SEND.c_str(), fd);
				goto out;
			}
			break;

		default:
			if (!redis.LPUSH(REDIS_KEY_ORIGINAL_MSG, data)) {
				syslog(LOG_WARNING, "%s, %d, LPUSH error.\n"
					, __func__, __LINE__);
				goto out;
			}
	}

	out:
		return;
}

void write_cb(struct bufferevent *bev, void *ctx)
{
	printf("write\n");
}

void event_cb(struct bufferevent *bev, short what, void *ctx)
{
	syslog(LOG_INFO, "FUNCTION:%s, LINE:%d, client connection error.\n"
			, __func__, __LINE__);
	
	bufferevent_free(bev);
}

void login_cb(redisReply *reply)
{
	int ret = 0;
	pthread_t *pid = NULL;
	ret = pthread_create(pid, NULL, login_cb_proc, (void *)reply);
	if (ret != 0){
		syslog(LOG_WARNING, "FUNCTION:[%s], LINE:[%d], return[%d], create thread error.\n"
			, __func__, __LINE__, ret);
		goto out;
	}
		
	out:
		return;
}

void* login_cb_proc(void *_data)
{
	int fd = 0;
	std::string token;
	std::string data;
	Redis redis;
	redisReply *reply;
	improto::IMProto imp;

	reply = (redisReply *)_data;

	if (reply->type != REDIS_REPLY_ARRAY
		|| reply->elements <= 0)
	{
		syslog(LOG_WARNING, "%s, %d, type[%d], size of array[%lu], \
			subscript message error\n"
			, __func__, __LINE__, reply->type, reply->elements);
		goto out;
	}

	fd = atoi(reply->element[2]->str);
	if (0 == fd) {
		syslog(LOG_INFO, "%s, %d, get fd error, raw value[%s]\n"
		, __func__, __LINE__, reply->element[2]->str);
		return (void*)-1;
	}

	token = redis.Get(reply->element[2]->str);

	if ("" == token){
		syslog(LOG_INFO, "%s, %d, get null or error\n"
			, __func__, __LINE__);
		imp.set_token("");
	}

	imp.set_token(token);
	if (!imp.SerializeToString(&data)){
		syslog(LOG_WARNING, "%s, %d, serialize object error, just trust to luck\n"
			, __func__, __LINE__);
		goto out;
	}

	if (send(fd, (void *)data.data(), data.length(), 0) == -1) {
		syslog(LOG_WARNING, "%s, %d, send to client error, errno[%d], just trust to luck\n"
			, __func__, __LINE__, errno);
		goto out;
	}

	out:
		return NULL;
}
