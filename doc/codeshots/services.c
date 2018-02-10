struct shared_tools {
	int sockfd;
	struct circular_buffer *cb;
	struct event *e;
	struct proto_params *params;
};

/* shared structures */
struct circular_buffer recv_cb;
struct circular_buffer send_cb;
struct event e;
/* threads args to keep alive */
struct shared_tools recv_tools, send_tools;

void init_transport(int sockfd, struct proto_params *params)
{
    pthread_t t;

    .....
    
    /* initialize shared tools */
    recv_tools.sockfd = sockfd;
    recv_tools.e = &e;
    recv_tools.cb = &recv_cb;
    recv_tools.params = params;
    send_tools = recv_tools;
    send_tools.cb = &send_cb;

    .....
    
    /* create threads */
    if (pthread_create(&t, NULL, recv_service, &recv_tools) != 0)
        handle_error("creating recv_service");
    if (pthread_create(&t, NULL, send_service, &send_tools) != 0)
        handle_error("creating send_service");
}
