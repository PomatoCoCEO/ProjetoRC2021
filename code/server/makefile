CC = gcc
FLAGS = -Wall
PROG = server
OBJS = admin.o server.o


all: ${PROG}


clean:
	rm ${OBJS} ${PROG}


${PROG}: ${OBJS}
	${CC} ${FLAGS} ${OBJS} -lm -o $@

.c.o:
	${CC} ${FLAGS} $< -c -o $@

##########################

admin.o: admin.c server.h

server.o: server.c server.h

server: server.o admin.o