#include "event.h"
#include <stdio.h>

int cond_event_signal(struct event *e, unsigned int event_type)
{
    int retval = 0;

    if (pthread_mutex_lock(&e->mtx) != 0)
        retval = -1;

    else {

        while (e->type != NO_EVENT)
            if (pthread_cond_wait(&e->cnd_no_event, &e->mtx) != 0)
                retval = -1;

        e->type = event_type;

        if (pthread_cond_signal(&e->cnd_event) != 0)
            retval = -1;

        if (event_type == ACK_EVENT)
            puts("ACK signaled");
        if (event_type == PKT_EVENT)
            puts("PKT signaled");

        if (pthread_mutex_unlock(&e->mtx) != 0)
            retval = -1;
    }

    return retval;
}



int cond_ack_event_signal(struct event *e, uint8_t acknum)
{
    int retval = 0;

    if (pthread_mutex_lock(&e->mtx) != 0)
        retval = -1;

    else {

        while (e->type != NO_EVENT)
            if (pthread_cond_wait(&e->cnd_no_event, &e->mtx) != 0)
                retval = -1;

        e->acknum = acknum;
        e->type = ACK_EVENT;

        if (pthread_cond_signal(&e->cnd_event) != 0)
            retval = -1;

        puts("ACK signaled");

        if (pthread_mutex_unlock(&e->mtx) != 0)
            retval = -1;
    }

    return retval;
}
