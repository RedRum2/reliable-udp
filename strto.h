#ifndef _STRTO_H
#define _STRTO_H


#include <stdint.h>


#define MIN_PORT 	49152
#define MAX_PORT 	65535
#define MAX_LOSS	100
#define MIN_WIDTH	1
#define MAX_WIDTH	127
#define MIN_TIMEOUT	250
#define MAX_TIMEOUT	3000


uint16_t strtoport(const char *arg);
uint16_t strtotimeout(const char *arg);
uint8_t strtowidth(const char *arg);
uint8_t strtoloss(const char *arg);


#endif /* _STRTO_H */
