#include "server.h"

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


int verify_userId(msg_t* msg, struct sockaddr_in* client_ip) {
    int i;
    char* buf = msg->msg;
    for(i = 0; i<shmem->no_users; i++) {
        if(strcmp(shmem->users[i].userId, buf)==0) {
            if(shmem->users[i].ip_address.sin_addr.s_addr == client_ip->sin_addr.s_addr) {
                shmem->users[i].ip_address = *client_ip;
                return i;
            }
            return -1;
        }
    }
    return -2;
} 

void process_client(int pos, struct sockaddr_in client_ip,int sock_fd) {
    // char password[];
    // ! create new socket for this client
    msg_t password, ans;
    struct sockaddr_in cp = client_ip;
    socklen_t slen = sizeof(cp);
    // if(recvfrom(sock_fd,&password, sizeof(password),&cp, &slen)==-1) {
    //     error_msg("Problems receiving password");
    // }
    if(msgrcv(msqid, &password, sizeof(password)-sizeof(long), pos+1, 0) == -1) {
        error_msg("Erro na receção da message queue ");
    }

    if(strcmp(password.msg, shmem->users[pos].password)!=0) {
        msg_t password_incorrect;
        sprintf(password_incorrect.msg, "PASSWORD INCORRECT");
        sendto(sock_fd, &password_incorrect, sizeof(password_incorrect),0, (struct sockaddr*) &(shmem->users[pos].ip_address), slen);
        close(sock_fd);
        return;
    }
    int perm = (int) shmem->users[pos].permissions;
    sprintf(ans.msg, "%d\n",perm);
    sendto(sock_udp, &ans, sizeof(ans),0,(struct sockaddr *) &(shmem->users[pos].ip_address), slen);
    while(1){
        // "P2P" "C2S" "GROUP"
        msg_t type;
        if(msgrcv(msqid, &type, sizeof(type)-sizeof(long), pos+1, 0) == -1) {
            error_msg("Problems receiving password");
        }
        // assuming client verifies whether type is allowed
        
    }
}

int main(int argc, char **argv)
{
    struct sockaddr_in si_server, si_client;
    socklen_t slen = sizeof(si_client);
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
    if((msqid = msgget(IPC_PRIVATE, IPC_CREAT|0766))<0) {
        error_msg("Erro na criação da message queue ");
        // cleanup();
        exit(1);
    }
    // Cria um socket para recepção de pacotes UDP
    if ((sock_udp = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
        error_msg("Erro na criação do socket");

    // Preenchimento da socket address structure
    si_server.sin_family = AF_INET;
    si_server.sin_port = htons(client_port);
    inet_pton(AF_INET, SERVER_IP, &(si_server.sin_addr));

    // Associa o socket à informação de endereço
    if (bind(sock_udp, (struct sockaddr *)&si_server, sizeof(si_server)) == -1)
        error_msg("Erro no bind");
    
    while (1) {
        // char buf[256];
        msg_t buf, ans;
        if(recvfrom(sock_udp, &buf, sizeof(msg_t),0,(struct sockaddr *) &si_client, &slen) == -1) {
		    error_msg("Erro no recvfrom");
		    exit(1);
		}
        if(buf.userNo !=0) {
            if(msgsnd(msqid, &buf, sizeof(buf)-sizeof(long), 0) == -1) {
                error_msg("Problemas no envio pela message queue: ");
            }
        }
        else{
            int ret = verify_userId(&buf, &si_client);
            
            switch(ret) {
                case -1:
                    sprintf(ans.msg, "IP INCORRECT");
                    sendto(sock_udp, &ans, sizeof(ans),0,(struct sockaddr *) &si_client, slen);
                    break;
                case -2:
                    sprintf(ans.msg, "NONEXISTENT USER");
                    sendto(sock_udp, &ans, sizeof(ans),0,(struct sockaddr *) &si_client, slen);
                    break;
                default:
                    sprintf(ans.msg, "OK");
                    ans.userNo = ret+1; // beware of ZERO
                    sendto(sock_udp, &ans, sizeof(ans), 0, (struct sockaddr *)&si_client, slen);
                    // maybe save pid
                    if(fork()==0){
                        // deal_with_client
                        process_client(ret, si_client, sock_udp);
                        exit(0);
                    } 
                    break;
            }
            // Para ignorar o restante conteúdo (anterior do buffer)
            

            // sleep(1);
        }
    }


    return 0;
}



