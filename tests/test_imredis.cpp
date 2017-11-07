#include <string>
#include <iostream>
#include "../imredis.h"

int main()
{
	Redis redis;

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

	//std::cout<<redis.Publish("testchannel", "helloworld")<<std::endl;

	//std::cout<<redis.Subscribe("testchannel")<<std::endl;

	return 0;
}
