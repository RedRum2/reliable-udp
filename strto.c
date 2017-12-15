#include "strto.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


#define MIN_PORT 	49152
#define MAX_PORT 	65535
#define MAX_LOSS	100
#define MIN_WIDTH	1
#define MAX_WIDTH	127
#define MIN_TIMEOUT	100
#define MAX_TIMEOUT	3000


unsigned long argtoul(const char *arg)
{
    char *p;
    unsigned long v;

    errno = 0;
    v = strtoul(arg, &p, 0);
    if (errno != 0 || *p != '\0') {
        fprintf(stderr, "Invalid argument '%s'\n", arg);
        exit(EXIT_FAILURE);
    }

    return v;
}



uint16_t strtoport(const char *arg)
{
    unsigned long port = argtoul(arg);

    if (port < MIN_PORT || port > MAX_PORT) {
        fprintf(stderr,
                "Port number '%lu' out of range [%d, %d]\n",
                port, MIN_PORT, MAX_PORT);
        exit(EXIT_FAILURE);
    }
    /* port number < 2^16 : no loss of data after the cast */
    return (uint16_t) port;
}



uint16_t strtotimeout(const char *arg)
{
    unsigned long msec = argtoul(arg);

    if (msec < MIN_TIMEOUT || msec > MAX_TIMEOUT) {
        fprintf(stderr,
                "Timeout (msec) '%lu' out of range [%d, %d]\n",
                msec, MIN_TIMEOUT, MAX_TIMEOUT);
        exit(EXIT_FAILURE);
    }
    /* msec < 2^16 : no loss of data after the cast */
    return (uint16_t) msec;
}



uint8_t strtowidth(const char *arg)
{
    unsigned long width = argtoul(arg);

    if (width < MIN_WIDTH || width > MAX_WIDTH) {
        fprintf(stderr,
                "Window width '%lu' out of range [%d, %d]\n",
                width, MIN_WIDTH, MAX_WIDTH);
        exit(EXIT_FAILURE);
    }
    /* port number < 2^8 : no width of data after the cast */
    return (uint8_t) width;
}



uint8_t strtoloss(const char *arg)
{
    unsigned long loss = argtoul(arg);

    if (loss > MAX_LOSS) {
        fprintf(stderr,
                "Loss probability '%lu' out of range [0; %d]\n",
                loss, MAX_LOSS);
        exit(EXIT_FAILURE);
    }
    /* loss <= 100 < 2^8 : no loss of data after the cast */
    return (uint8_t) loss;
}
