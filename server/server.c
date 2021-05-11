#include <sys/socket.h>
#include <sys/types.h>
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

#define SERVER_UDP_PORT 9000
#define SERVER_TCP_PORT 9000
#define SERVER_IP "129.136.212.251"
#define BUF_SIZE 1024
#define NO_MAX_USERS 10

pid_t admin_pid;
int sock_udp;

typedef struct
{
    char userId[256];
    char password[256];
    int ip_addr;
    unsigned char permissions : 3; // 0 0 0 -> 1 1 1
} user_info;

typedef struct
{
    user_info users[NO_MAX_USERS];
    int no_users;

} shm_t;
smt_t *shmem;

void error_msg(char *msg)
{
    fprintf(stderr, "Erro: %s -> %s\n", msg, strerror(errno));
    exit(1);
}

void wrong_command(int fd, char *error_msg)
{
    write(fd, error_msg, 1 + strlen(error_msg));
}

void process_command(char instruction[], int admin_fd_socket)
{
    char *command;
    memset(command, 0, 30);
    command = strtok(instruction, " \r\n");
    printf("Command: \"%s\"\n", command);
    if (strcmp(command, "LIST") == 0)
    {
        // tem lista em memória partilhada
        // listar todas
    }
    else if (strcmp(command, "ADD") == 0)
    {
        // * ADD <User-id> <IP> <Password> <Cliente-Servidor> <P2P> <Grupo>:
        //STRTOK
        user_info new;
        char *next_token = strtok(NULL, " \r\n");
        if (next_token == NULL)
        {
            //wrong command
            break;
        }
        strcpy(new.userId, next_token);
        char *next_token = strtok(NULL, " \r\n");
        if (next_token == NULL)
        {
            //wrong command
            break;
        }
        strcpy(new.password, next_token);
        unsigned char perm = 0;
        for (int i = 2; i >= 0; i--)
        {
            char *next_token = strtok(NULL, " \r\n");
            if (next_token == NULL)
            {
                //wrong command
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
                //wrong command
            }
        }
        new.permissions = perm;
    }
    else if (strcmp(command, "DEL") == 0)
    {
        //strtok
    }
    else if (strcmp(command, "QUIT") == 0)
    {
        // leave
    }
    else
    {

        //wrong command
    }
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
    if (admin < 0)
        error_msg("Problems obtaining TCP socket for admin!\n");

    if (bind(admin_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
        error_msg("na funcao bind");
    //only one admin
    if (listen(admin_fd, 1) < 0)
        error_msg("na funcao listen");
    admin_addr_size = sizeof(admin_addr);

    //char ip_server_tcp[INET_ADDRSTRLEN];

    // ! inet_ntop(AF_INET, &((amdin_addr.sin_addr)), ip, INET_ADDRSTRLEN);
    printf("Server accepting connections in (IP:port) %s:%d\n", SERVER_IP, SERVER_PORT);

    while (1) // supõe-se que
    {
        int admin_socket_fd;

        while (waitpid(-1, NULL, WNOHANG) > 0)
            ;

        admin_socket_fd = accept(admin_fd, (struct sockaddr *)&(admin_addr), (socklen_t *)&admin_addr_size);

        read_commands(admin_socket_fd);
    }
    int sz;

    // accept commands
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

int main(int argc, char **argv)
{
    get_shared_memory();
    int client_port, config_port;
    char configFile[256];
    client_port = atoi(argv[1]);
    config_port = atoi(argv[2]);

    read_config_file("") if ((admin_pid = fork()) == 0)
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

    return 0;
}