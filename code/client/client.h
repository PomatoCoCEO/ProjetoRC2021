#include <sys/socket.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>

typedef struct {
    long userNo;
    char msg[256];
} msg_t;

int sock_fd, allowed, no_user;
msg_t msg;
struct sockaddr_in si_server;
socklen_t slen = sizeof(si_server);
const char* permTypes[] = {"Client-Server", "P2P" , "GROUP"};




int receive_msg();
int client_to_server();
int peer_to_peer();
int group_comm();