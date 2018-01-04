#include "srvcmd.h"
#include "rw.h"
#include "transport.h"
#include "cmd_commons.h"


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
    uint8_t *header;

    /* execute ls command */
    char *cmd = "ls > file_list.txt";
    if (system(cmd) == -1)
        handle_error("system() - executing ls command");

    /* open the destination file of the list command */
    fd = open(filename, O_RDONLY);
    if (fd == -1)
        handle_error("open() - opening LIST file");

    /* calculate file size */
    if (fstat(fd, &st) == -1)
        handle_error("fstat() - getting LIST file stats");
    file_size = st.st_size;

    /* allocate header buffer */
    header = malloc(sizeof(file_size));
    if (!header)
        handle_error("malloc() - allocating LIST header");

    /* set the header */
    memcpy(header, &file_size, sizeof(file_size));

    /* send file and free resources */
    send_file(fd, header, file_size, sizeof(file_size));
    free(header);
    if (close(fd) == -1)
        handle_error("close() - closing file list");
}



void srv_get(void)
{
    struct stat st;
    int fd;
    size_t header_size;

    char filename[MAXLINE];
    uint8_t *header;
    uint8_t response_code;
    uint64_t file_size;


    /* Read filename */
    if (rdt_read_string(filename, MAXLINE) <= 0)
        handle_error("rdt_read_string() - reading requested filename");
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
            handle_error("open() - opening requested file");
    }

    /* Calculate file size */
    if (fstat(fd, &st) == -1)
        handle_error("fstat() - getting requested file stats");
    file_size = st.st_size;

    /* allocate the header buffer */
    header_size = sizeof(response_code) + sizeof(file_size);
    header = malloc(header_size);
    if (!header)
        handle_error("malloc() - allocating GET header");

    /* set the header */
    header[0] = GET_OK;
    memcpy(header + 1, &file_size, sizeof(file_size));

    /* send file and free resources */
    send_file(fd, header, file_size, header_size);
    free(header);
    if (close(fd) == -1)
        handle_error("close() - closing requested file");
}




void report_error(const char *msg)
{
    uint8_t outcome = PUT_FAILURE;
    rdt_send(&outcome, sizeof(outcome));
    printf("PUT failed: %s\n", msg);
}


void srv_put(void)
{
    int fd;
    char filename[MAXLINE];
    uint8_t outcome;
    uint64_t file_size;


    /* read filename */
    if (rdt_read_string(filename, MAXLINE) <= 0) {
        report_error("rdt_read_string() - reading PUT filename");
        return;
    }
    fprintf(stderr, "filename: %s\n", filename);

    /* read file size */
    rdt_recv(&file_size, sizeof(file_size));
    fprintf(stderr, "file size: %lu\n", file_size);

    /* open the file */
    fd = open(filename, O_WRONLY | O_CREAT, 0644);
    if (fd == -1) {
        report_error("open() - opening PUT file on writing");
        return;
    }

    /* receive and store the file */
    recv_file(fd, file_size);

    /* send positive outcome */
    outcome = PUT_SUCCESS;
    rdt_send(&outcome, sizeof(outcome));
}
