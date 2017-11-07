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
}
#include "imredis.h"
#include "immessage.pb.h"


#define PORT 8000
#define IP "0.0.0.0"
#define LOG_BUF_MAX_SIZE 1024
#define DATA_BUF_MAX_SIZE 1024

//redis key
const std::string REDIS_KEY_ORIGINAL_MSG = "originalmsg";

enum msg_type{
	LOGIN = 1,
};

void listener_cb(struct evconnlistener *
	, evutil_socket_t
	, struct sockaddr *
	, int socklen, void *);

void read_cb(struct bufferevent *bev, void *ctx);
void write_cb(struct bufferevent *bev, void *ctx);
void event_cb(struct bufferevent *bev, short what, void *ctx);

#if 0
/* 有问题，要是别的类型呢？ */
char* logf(char *msg)
{
	char buf[LOG_BUF_MAX_SIZE];
	sprintf(buf, "FUCTION[%s], LINE[%d]:%s.\n"
		, __func__
		, __LINE__
		, msg);
	
	return buf;
}
#endif

int main(int argc, char *argv[])
{
	int sock = 0;
	struct sockaddr_in 		addr;
	struct event_base 		*base = NULL;
	struct evconnlistener 	*listener = NULL;
	
	/* config */
	openlog(argv[0], LOG_CONS|LOG_PERROR|LOG_PID, LOG_USER);
	event_enable_debug_mode();

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
	int fd;							//客户端socket
	char data[DATA_BUF_MAX_SIZE];	//消息缓冲区
	improto::IMProto imp;			//
	Redis redis;

	/* 有问题，万一读不完呢？ */
	bufferevent_read(bev, data, sizeof(data));

	if (!imp.ParseFromString(data)) {
		syslog(LOG_INFO, "%s, %d, Parse to improto error.\n"
			, __func__, __LINE__);
		goto out;
	}

	switch (imp.type())
	{
		case improto::LOGIN:
			fd = bufferevent_getfd(bev);
			if (!redis.SetNX(""+fd, data)) {
				syslog(LOG_WARNING, "%s, %d, fd[%d], SETNX error, maybe already exist.\n"
					, __func__, __LINE__, fd);
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

