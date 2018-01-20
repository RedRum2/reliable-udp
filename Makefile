CC = gcc
CFLAGS = -Wall -Wextra -pthread -O2
SRC = $(shell ls *.c)
OBJ = $(SRC:.c=.o)

all: $(OBJ) 
	${CC} ${CFLAGS} client.o rw.o clicmd.o cmd_commons.o transport.o simul_udt.o event.o window.o bit_array.o adaptive.o cb_utils.o timespec_utils.o -o client queue.o
	${CC} ${CFLAGS} server.o strto.o rw.o srvcmd.o cmd_commons.o transport.o simul_udt.o event.o window.o bit_array.o adaptive.o cb_utils.o timespec_utils.o -o server queue.o


client.o: rw.h clicmd.h simul_udt.h transport.h

server.o: rw.h srvcmd.h simul_udt.h strto.h transport.h

rw.o: rw.h

strto.o: strto.h

cmd_commons.o: cmd_commons.h rw.h transport.h 

queue.o: queue.h 

clicmd.o: clicmd.h cmd_commons.h transport.h

srvcmd.o: srvcmd.h cmd_commons.h transport.h

transport.o: transport.h rw.h simul_udt.h event.h window.h adaptive.h queue.h cb_utils.h timespec_utils.h

simul_udt.o: simul_udt.h

event.o: event.h

window.o: window.h bit_array.h

adaptive.o: adaptive.h timespec_utils.h

bit_array.o: bit_array.h

cb_utils.o: cb_utils.h

timespec_utils.o: timespec_utils.h

clean:
	rm -f *.o core 

cleanall:
	rm -f *.o core client server
