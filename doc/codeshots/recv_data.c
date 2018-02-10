void *recv_service(void *p)
{
    for (;;) {

        r = read(sockfd, buffer, max_recvsize);

        if (r == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // timeout expired: close connection
                puts("Connection expired");
                exit(EXIT_SUCCESS);
            }
            handle_error("recv_service - read()");
        }

        /* segment received */
        if (r == sizeof(struct segment)) {
            // ....
            continue;
        }

        /* ACK received */
        if (r == sizeof(acknum)) {
            // ....
            continue;
        }
    }
    return NULL;
}
