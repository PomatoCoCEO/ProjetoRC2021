CC = gcc
FLAGS = -Wall -pthread
PROG = server
OBJS = admin.o server_group.o server_p2p.o server_c2s.o server.o 


all: ${PROG}


clean:
	rm ${OBJS} ${PROG}


${PROG}: ${OBJS}
	${CC} ${FLAGS} ${OBJS} -lm -o $@

#.c.o:
#	${CC} ${FLAGS} $< -c -o $@

%.o: %.c
	${CC} ${FLAGS} -c $< 

##########################
