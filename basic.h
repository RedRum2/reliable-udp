#ifndef _BASIC_H
#define _BASIC_H


#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>

#define SERVER_PORT	5193
#define MAXLINE		1024
#define MAX_BUFSIZE 4096	

// command codes
#define LIST 		0
#define GET 		1
#define PUT 		2
#define MAXCMD 		2

// response codes
#define GET_OK 		0
#define GET_NOENT 	1
#define PUT_SUCCESS 2
#define PUT_FAILURE 3

#define handle_error(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)


struct proto_params {
	uint16_t T;
	uint8_t  P;
	uint8_t  N;
	uint8_t  adaptive;
};


#endif /* BASIC_H */
