#ifndef SERVER_STRUCTS_H
#define SERVER_STRUCTS_H

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


#define NO_MAX_USERS 10
#define NO_MAX_GROUPS 20

typedef struct
{
    char userId[256];
    char password[256];
    struct sockaddr_in ip_address;
    unsigned char permissions : 3; // 0 0 0 -> 1 1 1
    unsigned char online:1;
    pthread_t thread;
} user_info;

typedef struct {
    struct sockaddr_in ip_address;
    char name[256];
} group_info;

typedef struct
{
    user_info users[NO_MAX_USERS];
    int no_users;
    group_info groups[NO_MAX_GROUPS];
    int no_groups;
} server_info_t;

typedef struct {
    long userNo;
    char msg[256];
} msg_t;

void receive_message(msg_t* msg, int pos);
int find_user(msg_t* msg);

#endif