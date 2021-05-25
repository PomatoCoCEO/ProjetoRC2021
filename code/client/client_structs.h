#ifndef CLIENT_STRUCTS_H
#define CLIENT_STRUCTS_H

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
#include <sys/ipc.h>
#include <sys/msg.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>

typedef struct {
    long userNo;
    char msg[256];
} msg_t;

typedef struct {
    struct sockaddr_in ip_address;
    pthread_t thread;
    int sock_fd;
    char name[256];
} group_t;

typedef struct {
    group_t group[10];
    int no_groups; 
} group_array;

int sock_fd, allowed, no_user;
short local_port_no;
struct in_addr localInter;
msg_t msg;
struct sockaddr_in si_server, group_sock;
group_array groups;
char username[256], local_ip[20];
struct ip_mreq group;
socklen_t slen;//  = sizeof(si_server);


int receive_msg();
void error_msg(char *msg);

#endif