#include <string>
#include <iostream>
#include "../src/imredis.h"
#include <hiredis/hiredis.h>
#include <syslog.h>

/* for debug */
/*
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
*/

void dumpReply(redisReply *reply)
{
	std::cout<<"type:"<<reply->type<<'\n'
		<<"integer:"<<reply->integer<<'\n'
		<<"len:"<<reply->len<<'\n';
	if (reply->len){
		std::cout<<"str:"<<reply->str<<'\n';
	}
	std::cout<<"elements:"<<reply->elements<<'\n';
	if (reply->elements){
		for (int i = 0; i < reply->elements; i++){
			dumpReply(reply->element[i]);
		}	
	}
	std::cout<<std::endl;
}


int main(int argc, char *argv[])
{
	Redis redis;
#if 0
	std::cout<<"test 'setnx testkeyvalue 123' >>> ";
	if (redis.SetNX("testkeyvalue", "123")){ 
		std::cout<<"OK"<<std::endl;
	} else {
		std::cout<<"error"<<std::endl;
	}

	/*
	std::cout<<redis.LPUSH("testlist", "helloworld")<<std::endl;
	std::cout<<redis.RPOP("testlist")<<std::endl;
	*/
#endif	
	std::cout<<redis.Subscribe("testchannel", dumpReply)<<std::endl;
	
	return 0;
}
