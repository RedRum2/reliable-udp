void sig_zombie_handler(int sig)
{
    pid_t pid;
    (void) sig;

    while ((pid = waitpid(-1, NULL, WNOHANG)) > 0)
        ;
    return;
}

void register_zombie_handler(void)
{
    struct sigaction sa;

    sa.sa_handler = sig_zombie_handler;
    // make certain system calls restartable across signals
    sa.sa_flags = SA_RESTART;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGCHLD, &sa, NULL) == -1)
        handle_error("sigaction()");
}

int main(int argc, char **argv)
{
    /* create listen socket ..... */
    /* set server address ..... */
    /* bind the socket to the address ..... */

    /* register SIGCHLD signal handler */
    register_zombie_handler();

    for (;;) {
        /* wait for connection requests ..... */
        /* create a new process to handle client's requests ..... */
    }
}
