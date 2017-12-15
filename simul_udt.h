#ifndef	SIMUL_UDT_H
#define SIMUL_UDT_H


#include <sys/types.h>
#include <sys/socket.h>


ssize_t udt_sendto(int sockfd, const void *buf, size_t len, 
				 const struct sockaddr *addr, socklen_t addrlen,
                 double loss);
ssize_t udt_send(int sockfd, void *buf, size_t size, double loss);


#endif /* SIMUL_UDT_H */
