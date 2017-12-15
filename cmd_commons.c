#include "cmd_commons.h"
#include "transport.h"
#include "rw.h"


void send_file(int fd, void *buffer, size_t file_size,
               size_t header_size)
{
    size_t buf_size, total_size = header_size + file_size;
    unsigned int i, n = total_size / MAX_BUFSIZE;

    for (i = 0; i <= n; i++) {

        if (i != 0)             // consider header only at the first pass
            header_size = 0;

        if (i == n) {           // calculate last bytes to send
            buf_size = total_size % MAX_BUFSIZE;
            if (!buf_size)
                break;          // total size is a multiple of MAX_BUFSIZE: send only n-1 chunks 
        } else
            buf_size = MAX_BUFSIZE;

        if (readn(fd, buffer + header_size, buf_size - header_size) == -1)
            handle_error("GET - reading file");

        rdt_send(buffer, buf_size);
    }
}


void recv_file(int fd, size_t size)
{
    unsigned int i, n = size / MAX_BUFSIZE;
    size_t buf_size;
    int8_t buffer[MAX_BUFSIZE];

    for (i = 0; i <= n; i++) {

        if (i == n) {           // calculate last bytes to send
            buf_size = size % MAX_BUFSIZE;
            if (!buf_size)
                break;          // size is a multiple of MAX_BUFSIZE: receive only n-1 chunks
        } else
            buf_size = MAX_BUFSIZE;

        rdt_recv(buffer, buf_size);

        if (writen(fd, buffer, buf_size) == -1)
            handle_error("rdt_recv - writing file");
    }
}
