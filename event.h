#ifndef _EVENT_H
#define _EVENT_H


#include <pthread.h>
#include <inttypes.h>

#define NO_EVENT	0
#define PKT_EVENT	1
#define ACK_EVENT	2


struct event {
	pthread_mutex_t mtx;
	pthread_cond_t cnd_event;
	pthread_cond_t cnd_no_event;
	unsigned int type;
	uint8_t acknum;
};

int cond_event_signal(struct event *e, unsigned int event_type);
int cond_ack_event_signal(struct event *e, uint8_t acknum);


#endif /* _EVENT_H */
