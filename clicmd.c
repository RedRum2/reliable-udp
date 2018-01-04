#include "basic.h"
#include "transport.h"
#include "cmd_commons.h"


/* Function:	get_cmdcode
 * ------------------------------------------
 * Return the code related to the string in input.
 *
 * Parameters:
 * 		input	command in string format
 *
 * Returns:
 * 		the code related to the command
 */
unsigned short get_cmdcode(const char *input)
{
    unsigned short i;
    char *commands[] = { "list", "get", "put" };

    for (i = 0; i <= MAXCMD; i++)
        if (strcmp(input, commands[i]) == 0)
            break;

    return i;
}



void cli_list()
{
    uint8_t cmd = LIST;
    uint64_t file_size;
    char *buffer;

    /* send LIST command */
    rdt_send(&cmd, sizeof(cmd));

    /* read list size */
    rdt_recv(&file_size, sizeof(file_size));

    /* allocate buffer */
    buffer = malloc(file_size);
    if (!buffer)
        handle_error("malloc()");

    /* recv file list */
    rdt_recv(buffer, file_size);

    /* print file list and free memory */
    printf("\n%s\n", buffer);
    free(buffer);
}



void cli_get(const char *filename)
{
    uint64_t file_size;
    uint8_t code;
    int fd;

    size_t buf_size = sizeof(uint8_t) + strlen(filename) + sizeof(char);
    uint8_t buffer[buf_size];

    /* build request */
    buffer[0] = GET;
    memcpy(buffer + 1, filename, strlen(filename) + sizeof(char));

    /* send request */
    rdt_send(buffer, buf_size);

    /* read response code */
    rdt_recv(&code, sizeof(code));

    /* check response code */
    if (code == GET_NOENT) {    // file not found
        printf("File \"%s\" does not exist.\n", filename);
        return;
    }

    /* read file size */
    rdt_recv(&file_size, sizeof(file_size));

    /* open file */
    if ((fd = open(filename, O_WRONLY | O_CREAT, 0644)) == -1)
        handle_error("open() - opening GET destination file");

    /* receive and store the file */
    recv_file(fd, file_size);

    /* close file */
    if (close(fd) == -1)
        handle_error("close() - closing GET destination file");
}



void cli_put(const char *filename)
{
    struct stat st;
    int fd;
    size_t header_size;
    uint8_t *header;
    uint8_t cmd = PUT, outcome;
    uint64_t file_size;

    /* open the file */
    errno = 0;
    fd = open(filename, O_RDONLY);
    if (fd == -1) {
        if (errno == ENOENT) {  // The file does not exist
            printf("The file \"%s\" does not exist\n", filename);
            return;
        } else
            handle_error("open() - opening PUT file");
    }

    /* get file size */
    if (fstat(fd, &st) == -1)
        handle_error("fstat() - getting PUT file stats");
    file_size = st.st_size;

    /* allocate the header buffer */
    header_size = sizeof(cmd) +
        strlen(filename) + sizeof(char) + sizeof(file_size);
    header = malloc(header_size);
    if (!header)
        handle_error("malloc() - allocating PUT header");

    /* set the header */
    header[0] = cmd;
    memcpy(header + sizeof(cmd), filename,
           strlen(filename) + sizeof(char));
    memcpy(header + sizeof(cmd) + strlen(filename) + sizeof(char),
           &file_size, sizeof(file_size));

    /* send file and free resources */
    send_file(fd, header, file_size, header_size);
    free(header);
    if (close(fd) == -1)
        handle_error("close() - closing PUT file");

    /* receive and print operation outcome */
    rdt_recv(&outcome, sizeof(outcome));
    if (outcome == PUT_SUCCESS)
        puts("PUT operation succeed!\n");
    else
        puts("PUT operation failed.\n");
}
