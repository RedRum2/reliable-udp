#include "transport.h"
#include "simul_udt.h"
#include "window.h"
#include "queue.h"
#include "adaptive.h"
#include "cb_utils.h"
#include "timespec_utils.h"



/* shared structures */
struct circular_buffer recv_cb;
struct circular_buffer send_cb;
struct event e;

/* threads args to keep alive */
struct shared_tools recv_tools, send_tools;





int min(int x, int y)
{
    return x <= y ? x : y;
}


/*
 * Function:	rdt_send
 * ----------------------------------------------------------
 * Put data into the shared sending circular buffer, checking how
 * much space is available, and put MSS multiples each time, in order
 * to let the sender service to create as full as possible packets.
 * If at least MSS bytes are not available, wait until there is enough
 * free space.
 *
 * Parameters:
 * 		buf:	the address of the buffer containing data to send
 * 		len:	the size of the buffer buf
 */
void rdt_send(const void *buf, size_t len)
{
    size_t free, tosend, left = len;

    while (left) {

        if (pthread_mutex_lock(&send_cb.mtx) != 0)
            handle_error("pthread_mutex_lock");

        /* check available space */
        while ((free =
                space_available(send_cb.S, send_cb.E, CBUF_SIZE)) <= MSS)
            if (pthread_cond_wait(&send_cb.cnd_not_full, &send_cb.mtx) !=
                0)
                handle_error("pthread_cond_wait");

        /* calculate how much data to send */
        tosend = left < MSS ? left : min(left / MSS, free / MSS) * MSS;

        memcpy_tocb(send_cb.buf, buf + len - left, tosend, send_cb.E,
                    CBUF_SIZE);

        send_cb.E = (send_cb.E + tosend) % CBUF_SIZE;

        if (pthread_mutex_unlock(&send_cb.mtx) != 0)
            handle_error("pthread_mutex_unlock");

        if (cond_event_signal(&e, PKT_EVENT) == -1)
            handle_error("cond_event_signal()");

        left -= tosend;
    }
}




/*
 * Function:	rdt_recv	
 * ----------------------------------------------------------
 * Empty the circular buffer and put as much data as possibile into 
 * the buffer buf until to put exactly len bytes. 
 * If the circular buffer is empty, wait until any data is available.
 *
 * Parameters:
 * 		buf:	the address of the buffer wherein put data
 * 		len:	the number of bytes to draw from the buffer
 */
void rdt_recv(void *buf, size_t len)
{
    size_t data, toread, left = len;

    while (left) {

        if (pthread_mutex_lock(&recv_cb.mtx) != 0)
            handle_error("pthread_mutex_lock");

        while (recv_cb.S == recv_cb.E)
            /* circular buffer is empty */
            if (pthread_cond_wait(&recv_cb.cnd_not_empty, &recv_cb.mtx) !=
                0)
                handle_error("pthread_cond_wait");

        /* circular buffer not empty */
        data = data_available(recv_cb.S, recv_cb.E, CBUF_SIZE);
        toread = data < left ? data : left;
        memcpy_fromcb(buf + len - left, recv_cb.buf, toread, recv_cb.S,
                      CBUF_SIZE);
        recv_cb.S = (recv_cb.S + toread) % CBUF_SIZE;

        if (pthread_cond_signal(&recv_cb.cnd_not_full) != 0)
            handle_error("pthread_cond_signal");

        if (pthread_mutex_unlock(&recv_cb.mtx) != 0)
            handle_error("pthread_mutex_unlock");

        left -= toread;
    }
}




/*
 * Function:	rdt_read_string
 * --------------------------------------------------------
 * reads characters from the reliable socket until finds 
 * terminating null byte.
 *
 * Parameters:
 * 		buf:	the char buffer to store the string
 * 		maxlen:	the maximum number of characters to be read
 *
 * Returns:
 * 		the number of read characters
 * 		0 if nothing was read
 * 		-1 on error
 */
ssize_t rdt_read_string(char *buf, size_t maxlen)
{
    unsigned int i;
    char *p = buf;

    for (i = 0; i < maxlen; i++) {

        if (pthread_mutex_lock(&recv_cb.mtx) != 0)
            handle_error("pthread_mutex_lock");

        while (recv_cb.S == recv_cb.E)
            /* circular buffer empty */
            if (pthread_cond_wait(&recv_cb.cnd_not_empty, &recv_cb.mtx) !=
                0)
                handle_error("pthread_cond_wait");

        *p = recv_cb.buf[recv_cb.S];
        recv_cb.S = (recv_cb.S + 1) % CBUF_SIZE;

        if (pthread_cond_signal(&recv_cb.cnd_not_full) != 0)
            handle_error("pthread_cond_signal");

        if (pthread_mutex_unlock(&recv_cb.mtx) != 0)
            handle_error("pthread_mutex_unlock");

        if (*p == '\0')
            break;

        p++;
    }

    return i;
}




/*
 * Function:	store_pkt
 * ------------------------------------------------------------
 * Store segment and all the related info into the 
 * send_service's local buffer.
 * Take the segment from the first position of the shared 
 * circular buffer.
 *
 * Parameters
 * 		base	the address of the local buffer
 * 		seqnum	the segment's sequence number
 * 		size	the size of the segment's payload
 * 		cb		the address of the circular buffer
 */
void store_pkt(struct packet *base, uint8_t seqnum, size_t size,
               struct circular_buffer *cb)
{
    struct packet *pkt = base + seqnum;
    struct segment *sgt = &(pkt->sgt);

    sgt->seqnum = seqnum;
    sgt->size = size;
    memcpy_fromcb(sgt->payload, cb->buf, size, cb->S, CBUF_SIZE);

    pkt->rtx = false;
}




/*
 * Function:	empty_buffer
 * ------------------------------------------------
 * Remove application data from the circular buffer,
 * make packets and store them into a local buffer.
 * Do this until the circular buffer is not empty and the
 * local buffer has enough free space to store packets.
 *
 * Parameters:
 * 		cb				buffer containing application data
 * 		pkts			local buffer containing stored packets
 * 		w				window taking track of in-flight packets
 * 		last_seqnum		index of the next packet to store
 */
void empty_buffer(struct circular_buffer *cb, struct packet *pkts,
                  struct window *w, unsigned int *last_seqnum)
{
    size_t data, size;

    if (pthread_mutex_lock(&cb->mtx) != 0)
        handle_error("pthread_mutex_lock");

    while (cb->S != cb->E && 
			cbuf_free(w->base, *last_seqnum, MAXSEQNUM)) {
        // shared buffer not empty and local buffer has free slots

        data = data_available(cb->S, cb->E, CBUF_SIZE);
        size = data < MSS ? data : MSS;

        /* store a new packet */
        store_pkt(pkts, *last_seqnum, size, cb);
        *last_seqnum = (*last_seqnum + 1) % MAXSEQNUM;
        cb->S = (cb->S + size) % CBUF_SIZE;

        if (pthread_cond_signal(&cb->cnd_not_full) != 0)
            handle_error("pthread_cond_signal");
    }

    if (pthread_mutex_unlock(&cb->mtx) != 0)
        handle_error("pthread_mutex_unlock");
}












/*
 * Function:	fprint_pkt
 * -------------------------------------
 * Debug print function.
 */
void fprint_pkt(FILE * stream, void *p)
{
    struct packet *pkt = p;

    fprintf(stream, "%u", pkt->sgt.seqnum);
    //fprintf(stream, "%lld.%.9ld", (long long) ts->tv_sec, ts->tv_nsec);
}




/*
 * Function:	pkt_exptimecmp	
 * -------------------------------------
 * Compare the expiration time of two segments.
 *
 * Parameters:
 * 		xp	the address of the first packet
 * 		yp	the address of the second packet
 *
 * 	Returns:
 * 		1
 * 		0
 * 		-1
 */
int pkt_exptimecmp(void *xp, void *yp)
{
    struct packet *x = xp;
    struct packet *y = yp;

    return timespec_cmp(&x->exptime, &y->exptime);
}




struct packet *dequeue_pkt(struct queue_t *queue)
{
    struct packet *pkt;
    struct node_t *node = NULL;

    if (dequeue_node(queue, node) == -1)
        handle_error("dequeue_node()");
    pkt = node->value;
    free(node);
    return pkt;
}




/*
 * Function:	pkt_settime
 * -----------------------------------------------------------
 * Register the time when the segment is send and the time when
 * his timeout will expire.
 *
 * Parameters:
 * 		pkt			packet info address
 * 		timeout		timeout value
 */
void pkt_settime(struct packet *pkt, struct timespec *timeout)
{
    if (clock_gettime(CLOCK_REALTIME, &pkt->sendtime) == -1)
        handle_error("getting packet timestamp");

    timespec_add(&pkt->exptime, &pkt->sendtime, timeout);
}




bool pkt_expired(struct packet *pkt)
{
    struct timespec now;

    if (clock_gettime(CLOCK_REALTIME, &now) == -1)
        handle_error("clock_gettime()");

    if (timespec_cmp(&now, &pkt->exptime) < 0)
        return false;
    else
        return true;
}




/*
 * Function:	get_head_packet
 * -----------------------------------------------------
 * Dequeue the packet from the timeout queue.
 * 
 * Parameters:
 * 		queue	the address of the timout queue
 */
struct packet *get_head_packet(struct queue_t *queue)
{
    struct node_t *node = queue->head;
    struct packet *pkt = node->value;
    return pkt;
}




/*
 * Function:	send_packet
 * -----------------------------------------------------------
 * Extract the segment from the packet and send it.
 *
 * Parameters:
 * 		sockfd	the socket file descriptor
 * 		pkt		the address of the packet
 * 		loss	the loss probability
 */
void send_packet(int sockfd, struct packet *pkt, double loss)
{
    struct segment *sgt = &pkt->sgt;
    if (udt_send(sockfd, sgt, sizeof(struct segment), loss) == -1)
        handle_error("udt_send() - sending packet");
}




/* Function:	resend_expired
 * -----------------------------------------------------------------------
 * Send the expired segments, checking all the timestamps that are older
 * than current time until the queue is empty or a segment is not expired.
 *
 * Parameters:
 * 		sockfd:			the socket file descriptor
 * 		loss:			loss probability
 * 		time_queue:		packets' timestamps queue
 * 		timeout:		timeout value
 * 		w:				packets' active seqnum window
 */
void resend_expired(int sockfd, double loss, struct queue_t *time_queue,
                    struct timespec *timeout, struct window *w)
{
    struct packet *pkt;

    while (time_queue->head != NULL) {

        /* fetch first to expire packet */
        pkt = get_head_packet(time_queue);

        if (!pkt_expired(pkt))
            break;

        /* packet expired */

        dequeue(time_queue);
        //fprint_queue(stderr, time_queue, fprint_pkt);

        /* check if packed has been acked */
        if (pkt_acked(w, pkt->sgt.seqnum))
            continue;

        fprintf(stderr, "try to resend packet %u\n", pkt->sgt.seqnum);
        send_packet(sockfd, pkt, loss);
        pkt->rtx = true;

        /* set packet time */
        pkt_settime(pkt, timeout);

        prio_enqueue(pkt, time_queue, pkt_exptimecmp);
        //fprint_queue(stderr, time_queue, fprint_pkt);
    }
}




/*
 * Function:	more_packets
 * --------------------------------------------
 * States if the are more packets stored but not sent,
 * also consedering when lastseqnum restart from the beginning 
 * of the circular buffer and the base is still at the end of it.
 *
 * Parameters:
 * 		next	index of the next packet to send
 * 		base	index of the base of the window
 * 		last	index of the next packet to store
 *
 * Returns:
 * 		true:	there is at least one packet to send
 * 		false:	otherwise
 */
bool more_packets(unsigned int next, unsigned int base, unsigned int last)
{
    if (base <= last)
        return next < last;
    else
        return !(next < base && next > last);
}




/*
 * Function		send_packets
 * -------------------------------------------------------------------------------
 * Send the segments stored in the local buffer, register their 
 * send and expiration time and add them to the timeout queue.
 * Do this as long as the index of the next segment to send is 
 * inside the window and there are segments to send.
 *
 * Parameters:
 * 		sockfd		the socket file descriptor
 * 		loss		the segment's loss probability
 * 		pkts		the address of the local buffer containing the packets to send
 * 		lastseqnum	the index of the last segment passed by application
 * 		w			the address of the send window
 * 		timequeue	the queue of the segments' expiration times 
 * 		timeout		the timeout value
 */
void send_packets(int sockfd, double loss, struct packet *pkts,
                  unsigned int lastseqnum, struct window *w,
                  struct queue_t *time_queue, struct timespec *timeout)
{
    static unsigned int nextseqnum = 0;
    struct packet *pkt;         // packet pointer

    while (in_window(w, nextseqnum) &&
           more_packets(nextseqnum, w->base, lastseqnum)) {
        // nextseqnum is inside the window and
        // there are packets not sent yet

        pkt = pkts + nextseqnum;

        fprintf(stderr, "try to send packet %u\n", nextseqnum);
        send_packet(sockfd, pkt, loss);

        /* set packet sendtime and exptime */
        pkt_settime(pkt, timeout);

        prio_enqueue(pkt, time_queue, pkt_exptimecmp);
        //fprint_queue(stderr, time_queue, fprint_pkt);

        nextseqnum = (nextseqnum + 1) % MAXSEQNUM;
    }

    fprintf(stderr, "base = %u, nextseqnum = %u, lastseqnum = %u\n",
            w->base, nextseqnum, lastseqnum);
}



/*
 * Function:	calc_wait_time	
 * --------------------------------------------------------------
 * Calculate remaining time to timeout
 *
 * Parameters:
 * 		q:			queue containing packet's timestamps	
 * 		wait_time: 	struct that will contain the new wait time
 *
 * Returns:
 *		 0:	success
 *		-1: the head packet's timeout expired
 */
int calc_wait_time(struct queue_t *q, struct timespec *wait_time)
{
    struct packet *pkt;
    struct timespec now, left;

    if (clock_gettime(CLOCK_REALTIME, &now) == -1)
        handle_error("clock_gettime()");

    if (!q->head) {
        /* queue is empty: turn off the timeout */
        left.tv_sec = 15;
        left.tv_nsec = 0;
    } else {
        pkt = get_head_packet(q);

        /* calculate remaining time to timeout */
        if (timespec_sub(&left, &pkt->exptime, &now) == -1)
            /* now > exptime: timeout expired */
            return -1;
    }

    timespec_add(wait_time, &now, &left);
    return 0;
}




void update_timeout(struct timespec *timeout, struct packet *pkt)
{
    struct timespec now, elapsed;

    /* check if packet was retransmitted */
    if (pkt->rtx)
        return;

    if (clock_gettime(CLOCK_REALTIME, &now) == -1)
        handle_error("ack timestamp");

    if (timespec_sub(&elapsed, &now, &pkt->sendtime) == -1)
        handle_error("calculating elapsed time");

    adapt_timeout(timeout, &elapsed);
}




void *send_service(void *p)
{
    struct packet pkts_buffer[MAXSEQNUM];
    struct queue_t time_queue;
    struct timespec wait_time;
    struct timespec timeout;
    struct window w;

    struct shared_tools *tools = p;
    struct circular_buffer *cb = tools->cb;
    struct event *e = tools->e;
    struct proto_params *params = tools->params;
    int sockfd = tools->sockfd;

    double loss = params->P / 100.0;
    unsigned int lastseqnum = 0;
    uint8_t acknum;
    int condret;


    /* initialize send window */
    w.base = 0;
    w.width = params->N;
    reset(&w.ack_bar);

    /* initialize timeout queue */
    time_queue.head = time_queue.tail = NULL;

    /* initialize timeout */
    nsectots(&timeout, (long long) params->T * 1000000);

    if (pthread_mutex_lock(&e->mtx) != 0)
        handle_error("pthread_mutex_lock");

    for (;;) {

        e->type = NO_EVENT;
        if (pthread_cond_broadcast(&e->cnd_no_event) != 0)
            handle_error("pthread_cond_broadcast()");

        condret = 0;
        while (e->type == NO_EVENT && condret != ETIMEDOUT) {
            // no events and timeout not expired

            /* calculate remaining time to wait */
            if (calc_wait_time(&time_queue, &wait_time) == -1) {
                /* timeout expired: resend expired packets */
                resend_expired(sockfd, loss, &time_queue, &timeout, &w);
                fputs
                    ("timeout expired while calculating remaining time to timeout\n",
                     stderr);
                continue;
            }

            condret =
                pthread_cond_timedwait(&e->cnd_event, &e->mtx, &wait_time);
            if (condret != 0 && condret != ETIMEDOUT)
                handle_error("pthread_cond_timedwait");
        }

        /* TIMEOUT EVENT */
        if (condret == ETIMEDOUT) {
            fputs("TIMEOUT EVENT\n", stderr);
            resend_expired(sockfd, loss, &time_queue, &timeout, &w);
            continue;
        }

        switch (e->type) {

        case PKT_EVENT:
            fputs("PACKET EVENT\n", stderr);
            break;

        case ACK_EVENT:
            fputs("ACK EVENT\n", stderr);
            acknum = e->acknum;
            fprintf(stderr, "Received ACK %d\n", acknum);
            if (params->adaptive)
                update_timeout(&timeout, pkts_buffer + acknum);
            fprint_timespec(stderr, &timeout);
            update_window(&w, acknum);
            break;

        default:
            fputs("Unexpected event type\n", stderr);
            break;
        }

        /* empty shared buffer and put segments into the local one */
        empty_buffer(cb, pkts_buffer, &w, &lastseqnum);
        /* send available segments */
        send_packets(sockfd, loss, pkts_buffer, lastseqnum, &w,
                     &time_queue, &timeout);
    }

    if (pthread_mutex_unlock(&e->mtx) != 0)
        handle_error("pthread_mutex_unlock");

    return NULL;
}




/*
 * Function:	deliver_segment
 * ---------------------------------------------------------------
 * Put the just arrived segment on the shared circular buffer if 
 * there is enough free space.
 *
 * Parameters:
 * 		cb:		circular buffer address
 * 		sgt:	segment address
 */
void deliver_segment(struct circular_buffer *cb, struct segment *sgt)
{
    if (pthread_mutex_lock(&cb->mtx) != 0)
        handle_error("pthread_mutex_lock");

    /* check free space */
    while (space_available(cb->S, cb->E, CBUF_SIZE) <= MSS)
        if (pthread_cond_wait(&cb->cnd_not_full, &cb->mtx) != 0)
            handle_error("pthread_cond_wait");

	memcpy_tocb(cb->buf, sgt->payload, sgt->size, cb->E, CBUF_SIZE);
	cb->E = (cb->E + sgt->size) % CBUF_SIZE;

    if (pthread_cond_signal(&cb->cnd_not_empty) != 0)
        handle_error("pthread_cond_signal");
    if (pthread_mutex_unlock(&cb->mtx) != 0)
        handle_error("pthread_mutex_unlock");
}




/*
 * Function		process_segment
 * ------------------------------------------------------------------
 * Check if the received segment is into the receiving window, and mark it
 * as arrived, otherwise ignore the segment.
 * If the segment's sequence number match the base of the window, deliver
 * all the arrived segments with consicutive sequence number starting from
 * the base.
 *
 * Parameters:
 * 		sgt:			the address of the segment to process
 * 		segments_cb:	the buffer containing the segments to deliver
 * 		w:				the receive window
 * 		cb:				the shared buffer with application layer
 *
 * Returns:
 *		true:	the sequnce number is between [base - N; base + N)
 *				and must send an ack to the sender
 *		false:	otherwise
 */
bool process_segment(struct segment *sgt, struct segment *segments_cb,
                     struct window *w, struct circular_buffer *cb)
{
    static unsigned int S = 0;
    unsigned int i, s, seqnum;

    seqnum = sgt->seqnum;
    fprintf(stderr, "received segment %u\n", seqnum);

    if (in_window(w, seqnum)) {

        /* calculate seqnum distance from the base of the window */
        i = distance(w, seqnum);

        /* check if the segment is a duplicate */
        if (is_duplicate(w, i)) {
            //fputs("Already received\n", stderr);
            return true;
        }

        /* store the segment */
        segments_cb[(S + i) % w->width] = *sgt;

        /* mark segment as arrived */
        if (set_bit(&w->ack_bar, i) == -1)
            handle_error("set_bit()");
        fprint_window(stderr, w);

        if (seqnum == w->base) {
            /* calculate the number of consecutive arrived segments */
            s = calc_shift(w);
            /* deliver consecutive arrived segments */
			for (i = 0; i < s; i++) {
				deliver_segment(cb, segments_cb + S);
				S = (S + 1) % w->width;
			}
            /* update window indexes */
            shift_window(w, s);
            w->base = (w->base + s) % MAXSEQNUM;
        }
        return true;
    } else if (in_prewindow(w, seqnum)) {
        //fputs("Already received\n", stderr);
        return true;
    }
    return false;
}




/*
 * Function:	recv_service
 * ------------------------------------------
 * Loop routine that read the socket.
 * Check the size of the received data in order to recognize the content.
 * Denpendig on the content, either handle segment arrivals or
 * signal ack arrivals to the sender routine.
 *
 * Parameters:
 * 		p:		a pointer to the required parameters and shared structs
 */
void *recv_service(void *p)
{
    struct shared_tools *tools = p;

    struct circular_buffer *cb = tools->cb; // shared buffer with application layer
    struct event *e = tools->e; // event struct to signal ack events
    struct proto_params *params = tools->params;

    struct window recv_window;  // window to implement selective repeat 
    struct segment segments_cb[params->N];  // buffer to store arrived segments
    struct segment *sgt;        // temporary segment address buffer

    struct timeval timeout;     // connection timeout

    double loss = params->P / 100.0;
    uint8_t acknum;             // temporary ack buffer
    size_t max_recvsize = sizeof(struct segment);   // receive buffer max size
    char buffer[max_recvsize];  // receive buffer
    int sockfd = tools->sockfd; // socket file descriptor
    ssize_t r;                  // return value for the read


    /* initialize recv_window */
    recv_window.base = 0;
    recv_window.width = params->N;
    reset(&recv_window.ack_bar);


    /* set connection timeout */
    timeout.tv_sec = 30;
    timeout.tv_usec = 0;
    if (setsockopt
        (sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) == -1)
        handle_error("recv_service - setting socket timeout");


    for (;;) {

        r = read(sockfd, buffer, max_recvsize);

        if (r == -1) {

            if (errno == EINTR)
                // signal interruption
                continue;

            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // timeout expired: close connection
                puts("Connection expired\n");
                exit(EXIT_SUCCESS);
            }

            handle_error("recv_service - read()");
        }


        /* segment received */
        if (r == sizeof(struct segment)) {

            sgt = (struct segment *) buffer;
            if (process_segment(sgt, segments_cb, &recv_window, cb)) {
                /* send ACK */
                fprintf(stderr, "try to send ACK %u\n", sgt->seqnum);
                if (udt_send
                    (tools->sockfd, &sgt->seqnum, sizeof(uint8_t),
                     loss) == -1)
                    handle_error("udt_send() - sending ACK");
            }
            continue;
        }


        /* ACK received */
        if (r == sizeof(acknum)) {

            acknum = (uint8_t) * buffer;
            //fprintf(stderr, "received ACK %u\n", acknum); 
            if (cond_ack_event_signal(e, acknum) == -1)
                handle_error("cond_event_signal()");
            continue;
        }


        fputs("recv_service: undefined data received\n", stderr);
    }

    return NULL;
}




/*
 * Function:	init_transport
 * ----------------------------------------------
 * Initialize shared structures and create sending and receiving
 * threads.
 *
 * Parameters:
 * 		sockfd	connection socket descriptor
 * 		params	protocol's parameters
 */
void init_transport(int sockfd, struct proto_params *params)
{
    pthread_t t;


    /* initialize circular buffers */

    recv_cb.E = recv_cb.S = 0;
    send_cb.E = send_cb.S = 0;


    /* initialize shared tools */

    recv_tools.sockfd = sockfd;
    recv_tools.e = &e;
    recv_tools.cb = &recv_cb;
    recv_tools.params = params;

    send_tools = recv_tools;
    send_tools.cb = &send_cb;


    /* initialize mutexes */

    if (pthread_mutex_init(&e.mtx, NULL) != 0)
        handle_error("pthread_mutex_init()");
    if (pthread_mutex_init(&recv_cb.mtx, NULL) != 0)
        handle_error("pthread_mutex_init()");
    if (pthread_mutex_init(&send_cb.mtx, NULL) != 0)
        handle_error("pthread_mutex_init()");


    /* initialize conditions */

    if (pthread_cond_init(&recv_cb.cnd_not_empty, NULL) != 0)
        handle_error("pthread_cond_init()");
    if (pthread_cond_init(&send_cb.cnd_not_empty, NULL) != 0)
        handle_error("pthread_cond_init()");

    if (pthread_cond_init(&recv_cb.cnd_not_full, NULL) != 0)
        handle_error("pthread_cond_init()");
    if (pthread_cond_init(&send_cb.cnd_not_full, NULL) != 0)
        handle_error("pthread_cond_init()");

    if (pthread_cond_init(&e.cnd_event, NULL) != 0)
        handle_error("pthread_cond_init()");
    if (pthread_cond_init(&e.cnd_no_event, NULL) != 0)
        handle_error("pthread_cond_init()");


    /* create threads */

    if (pthread_create(&t, NULL, recv_service, &recv_tools) != 0)
        handle_error("creating recv_service");

    if (pthread_create(&t, NULL, send_service, &send_tools) != 0)
        handle_error("creating send_service");
}
