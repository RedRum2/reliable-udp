#include "cmd_commons.h"
#include "transport.h"
#include "rw.h"

/*
 * Function:	send_file
 * --------------------------------------------------
 * Send a message composed by a header and a file.
 * Split out the file in order to allocate only a restricted
 * amount of memory and send a chunk at time.
 *
 * Parameters:
 * 		fd:				descriptor of the file to send
 * 		header:			address of the header buffer
 * 		file_size:		size of the file	
 * 		header_size:	size of the header
 */
void send_file(int fd, void *header, size_t file_size, size_t header_size)
{
    int8_t buffer[MAX_BUFSIZE];
    size_t buf_size, total_size;
    unsigned int i, n;

    total_size = header_size + file_size;
    n = total_size / MAX_BUFSIZE;

    memcpy(buffer, header, header_size);

    for (i = 0; i <= n; i++) {

        if (i == n) {           // calculate last bytes to send
            buf_size = total_size % MAX_BUFSIZE;
            if (!buf_size)
                break;          // total size is a multiple of MAX_BUFSIZE: send only n-1 chunks 
        } else
            buf_size = MAX_BUFSIZE;

        if (readn(fd, buffer + header_size, buf_size - header_size) == -1)
            handle_error("readn() - reading file to send");

        header_size = 0;        // consider header only at the first pass

        rdt_send(buffer, buf_size);
    }
}


/*
 * Function:	recv_file
 * --------------------------------------------------
 * Store a received file one chunk at time in order to save memory.
 *
 * Parameters:
 * 		fd:				descriptor of the file to send
 * 		file_size:		size of the file	
 */
void recv_file(int fd, size_t size)
{
    unsigned int i, n = size / MAX_BUFSIZE;
    size_t buf_size;
    int8_t buffer[MAX_BUFSIZE];

    for (i = 0; i <= n; i++) {

        if (i == n) {           // calculate last bytes to store 
            buf_size = size % MAX_BUFSIZE;
            if (!buf_size)
                break;          // size is a multiple of MAX_BUFSIZE: receive only n-1 chunks
        } else
            buf_size = MAX_BUFSIZE;

        rdt_recv(buffer, buf_size);

        if (writen(fd, buffer, buf_size) == -1)
            handle_error("writen() - writing received file");
        printf("\rDownloading file: %u%%", i * 100 / n);
        fflush(stdout);
    }
    printf("\n");
}
