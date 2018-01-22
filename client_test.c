#include "basic.h"
#include "clicmd.h"
#include "rw.h"
#include "simul_udt.h"
#include "transport.h"
#include "timespec_utils.h"


void client_job(void);
void test_job(struct proto_params *params);
void create_connection(int sockfd, struct sockaddr_in *addr);
void create_test_connection(int sockfd, struct sockaddr_in *addr);


int main(int argc, char **argv)
{
    int sockfd;
	int pid;
	unsigned int i;
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

    for (i = 0; i < 14; i++) {

		// create child 
		if ((pid = fork()) == -1)
			handle_error("fork()");

		if (!pid) {
			// create a new socket 
			sockfd = socket(AF_INET, SOCK_DGRAM, 0);
			if (sockfd == -1)
				handle_error("socket()");

			create_test_connection(sockfd, &servaddr);
		}

		// wait for child termination 
		if (wait(NULL) == -1)
			handle_error("wait");
        printf("Buried zombie %d\n", pid);
	}
	

	//create_test_connection(sockfd, &servaddr);

    close(sockfd);
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
	printf("testjob\n");

	struct timespec start, end, elapsed;
	char *filename = "test";
	FILE *file = fopen("test_files/test.csv", "a");
	if (!file)
		handle_error("fopen()");

	if (clock_gettime(CLOCK_REALTIME, &start) == -1)
		handle_error("getting test start time");

	cli_get(filename);

	if (clock_gettime(CLOCK_REALTIME, &end) == -1)
		handle_error("getting test end time");
	if (timespec_sub(&elapsed, &end, &start) == -1)
		handle_error("calculating test elapsed time");

	puts("tempo impiegato");
	fprintf(file, "%u ", params->N);
	//fprintf(stdout, "%u ", params->P);
	//fprintf(stdout, "%u ", params->T);
	fprint_timespec(file, &elapsed);
	fclose(file);

	exit(EXIT_SUCCESS);
}







void create_connection(int sockfd, struct sockaddr_in *addr)
{
    socklen_t addrlen;
    bool connected = false;
    struct timeval timeout;
    static struct proto_params params;

    addrlen = sizeof(struct sockaddr);

    /* set receiving timeout on the socket */
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    if (setsockopt
        (sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) == -1)
        handle_error("setting socket timeout");


    while (!connected) {

        /* send SYN */
        if (udt_sendto
            (sockfd, NULL, 0, (struct sockaddr *) addr, addrlen,
             0.3) == -1)
            handle_error("udt_sendto() - sending SYN");
        fputs("SYN sent, waiting for SYN ACK\n", stderr);

        /* get SYN_ACK and server connection address */
        errno = 0;
        if (recvfrom
            (sockfd, &params, sizeof(params), 0, (struct sockaddr *) addr,
             &addrlen) == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                // timeout expired
                continue;
            handle_error("recvfrom()");
        }

        connected = true;
    }


    /* turn timeout off */
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    if (setsockopt
        (sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) == -1)
        handle_error("setting socket timeout");

    /* set the endpoint */
    if (connect(sockfd, (struct sockaddr *) addr, addrlen) == -1)
        handle_error("connect()");

    /* initialize transport layer */
    init_transport(sockfd, &params);
}

void client_job(void)
{
    unsigned short cmd_code;
    char line[MAXLINE], *filename, *cmd;

    for (;;) {

        filename = NULL;

        if (!fgets(line, MAXLINE, stdin)) {
            perror("fgets()");
            continue;
        }

        /* replace '\n' with '\0' */
        line[strcspn(line, "\n")] = 0;

        cmd = extract_cmd(line);
        if (!cmd)
            handle_error("parsing command from input");

        cmd_code = get_cmdcode(cmd);


        switch (cmd_code) {


        case LIST:
            puts("case LIST\n");
            cli_list();
            break;


        case GET:
            puts("case GET\n");
            filename = extract_filename(line);
            if (!filename)
                handle_error("parsing filename from input()");
            fprintf(stderr, "filename: \"%s\"\n", filename);
            cli_get(filename);
            break;


        case PUT:
            puts("case PUT\n");
            if ((filename = extract_filename(line)) == NULL)
                handle_error("parsing filename from input()");
            fprintf(stderr, "filename: \"%s\"\n", filename);
            cli_put(filename);
            break;


        default:
            fprintf(stderr, "Command not found.\n");

        }

        free(filename);
        free(cmd);
    }
}
