#ifndef _RW_H
#define _RW_H


#include <stdlib.h>

char *extract_cmd(const char *);
char *extract_filename(const char *);
ssize_t writen(int, const void *, size_t);
ssize_t readn(int, void *, size_t);
ssize_t read_string(int, void *, size_t);


#endif /* _RW_H */
