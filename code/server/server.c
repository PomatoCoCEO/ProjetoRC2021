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

#define SERVER_UDP_PORT 9000
#define SERVER_TCP_PORT 9000
#define SERVER_IP "129.136.212.251"
#define BUF_SIZE 1024
#define NO_MAX_USERS 10
#define CONFIG_FILE "data/users.dat"

typedef struct
{
    char userId[256];
    char password[256];
    // char ip[16];
    struct sockaddr_in ip_address;
    // unsigned int ip_addr;
    unsigned char permissions : 3; // 0 0 0 -> 1 1 1
} user_info;

typedef struct
{
    user_info users[NO_MAX_USERS];
    int no_users;

} shm_t;


void write_info_to_file();
void get_info_from_file();
void get_shared_memory();
void receive_admin();
void read_commands(int admin_socket_fd);
int remove_from_list(char * name);
int process_command(int admin_fd_socket, char instruction[]);
void print_user_info(int fd, user_info* user);
void wrong_command(int fd, char *error_msg);
void error_msg(char *msg);




pid_t admin_pid;
int sock_udp;
shm_t *shmem;


void error_msg(char *msg)
{
    fprintf(stderr, "Erro: %s -> %s\n", msg, strerror(errno));
    exit(1);
}

void wrong_command(int fd, char *error_msg) // answer to client 
{
    write(fd, error_msg, 1 + strlen(error_msg));
}

void print_user_info(int fd, user_info* user) {
    const char* aid[2]={"No", "Yes"};
    char ip_addr[16];
    char msg[512];
    inet_ntop(AF_INET, &(user->ip_address.sin_addr),ip_addr, INET_ADDRSTRLEN);
    char perm[3];
    for(int i = 2; i>=0; i--) {
        if(user->permissions & (1<<i) ) {
            perm[i]=1;
        }
        else perm[i]=0;
    }
    sprintf(msg,"%s %s %s %s %s %s\n", user->userId, ip_addr, user->password, aid[perm[2]], aid[perm[1]], aid[perm[0]]);
    write (fd, msg, strlen(msg));
}

int process_command(int admin_fd_socket, char instruction[])
{
    char* command;
    // memset(command, 0, 256);
    command = strtok(instruction, " \r\n");
    printf("Command: \"%s\"\n", command);
    if (strcmp(command, "LIST") == 0)
    {
        char msg[49];
        msg[48]=0;
        sprintf(msg, "User-id IP Password Cliente-Servidor P2P Grupo\n");
        write(admin_fd_socket, msg, strlen(msg));
        for(int i = 0; i<shmem->no_users; i++) {
            print_user_info(admin_fd_socket, &(shmem->users[i]));
        }
        return 0;
    }
    else if (strcmp(command, "ADD") == 0)
    {
        // * ADD <User-id> <IP> <Password> <Cliente-Servidor> <P2P> <Grupo>
        user_info new;
        char *next_token = strtok(NULL, " \r\n");
        if (next_token == NULL)
        {
            //wrong command
            wrong_command(admin_fd_socket,"Use ADD <User-id> <IP> <Password> <Cliente-Servidor> <P2P> <Grupo> ");
            return 2;
        }
        strcpy(new.userId, next_token); // ID
        next_token = strtok(NULL, " \r\n");
        if (next_token == NULL) // IP
        {
            //wrong command
            wrong_command(admin_fd_socket,"Use ADD <User-id> <IP> <Password> <Cliente-Servidor> <P2P> <Grupo> ");
            return 2;
        }
        inet_pton(AF_INET, next_token, &(new.ip_address.sin_addr));
        new.ip_address.sin_family = AF_INET;
        //strcpy(new.ip, next_token); // ip
        next_token = strtok(NULL, " \r\n");
        if (next_token == NULL)
        {
            //wrong command
            wrong_command(admin_fd_socket,"Use ADD <User-id> <IP> <Password> <Cliente-Servidor> <P2P> <Grupo> ");
            return 2;
        }
        strcpy(new.password, next_token);
        unsigned char perm = 0;
        for (int i = 2; i >= 0; i--)
        {
            next_token = strtok(NULL, " \r\n");
            if (next_token == NULL)
            {
                //wrong command
                wrong_command(admin_fd_socket,"Wrond command. Use: ADD <User-id> <IP> <Password> <Cliente-Servidor> <P2P> <Grupo>");
                break;
            }
            if (strcasecmp(next_token, "YES") == 0)
            {
                perm |= (1 << i);
            }
            else if (strcasecmp(next_token, "NO") == 0)
            {
            }
            else
            {
                wrong_command(admin_fd_socket, "Use yes or no for permissions!");
                break;
            }
        }
        new.permissions = perm;
        // write "User added:"
        shmem->users[shmem->no_users++]=new;
        char* msg = "User added successfully:\n";
        write(admin_fd_socket, msg, strlen(msg));
        print_user_info(admin_fd_socket, &new);
        return 0;
    }
    else if (strcmp(command, "DEL") == 0)
    {
        //strtok
        char *next_token = strtok(NULL, " \r\n");
        if (next_token == NULL)
        {
            //wrong command
            wrong_command(admin_fd_socket,"Use ADD <User-id> <IP> <Password> <Cliente-Servidor> <P2P> <Grupo> ");
            return 2;
        }

        if (remove_from_list(next_token)==-1){
            wrong_command(admin_fd_socket, "UderId not found");
        }
        char* msg = "User deleted successfully...\n";
        write(admin_fd_socket, msg, strlen(msg));
        return 0;
    }
    else if (strcmp(command, "QUIT") == 0)
    {
        // leave
        char* msg = "Quitting...\n";
        write(admin_fd_socket, msg, strlen(msg));
        return 1;
    }
    else
    {
        wrong_command(admin_fd_socket,"Use ADD, LIST, DEL or QUIT!");
        //wrong command
        return 0;
    }
}

int remove_from_list(char * name){

    for (int i=0; i< shmem->no_users; ++i){
        if (strcasecmp( name, shmem->users[i].userId)==0){
            shmem->users[i] = shmem->users[--shmem->no_users];
            bzero(&(shmem->users[shmem->no_users]), sizeof(user_info));
            return 0;
        }
    }
    return -1;
    
}

void read_commands(int admin_socket_fd)
{
    int n = 0;
    char buf[BUF_SIZE];
    // load info to shared memory from file
    while (1)
    {
        n = read(admin_socket_fd, buf, BUF_SIZE - 1);
        if (n < 0)
        {
            perror("Problems reading from socket: ");
        }
    }
}



void receive_admin()
{

    struct sockaddr_in server_addr, admin_addr;
    size_t admin_addr_size;
    //  signal(SIGINT, cleanup);

    bzero((void *)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    inet_pton(AF_INET, SERVER_IP, &(server_addr.sin_addr));
    server_addr.sin_port = htons(SERVER_TCP_PORT);

    // for reading administration commands -> INITIALISE TCP SOCKET
    int admin_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (admin_fd < 0)
        error_msg("Problems obtaining TCP socket for admin!\n");

    if (bind(admin_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
        error_msg("na funcao bind");
    //only one admin
    if (listen(admin_fd, 1) < 0)
        error_msg("na funcao listen");
    admin_addr_size = sizeof(admin_addr);


    //char ip_server_tcp[INET_ADDRSTRLEN];

    // ! inet_ntop(AF_INET, &((amdin_addr.sin_addr)), ip, INET_ADDRSTRLEN);
    printf("Server accepting connections in (IP:port) %s:%d\n", SERVER_IP, SERVER_TCP_PORT);

    // while (1) // supõe-se que
    int ret;
    do {
        int admin_socket_fd;

        while (waitpid(-1, NULL, WNOHANG) > 0)
            ;

        admin_socket_fd = accept(admin_fd, (struct sockaddr *)&(admin_addr), (socklen_t *)&admin_addr_size);
        char buffer[256];
        int g = read(admin_socket_fd, buffer, 256);
        buffer[g]='\0';
        ret = process_command(admin_socket_fd, buffer);
    } while(ret !=1);

    // accept commands
}

void get_info_from_file(){
    
    FILE * config= fopen (CONFIG_FILE, "r");
    size_t sz = 256;
    char* ptr = (char*)malloc(256*sizeof(char));
    int i = 0;
    while(getline(&ptr, &sz, config)!=-1) {
        strtok(ptr, "\n");
        user_info user;
        char ip_aid[16]; bzero(ip_aid, sizeof(ip_aid));
        int perm;
        if(sscanf(ptr, "%s %s %s %d", user.userId, ip_aid,user.password, &perm)!=4) {
            fprintf(stderr, "Line \"%s\" badly formatted!\n",ptr);
        }
        if(perm>=0 && perm<8) user.permissions=perm;
        else{
            fprintf(stderr, "Line %d: Bad permission number! Use 0 to 7! \n",i+1);
        }
        if(inet_pton(AF_INET, ip_aid, &(user.ip_address.sin_addr))==0) {
            fprintf(stderr, "IP address \"%s\"not valid!\n",ip_aid);
        }
        if(i==NO_MAX_USERS) {
            fprintf(stderr, "Too many users to account for!\n");
            break;
        }
        shmem->users[i++]=user;
        
    }
    shmem->no_users=i;
    fclose(config);
}

void write_info_to_file(){
    FILE * config= fopen (CONFIG_FILE, "w");
    char ip[16];
    for (int i=0; i< shmem->no_users; ++i){
        inet_ntop( AF_INET,&((shmem->users[i].ip_address.sin_addr)), ip, INET_ADDRSTRLEN );
        fprintf(config, "%s %s %s %d\n", shmem->users[i].userId, ip, shmem->users[i].password, (int) shmem->users[i].permissions );
    }
    fclose(config);
}

void get_shared_memory()
{ // shared memory to go between processes
    int shmid = shmget(IPC_PRIVATE, sizeof(shm_t), IPC_CREAT | IPC_EXCL | 0766);
    if (shmid == -1)
    {
        error_msg("Problems obtaining shared memory");
    }
    if ((shmem = shmat(shmid, NULL, 0)) == (shm_t *)-1)
    {
        error_msg("Problems attaching shared memory");
    }
}


void cleanup(){


    write_info_to_file();
}

int main(int argc, char **argv)
{
    struct sockaddr_in si_server;
    printf("Boo\n");
    get_shared_memory();
    printf("Boo\n");
    get_info_from_file();
    printf("Boo\n");
    if(argc!=4) {
        fprintf(stderr, "Usage: <client-port> <config-port> <configFile>\n");
        return 1;
    }
    int client_port, config_port;
    char configFile[256];
    client_port = atoi(argv[1]);
    config_port = atoi(argv[2]);
    strcpy(configFile, argv[3]);
    if ((admin_pid = fork()) == 0)
    {
        receive_admin();
        exit(0);
    }
    // wait for client connections -> INITIALISE UDP socket
    // TODO udp connection

    // Cria um socket para recepção de pacotes UDP
    if ((sock_udp = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
        error_msg("Erro na criação do socket");

    // Preenchimento da socket address structure
    si_server.sin_family = AF_INET;
    si_server.sin_port = htons(SERVER_UDP_PORT);
    si_server.sin_addr.s_addr = htonl(INADDR_ANY);

    // Associa o socket à informação de endereço
    if (bind(sock_udp, (struct sockaddr *)&si_server, sizeof(si_server)) == -1)
        error_msg("Erro no bind");

    while (1)
    {
        // ? receive client stuff
    }

    write_info_to_file();

    return 0;
}