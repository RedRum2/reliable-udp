void rdt_recv(void *buf, size_t len)
{
    size_t data, toread, left = len;

    while (left) {

        if (pthread_mutex_lock(&recv_cb.mtx) != 0)
            handle_error("pthread_mutex_lock");

        while (recv_cb.S == recv_cb.E)
            /* circular buffer is empty */
            if (pthread_cond_wait(&recv_cb.cnd_not_empty, &recv_cb.mtx) !=
                0)
                handle_error("pthread_cond_wait");

        /* circular buffer not empty */
        data = data_available(recv_cb.S, recv_cb.E, CBUF_SIZE);
        toread = data < left ? data : left;
        memcpy_fromcb(buf + len - left, recv_cb.buf, toread, recv_cb.S,
                      CBUF_SIZE);
        recv_cb.S = (recv_cb.S + toread) % CBUF_SIZE;

        if (pthread_cond_signal(&recv_cb.cnd_not_full) != 0)
            handle_error("pthread_cond_signal");

        if (pthread_mutex_unlock(&recv_cb.mtx) != 0)
            handle_error("pthread_mutex_unlock");

        left -= toread;
    }
}
