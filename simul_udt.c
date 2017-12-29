#include "simul_udt.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>



/*
 * Function:	randgen
 * -------------------------------------
 * Generate a random value between 0 and 1.
 * The seed is set only at the first call of the function
 * in order to avoid same values extractions.
 *
 * Returns:
 * 		the random generated value.
 */
double randgen(void)
{
    static bool seed = false;
    if (!seed) {
        srand48(time(NULL));
        seed = true;
    }
    return drand48();
}



/*
 * Function:	udt_sendto
 * --------------------------------------------
 * Send data to destination address if the random generated number is
 * greater than the loss probability.
 *
 * Parameters:
 * 		sockfd:		socket file descriptor
 * 		buf			pointer to data
 * 		size		size in bytes of the data to send 
 * 		addr:		server address
 *		addrlen:	size of the server address structure
 *		loss:		segment loss probability
 *
 * Returns:
 * 		the number of bytes sent on success
 * 		-1 on error
 */
ssize_t udt_sendto(int sockfd, const void *buf, size_t len,
                   const struct sockaddr * addr, socklen_t addrlen,
                   double loss)
{
    double drand = randgen();
    ssize_t retval = len;

    // necessary flow control into a local network
    if (loss < 0.1)
        usleep(50);

    if (drand > loss) {
        retval = sendto(sockfd, buf, len, 0, addr, addrlen);
        fputs("frame sent\n", stderr);
    } else
        fputs("frame lost\n", stderr);

    return retval;
}




/*
 * Function:	udt_send
 * --------------------------------------
 * Write data to the socket if the random genarated number is
 * greater than the loss probability.
 *
 * Parameters:
 * 		sockfd		socket file descriptor
 * 		buf			pointer to data
 * 		size		size in bytes of the data to write
 * 		loss		loss probability
 *
 * Returns:
 * 		the number of bytes sent on success
 * 		-1 on error
 */
ssize_t udt_send(int sockfd, void *buf, size_t size, double loss)
{
    return udt_sendto(sockfd, buf, size, NULL, 0, loss);
}
