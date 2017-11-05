#include <hiredis/hiredis.h>

#define IP	 "0.0.0.0"
#define PORT 6379

class Redis
{
	public:
		Redis();
		Redis(const char *ip, int port);
		~Redis();

		bool SetNX();
		
	protected:
		
	private:
		redisContext *rc;
};

