CC = gcc
FLAGS = -Wall -pthread
PROG = client
OBJS = client.o


all: ${PROG}


clean:
	rm ${OBJS} ${PROG}


${PROG}: ${OBJS}
	${CC} ${FLAGS} ${OBJS} -lm -o $@

.c.o:
	${CC} ${FLAGS} $< -c -o $@

##########################


client.o: client.c  client.h  

client: client.o