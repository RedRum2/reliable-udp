#include "adaptive.h"
#include "timespec_utils.h"

#include <stdlib.h>

#define MIN_TIMEOUT_NSEC	250000000


long long calc_estimated_rtt(long long sample)
{
    static long long estimated = 1000000000;    // 1 second

    /*  est = (1-alpha)*est + alpha*sample
     *  alpha=1/8                           */
    estimated = estimated * 0.875 + (sample >> 3);

    return estimated;
}




long long calc_dev_rtt(long long sample, long long estimated)
{
    static long long dev = 0;
    long long sample_dev;

    /*  dev = (1-beta)*dev + beta*|sample-est|
     *  beta = 1/4                              */
    sample_dev = llabs(sample - estimated);
    dev = dev * 0.75 + (sample_dev >> 2);

    return dev;
}




void adapt_timeout(struct timespec *timeout, struct timespec *elapsed)
{
    long long estimated_rtt, sample_rtt, dev_rtt, timeout_nsec;

    sample_rtt = tstonsec(elapsed);

    estimated_rtt = calc_estimated_rtt(sample_rtt);
    dev_rtt = calc_dev_rtt(sample_rtt, estimated_rtt);

    timeout_nsec = estimated_rtt + 4 * dev_rtt;

    if (timeout_nsec < MIN_TIMEOUT_NSEC)
        timeout_nsec = MIN_TIMEOUT_NSEC;

    nsectots(timeout, timeout_nsec);
}
