#include "basic.h"
#include "simul_udt.h"
#include "transport.h"
#include "srvcmd.h"
#include "strto.h"


void parse_args(int argc, char **argv, struct proto_params *params,
                uint16_t * port);
void server_job(void);
void create_connection(struct proto_params *params,
                       struct sockaddr_in *cliaddr, socklen_t clilen);
void register_zombie_handler(void);
void sig_zombie_handler(int sig);



int main(int argc, char **argv)
{
    int sockfd, reuse, pid;
    struct sockaddr_in servaddr, cliaddr;
    struct proto_params params;
    socklen_t clilen;
    char *buf[MAXLINE];
    uint16_t server_port;


    /* init configuration parameters with default values */
    params.N = 30;
    params.T = 1000;            // milliseconds
    params.P = 10;              // decimal part
    params.adaptive = 1;        // boolean value
    server_port = SERVER_PORT;


    /* parse arguments */
    if (argc > 1)
        parse_args(argc, argv, &params, &server_port);


    /* create listen socket */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1)
        handle_error("socket()");


    /* clear server address structure */
    memset((void *) &servaddr, 0, sizeof(servaddr));
    /* set server address */
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERVER_PORT);
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);


    /* 
     * allow reuse of local address if there is not an active listening
     * socket bound to the address
     */
    reuse = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int))
        == -1)
        handle_error("setsosckopt()");


    /* bind the socket to the address */
    if (bind(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) ==
        -1)
        handle_error("bind()");

	
	/* register SIGCHLD signal handler */
	register_zombie_handler();


    for (;;) {

        clilen = sizeof(cliaddr);
        memset((void *) &cliaddr, 0, clilen);


        /* wait for connection requests */
		errno = 0;
        if (recvfrom
            (sockfd, buf, MAXLINE, 0, (struct sockaddr *) &cliaddr,
             &clilen) == -1) {

			 if (errno == EINTR)
			 	// signal interruption
			 	continue;

            handle_error("waiting for connection requests");
		}


		/* create a new proccess to handle the client requests */
		pid = fork();

		if (pid == -1)
			handle_error("fork()");

		if (!pid) {                 // child process

			/* close duplicated listen socket */
			if (close(sockfd) == -1)
				handle_error("close()");

			create_connection(&params, &cliaddr, clilen);
		}
    }

    /* NEVER REACHED */
    close(sockfd);
    exit(EXIT_SUCCESS);
}





void parse_args(int argc, char **argv, struct proto_params *params,
                uint16_t * port)
{
    int c;

    while ((c = getopt(argc, argv, "P:N:T:a")) != -1) {
        switch (c) {
        case 'P':
            params->P = strtoloss(optarg);
            break;
        case 'N':
            params->N = strtowidth(optarg);
            break;
        case 'T':
            params->T = strtotimeout(optarg);
            break;
        case 'a':
            params->adaptive = 1;
            break;
        case '?':              // option not recognized or missing required arg
            fprintf(stderr,
                    "Usage: %s [port] [-P loss] [-N width] [-T timeout] [-a]\n",
                    argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    /* optind is the first index of argv that is not an option */
    if (optind < argc)
        *port = strtoport(argv[optind]);
}



void create_connection(struct proto_params *params,
                       struct sockaddr_in *cliaddr, socklen_t clilen)
{
    int connsd;

	/* create a connection socket */
	if ((connsd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
		handle_error("socket()");

	/* set the end point */
	if (connect(connsd, (struct sockaddr *) cliaddr, clilen) == -1)
		handle_error("socket()");

	/* send the first unreliable SYN_ACK with protocol parameters */
	if (udt_send(connsd, params, sizeof(struct proto_params), params->P / 100.0) == -1)
		handle_error("udt_send() - sending SYN_ACK");

	init_transport(connsd, params);

	server_job();
}



void server_job(void)
{
    uint8_t cmd;

    for (;;) {

        puts("Waiting for requests");
        cmd = recvcmd();

        switch (cmd) {

        case LIST:
            puts("LIST request received");
            srv_list();
            break;

        case GET:
            puts("GET request received");
            srv_get();
            break;

        case PUT:
            puts("PUT request received");
            srv_put();
            break;

        default:
            puts("Unknown command received");
        }
    }
}


void sig_zombie_handler(int sig)
{
  	pid_t	pid;

	(void) sig;	

  	while((pid = waitpid(-1, NULL, WNOHANG)) > 0)
		printf("Buried zombie %d\n", pid);  // UNSAFE: non-reentrant function

  return;
}


void register_zombie_handler(void)
{
	struct sigaction sa;

	sa.sa_handler = sig_zombie_handler;
	sa.sa_flags = SA_RESTART;
	sigemptyset(&sa.sa_mask);

	if (sigaction(SIGCHLD, &sa, NULL) == -1)
		handle_error("sigaction()");
}
