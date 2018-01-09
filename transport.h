#ifndef _TRANSPORT_H
#define _TRANSPORT_H


#include "rw.h"
#include "basic.h"
#include "event.h"
#include "bit_array.h"
#include "queue.h"

#include <pthread.h>


#define MTU 			1500
#define UDPIP_HEADER 	28
#define SR_HEADER		(sizeof(uint8_t) + sizeof(uint16_t))
#define MSS 			(MTU - UDPIP_HEADER - SR_HEADER)
#define CBUF_SIZE 		(5 * MSS)
#define MAXSEQNUM		(1 << 8)


struct segment {
	uint8_t seqnum;
	uint16_t size;
	uint8_t payload[MSS];
};

struct packet {
	struct segment sgt;
	struct timespec sendtime;
	struct timespec exptime;
	bool rtx;
};

struct window {
	unsigned int base;
	unsigned int width;
	struct bit_array ack_bar;	// 128 bit array
};

struct circular_buffer {
    pthread_mutex_t mtx;
    pthread_cond_t cnd_not_empty;
    pthread_cond_t cnd_not_full;
    unsigned int S;
    unsigned int E;
    char buf[CBUF_SIZE];
};

struct shared_tools {
	int sockfd;
	struct circular_buffer *cb;
	struct event *e;
	struct proto_params *params;
};


void init_transport(int sockfd, struct proto_params *params);
void rdt_send(const void *buf, size_t len);
void rdt_recv(void *buf, size_t len);
ssize_t rdt_read_string(char *buf, size_t size);


#endif /* _TRANSPORT_H */
