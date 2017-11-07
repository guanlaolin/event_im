#ifndef __EV_IM_CLIENT__
#define __EV_IM_CLIENT__

struct client
{
	int fd;
	unsigned int	ip;
	unsigned short	port;
	int time;	//最后一次活动时间
};

#endif
