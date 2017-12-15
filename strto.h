#ifndef _STRTO_H
#define _STRTO_H


#include <stdint.h>



uint16_t strtoport(const char *arg);
uint16_t strtotimeout(const char *arg);
uint8_t strtowidth(const char *arg);
uint8_t strtoloss(const char *arg);


#endif /* _STRTO_H */
