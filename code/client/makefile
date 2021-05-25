CC = gcc
FLAGS = -Wall -pthread
PROG = client
OBJS = client_c2s.o client_group.o client_p2p.o client.o 


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


#client.o: client.c client_c2s.c client_p2p.c client_group.c

#client: client.o