#ifndef _CB_UTILS_H 
#define _CB_UTILS_H 

#include <unistd.h>
#include <stdbool.h>


bool cbuf_free(unsigned int start, unsigned int end, size_t size);
void memcpy_tocb(void *dest_cb, const void *source, size_t n,
                 unsigned int begin, size_t size);
void memcpy_fromcb(void *dest, const void *source_cb, size_t n,
                   unsigned int begin, size_t size);
size_t data_available(unsigned int s, unsigned int e, size_t size);
size_t space_available(unsigned int s, unsigned int e, size_t size);

#endif /* _CB_UTILS_H */
