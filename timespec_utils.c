#include "timespec_utils.h"

#include <errno.h>



int timespec_cmp(struct timespec *x, struct timespec *y)
{
    return (x->tv_sec < y->tv_sec ? -1
            : x->tv_sec > y->tv_sec ? 1 : (int) (x->tv_nsec - y->tv_nsec));
}



int timespec_sub(struct timespec *result, struct timespec *x,
                 struct timespec *y)
{
    long long carry_nsec;

    if (timespec_cmp(x, y) < 0) {
        errno = EINVAL;
        return -1;
    }

    if (x->tv_nsec < y->tv_nsec) {
        carry_nsec = (long long) x->tv_nsec + 1000000000;
        result->tv_nsec = carry_nsec - y->tv_nsec;
        result->tv_sec = x->tv_sec - y->tv_sec - 1;
    } else {
        result->tv_nsec = x->tv_nsec - y->tv_nsec;
        result->tv_sec = x->tv_sec - y->tv_sec;
    }

    return 0;
}



void timespec_add(struct timespec *result, struct timespec *x,
                  struct timespec *y)
{
    time_t carry;
    long long nsum;

    nsum = x->tv_nsec + y->tv_nsec;
    carry = (time_t) (nsum / 1000000000);
    result->tv_nsec = nsum % 1000000000;
    result->tv_sec = x->tv_sec + y->tv_sec + carry;
}



void fprint_timespec(FILE * stream, struct timespec *ts)
{
    fprintf(stream, "%lld.%.9ld\n", (long long) ts->tv_sec, ts->tv_nsec);
}



long long tstonsec(struct timespec *ts)
{
    long long result;

    result = ts->tv_sec * 1000000000;
    result += ts->tv_nsec;

    return result;
}



void nsectots(struct timespec *ts, long long nsec)
{
    ts->tv_sec = (time_t) (nsec / 1000000000);
    ts->tv_nsec = nsec % 1000000000;
}
