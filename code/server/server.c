
#include "admin.h"
#include "server.h"

// #define SERVER_UDP_PORT 9001
// #define SERVER_TCP_PORT 9001
#define SERVER_IP "10.0.2.15"// "129.136.212.251"
#define BUF_SIZE 1024
#define NO_MAX_USERS 10
#define CONFIG_FILE "data/users.dat"
int client_port, config_port;



void write_info_to_file();
void get_info_from_file();
void get_shared_memory();
void receive_admin();
int remove_from_list(char * name);
int process_command(int admin_fd_socket, char instruction[]);
void print_user_info(int fd, user_info* user);
void wrong_command(int fd, char *error_msg);
void error_msg(char *msg);






void error_msg(char *msg)
{
    fprintf(stderr, "Erro: %s -> %s\n", msg, strerror(errno));
    exit(1);
}

void wrong_command(int fd, char *error_msg) // answer to client 
{
    write(fd, error_msg, 1 + strlen(error_msg));
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
        fprintf(stderr, "Usage: ./server <client-port> <config-port> <configFile>\n");
        return 1;
    }
    
    char configFile[256];
    client_port = atoi(argv[1]);
    config_port = atoi(argv[2]);
    strcpy(configFile, argv[3]);
    if ((admin_pid = fork()) == 0)
    {
        receive_admin(); // aqui n será melhor while(1) receive_admin(); ??
        exit(0);
    }
    // wait for client connections -> INITIALISE UDP socket
    // TODO udp connection

    // Cria um socket para recepção de pacotes UDP
    if ((sock_udp = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
        error_msg("Erro na criação do socket");

    // Preenchimento da socket address structure
    si_server.sin_family = AF_INET;
    si_server.sin_port = htons(client_port);
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