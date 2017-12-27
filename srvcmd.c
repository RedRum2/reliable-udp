#include "srvcmd.h"
#include "rw.h"
#include "transport.h"
#include "cmd_commons.h"

#include <sys/wait.h>

extern char **environ;


uint8_t recvcmd(void)
{
    uint8_t cmd;

    rdt_recv(&cmd, sizeof(uint8_t));

    return cmd;
}



void srv_list(void)
{
    struct stat st;

    int fd;
    uint64_t file_size;
    char *filename = "file_list.txt";
    uint8_t *buffer;

    char *cmd = "ls > file_list.txt";
    if (system(cmd) == -1)
        handle_error("system()");

    /* open the destination file of the list command */
    fd = open(filename, O_RDONLY);
    if (fd == -1)
        handle_error("opening dest file");

    /* Calculate file size */
    if (fstat(fd, &st) == -1)
        handle_error("fstat()");
    file_size = st.st_size;

	/* Allocate buffer memory */
	buffer = malloc(sizeof(file_size) + file_size);
	if (!buffer)
		handle_error("malloc - allocating list buffer");

	memcpy(buffer, &file_size, sizeof(file_size));

    send_file(fd, buffer, file_size, sizeof(file_size));
    if (close(fd) == -1)
        handle_error("closing file list");
}



void srv_get(void)
{
    struct stat st;
    int fd;
    size_t header_size;

    char filename[MAXLINE];
    uint8_t buffer[MAX_BUFSIZE];
    uint8_t response_code;
    uint64_t file_size;


    /* Read filename */
    if (rdt_read_string(filename, MAXLINE) <= 0)
        handle_error("reading filename");
    fprintf(stderr, "filename: %s\n", filename);


    /* Open the file */
    errno = 0;
    fd = open(filename, O_RDONLY);

    if (fd == -1) {
        if (errno == ENOENT) {  // The file does not exist
            response_code = GET_NOENT;
            rdt_send(&response_code, sizeof(response_code));
            return;
        } else
            handle_error("open()");
    }

    /* Calculate file size */
    if (fstat(fd, &st) == -1)
        handle_error("fstat()");
    file_size = st.st_size;

    /* set the header */
    buffer[0] = GET_OK;
    memcpy(buffer + 1, &file_size, sizeof(file_size));
    header_size = sizeof(uint8_t) + sizeof(file_size);

    /* send file and free resources */
    send_file(fd, buffer, file_size, header_size);
    if (close(fd) == -1)
        handle_error("close()");
}




void report_error(const char *msg)
{
	uint8_t outcome = PUT_FAILURE;
	rdt_send(&outcome, sizeof(outcome));
	fprintf(stderr, "PUT failed: %s\n", msg);
}




void srv_put(void)
{
    int fd;
    char filename[MAXLINE];
    uint8_t outcome;
    uint64_t file_size;


    /* read filename */
    if (rdt_read_string(filename, MAXLINE) <= 0) {
        report_error("reading filename ");
		return;
	}
    fprintf(stderr, "filename: %s\n", filename);

    /* read file size */
    rdt_recv(&file_size, sizeof(file_size));
    fprintf(stderr, "file size: %lu\n", file_size);

    /* open the file */
    fd = open(filename, O_WRONLY | O_CREAT, 0644);
    if (fd == -1) {
        report_error("opening file on writing");
		return;
	}

    /* receive and store the file */
    recv_file(fd, file_size);

    /* send positive outcome */
    outcome = PUT_SUCCESS;
    rdt_send(&outcome, sizeof(outcome));
}
