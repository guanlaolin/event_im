#include <hiredis/hiredis.h>
#include <syslog.h>
#include "im_redis.h"

Redis::Redis()
{
	Redis(IP, PORT);
}

Redis::Redis(const char *ip, int port)
{
	rc = redisConnect(ip, port);
	if (rc == NULL || rc->err) {
    	if (rc) {
        	syslog(LOG_ERR, "Error: %s, %d, %s\n"
				, __func__, __LINE__
				, rc->errstr);
			//未进行错误处理
			//throw 
    	}
    } 
	else {
        syslog(LOG_ERR, "Can't allocate redis context\n");
		//未进行错误处理
		//throw 
    }
}

Redis::~Redis()
{
	redisFree(rc);
}

bool Redis::SetNX()
{
}