#ifndef _ADAPTIVE_H
#define _ADAPTIVE_H


#include <time.h>

void adapt_timeout(struct timespec *timeout, struct timespec *elapsed);


#endif /* _ADAPTIVE_H */
