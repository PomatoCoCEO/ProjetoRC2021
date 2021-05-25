#ifndef SERVER_H
#define SERVER_H


#define SERVER_IP "10.90.0.2" // "129.136.212.251"
#define BUF_SIZE 1024
#define CONFIG_FILE "data/users.dat"

#include "server_structs.h"

/*
typedef struct {
    struct sockaddr_in ip_address;
    char msg[256];
    unsigned char online:1;
}client_info;
*/




// #define SERVER_UDP_PORT 9001
// #define SERVER_TCP_PORT 9001
#include "server_c2s.h"
#include "server_p2p.h"
#include "server_group.h"



void write_info_to_file();
void get_info_from_file();
void get_shared_memory();
void* receive_admin(void* args);
int remove_from_list(char * name);
int process_command(int admin_fd_socket, char instruction[]);
void print_user_info(int fd, user_info* user);
void wrong_command(int fd, char *error_msg);
void error_msg(char *msg);

#endif
