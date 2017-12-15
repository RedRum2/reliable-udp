#ifndef _TIMESPEC_UTILS_H
#define _TIMESPEC_UTILS_H

#include <time.h>
#include <stdio.h>

int timespec_cmp(struct timespec *x, struct timespec *y);
int timespec_sub(struct timespec *result, struct timespec *x, struct timespec *y);
void timespec_add(struct timespec *result, struct timespec *x, struct timespec *y);
void fprint_timespec(FILE *stream, struct timespec *ts);
long long tstonsec(struct timespec *ts);
void nsectots(struct timespec *ts, long long x);

#endif /* _TIMESPEC_UTILS_H */
