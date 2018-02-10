void rdt_send(const void *buf, size_t len)
{
    size_t free, tosend, left = len;

    while (left) {

        if (pthread_mutex_lock(&send_cb.mtx) != 0)
            handle_error("pthread_mutex_lock");

        /* check available space */
        while ((free =
                space_available(send_cb.S, send_cb.E, CBUF_SIZE)) <= MSS)
            if (pthread_cond_wait(&send_cb.cnd_not_full, &send_cb.mtx) !=
                0)
                handle_error("pthread_cond_wait");

        /* calculate how much data to send */
        tosend = free > left ? left : (free / MSS) * MSS;

        memcpy_tocb(send_cb.buf, buf + len - left, tosend, send_cb.E,
                    CBUF_SIZE);

        send_cb.E = (send_cb.E + tosend) % CBUF_SIZE;

        if (pthread_mutex_unlock(&send_cb.mtx) != 0)
            handle_error("pthread_mutex_unlock");

        if (cond_event_signal(&e, PKT_EVENT) == -1)
            handle_error("cond_event_signal()");

        left -= tosend;
    }
}
