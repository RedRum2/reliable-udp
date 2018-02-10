void create_connection(struct proto_params *params,
                       struct sockaddr_in *cliaddr, socklen_t clilen)
{
    /* create a connection socket .... */
    /* set the end point .... */
    /* send SYN_ACK with protocol parameters .... */

    init_transport(connsd, params);
}


void create_connection(int sockfd, struct sockaddr_in *addr)
{
    /* set receiving timeout on the socket */

    while (!connected) {
        /* send SYN .... */
        /* get SYN_ACK and server connection address .... */
    }
    /* turn timeout off */
    /* set the endpoint */

    /* initialize transport layer */
    init_transport(sockfd, &params);
}
