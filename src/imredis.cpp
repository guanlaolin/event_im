#include <string>
#include <iostream>

extern "C" {
#include <hiredis/hiredis.h>
#include <syslog.h>
}

#include "imredis.h"

/* for debug */
void dumpReply(redisReply *reply, std::string prefix)
{
	std::cout<<prefix<<'\n'
		<<"type:"<<reply->type<<'\n'
		<<"integer:"<<reply->integer<<'\n'
		<<"len:"<<reply->len<<'\n';
	if (reply->len){
		std::cout<<"str:"<<reply->str<<'\n';
	}
	std::cout<<"elements:"<<reply->elements<<'\n';
	if (reply->elements){
		for (int i = 0; i < reply->elements; i++){
			dumpReply(reply->element[i], prefix+" child");
		}	
	}
	std::cout<<std::endl;
}

Redis::Redis()
{
	new(this) Redis(REDIS_SERVER, REDIS_PORT);
}

Redis::Redis(const std::string ip, const int port)
{
	rc = redisConnect(ip.c_str(), port);
	if (NULL == rc || rc->err) {
    	if (rc) {
        	syslog(LOG_ERR, "Error: %s, %d, %s\n"
				, __func__, __LINE__
				, rc->errstr);
			//未进行错误处理
			//throw 
    	} 
		else {
	        syslog(LOG_ERR, "Can't allocate redis context\n");
			//未进行错误处理
			//throw 
	    }
	}
}

Redis::~Redis()
{
	redisFree(rc);
}

bool Redis::SetNX(const std::string key, const std::string value)
{
	redisReply *reply = (redisReply *)redisCommand(this->rc
		, "SETNX %s %s", key.c_str(), value.c_str());
	//SETNX命令返回integer
	if (REDIS_REPLY_INTEGER != reply->type) {
		syslog(LOG_ERR, "Reply error: %s, %d, %s\n"
			, __func__, __LINE__
			, reply->str);
		return false;
	}

	return reply->integer;
}

bool Redis::LPUSH(const std :: string key, const std :: string value)
{
	redisReply *reply = (redisReply *)redisCommand(this->rc
		, "LPUSH %s %s", key.c_str(), value.c_str());
	//LPUSH命令返回integer
	if (REDIS_REPLY_INTEGER != reply->type) {
		syslog(LOG_ERR, "Reply error: func[%s], line[%d], replytype[%d], %s\n"
			, __func__, __LINE__, reply->type
			, reply->str);
		return false;
	}

	return reply->integer;
}

std::string 
Redis::RPOP(const std::string key)
{
	redisReply *reply = (redisReply *)redisCommand(this->rc
		, "RPOP %s", key.c_str());
	if (REDIS_REPLY_STRING != reply->type) {
		syslog(LOG_ERR, "Reply error: func[%s], line[%d], replytype[%d], %s\n"
			, __func__, __LINE__, reply->type
			, reply->str);
		return "";
	}

	return reply->str;
}

int Redis::Publish(const std::string channel, const std::string value)
{
	redisReply *reply = (redisReply *)redisCommand(this->rc
		, "PUBLISH %s %s", channel.c_str(), value.c_str());
	if (REDIS_REPLY_INTEGER != reply->type) {
		syslog(LOG_ERR, "Reply error: func[%s], line[%d], replytype[%d], %s\n"
			, __func__, __LINE__, reply->type
			, reply->str);
		return -1;
	}
	return reply->integer;
}

/* inelegant code */
bool Redis::Subscribe(const std::string channel, SUB_CB scb){
	redisReply *reply = (redisReply *)redisCommand(rc
		, "SUBSCRIBE %s", channel.c_str());

	freeReplyObject(reply);

	while (redisGetReply(rc, (void **)&reply) == REDIS_OK) {
		scb(reply);
		freeReplyObject(reply);
	}
	
	return false;
}

