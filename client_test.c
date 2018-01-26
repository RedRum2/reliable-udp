#include "basic.h"
#include "clicmd.h"
#include "rw.h"
#include "simul_udt.h"
#include "transport.h"
#include "timespec_utils.h"


void test_job(struct proto_params *params);
void create_test_connection(int sockfd, struct sockaddr_in *addr);


int main(int argc, char **argv)
{
    int sockfd;
    int pid;
    struct sockaddr_in servaddr;


    /* input check */
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <server IP address>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    // reset server address 
    memset((void *) &servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERVER_PORT);
    if (inet_aton(argv[1], &servaddr.sin_addr) == 0)
        handle_error("inet_aton()");

    for (;;) {

        // create child 
        if ((pid = fork()) == -1)
            handle_error("fork()");

        if (!pid) {
            // create a new socket 
            sockfd = socket(AF_INET, SOCK_DGRAM, 0);
            if (sockfd == -1)
                handle_error("socket()");

            create_test_connection(sockfd, &servaddr);

            close(sockfd);
            exit(EXIT_SUCCESS);
        }
        // wait for child termination 
        if (wait(NULL) == -1)
            handle_error("wait");
        printf("Buried zombie %d\n", pid);
    }


    //create_test_connection(sockfd, &servaddr);

    exit(EXIT_SUCCESS);
}

void create_test_connection(int sockfd, struct sockaddr_in *addr)
{
    socklen_t addrlen;
    static struct proto_params params;
    addrlen = sizeof(struct sockaddr);

    /* send SYN */
    if (udt_sendto
        (sockfd, NULL, 0, (struct sockaddr *) addr, addrlen, 0.0) == -1)
        handle_error("udt_sendto() - sending SYN");

    fputs("SYN sent, waiting for SYN ACK\n", stderr);

    /* get SYN_ACK and server connection address */
    if (recvfrom
        (sockfd, &params, sizeof(params), 0, (struct sockaddr *) addr,
         &addrlen) == -1)
        handle_error("recvfrom()");

    /* set the endpoint */
    if (connect(sockfd, (struct sockaddr *) addr, addrlen) == -1)
        handle_error("connect()");

    /* initialize transport layer */
    init_transport(sockfd, &params);
    fputs("connected!\n", stderr);
    test_job(&params);
}


void test_job(struct proto_params *params)
{
    printf("testing GET...\n");

    struct timespec start, end, elapsed;
    char *filename = "test";
    FILE *file = fopen("test_files/test.dat", "a");
    if (!file)
        handle_error("fopen()");

    if (clock_gettime(CLOCK_REALTIME, &start) == -1)
        handle_error("getting test start time");

    cli_get(filename);

    if (clock_gettime(CLOCK_REALTIME, &end) == -1)
        handle_error("getting test end time");
    if (timespec_sub(&elapsed, &end, &start) == -1)
        handle_error("calculating test elapsed time");

    //printf("GET success! N = %u\n", params->N);
    printf("GET success! P = %u\n", params->P);
    //printf("GET success! T = %u\n", params->T);

    puts("time: ");
    fprint_timespec(stdout, &elapsed);

    //fprintf(file, "%u ", params->N);
    fprintf(file, "%u ", params->P);
    //fprintf(file, "%u ", params->T);
    fprint_timespec(file, &elapsed);
    fclose(file);
}
