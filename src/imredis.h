#include <string>
#include <iostream>

extern "C" {
#include <hiredis/hiredis.h>
}

const std::string REDIS_SERVER = "127.0.0.1";
const int REDIS_PORT = 6379;

typedef void (*SUB_CB)(redisReply *);

class Redis
{
	public:
		Redis();
		Redis(const std::string ip, const int port);
		~Redis();

		//key-value
		bool SetNX(const std::string key, const std::string value);

		//list
		bool LPUSH(const std::string key, const std::string value);
		std::string RPOP(const std::string key);

		//publish-subscribe
		int Publish(const std::string channel, const std::string value);
		//problem code
		bool Subscribe(const std::string channel, SUB_CB scb);
		
	protected:
		
	private:
		redisContext *rc;
};