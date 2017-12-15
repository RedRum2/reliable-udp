#ifndef _CMD_COMMONS_H
#define _CMD_COMMONS_H


#include <stdlib.h>

void send_file(int fd, void *buffer, size_t file_size, size_t header_size);
void recv_file(int fd, size_t size);


#endif /* _CMD_COMMONS_H */
