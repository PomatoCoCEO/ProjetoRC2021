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
        
        shmem->users[i].online=0;
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

void send_msg_to_user(int pos, int pos_dest, msg_t * msg) {
    // pos está correto
    msg_t buf;
    //bzero(buf, sizeof(buf));
    sprintf(buf.msg, "NEW MESSAGE FROM %s: %s",shmem->users[pos].userId, msg->msg);
    buf.userNo= pos_dest+1;
    sendto(sock_udp, &buf, sizeof(msg_t),0,(struct sockaddr *) &(shmem->users[pos_dest].ip_address), slen);
}

int verify_userId(msg_t* msg, struct sockaddr_in* client_ip) {
    int i;
    char* buf = msg->msg;
    #ifdef DEBUG_LOGIN
    printf("Username received: \"%s\"\n", buf);
    #endif
    for(i = 0; i<shmem->no_users; i++) {
        #ifdef DEBUG_LOGIN
	    printf("Comparing against \"%s\"\n", shmem->users[i].userId);
        #endif
        if(strcmp(shmem->users[i].userId, buf)==0) {
            if(shmem->users[i].ip_address.sin_addr.s_addr == client_ip->sin_addr.s_addr) {
                shmem->users[i].ip_address = *client_ip;
                #ifdef DEBUG_LOGIN
		        printf("Returning %d...\n", i);
                #endif
                return i;
            }
            return -1;
        }
    }
    return -2;
} 

int find_user(msg_t* msg) {
    for(int i = 0; i<shmem->no_users; i++) {
        if(strcmp(shmem->users[i].userId, msg->msg)==0) {
            return i;
        }
    }
    return -1;
}

void handle_c2s(int pos, msg_t* ans, int sock_fd) {
    sprintf(ans->msg, "DESTINATION USER");
    ans->userNo = pos+1;
    sendto(sock_udp, ans, sizeof(msg_t),0,(struct sockaddr *) &(shmem->users[pos].ip_address), slen);

    if(msgrcv(msqid, ans, sizeof(msg_t)-sizeof(long), pos+1, 0) == -1) { // it's a pointer
        error_msg("Erro na receção da message queue ");
        return;
    }
    // processar -> int find_user
    int pos_dest = find_user(ans);
    printf("pos_dest = %d\n", pos_dest);
    if(pos_dest == -1) {
        sprintf(ans->msg, "USER NOT FOUND");
        sendto(sock_udp, ans, sizeof(msg_t),0,(struct sockaddr *) &(shmem->users[pos].ip_address), slen);
        return;
    }else{
        if(shmem->users[pos_dest].online ==1)
        {
            sprintf(ans->msg, "ENTER MESSAGE");
            sendto(sock_udp, ans, sizeof(msg_t),0,(struct sockaddr *) &(shmem->users[pos].ip_address), slen);
            if(msgrcv(msqid, ans, sizeof(msg_t)-sizeof(long), pos+1, 0) == -1) {
                error_msg("Erro na receção da message queue da mensagem");
                return;
            }

            send_msg_to_user(pos, pos_dest, ans ); // NEW MESSAGE from paulocorte: (...)
            sprintf(ans->msg, "MESSAGE SENT");
            sendto(sock_udp, ans, sizeof(msg_t),0,(struct sockaddr *) &(shmem->users[pos].ip_address), slen);
        } else{
            sprintf(ans->msg, "USER OFFLINE");
            sendto(sock_udp, ans, sizeof(msg_t),0,(struct sockaddr *) &(shmem->users[pos].ip_address), slen);
            return;
        }
    }
        
}


void handle_p2p(int pos, msg_t* ans, int sock_fd){
    sprintf(ans->msg, "DESTINATION USER");
    sendto(sock_udp, ans, sizeof(msg_t),0,(struct sockaddr *) &(shmem->users[pos].ip_address), slen);

    if(msgrcv(msqid, ans, sizeof(msg_t)-sizeof(long), pos+1, 0) == -1) {
        error_msg("Erro na receção da message queue ");
        return;
    }
    // processar -> int find_user
    int pos_dest = find_user(ans);
    printf("pos_dest = %d\n", pos_dest);
    if(pos_dest == -1) {
        sprintf(ans->msg, "USER NOT FOUND");
        sendto(sock_udp, ans, sizeof(msg_t),0,(struct sockaddr *) &(shmem->users[pos].ip_address), slen);
        return;
    }else{
        if(shmem->users[pos_dest].online==1)
        {
            // char buf[256];
            msg_t msg;
            msg.userNo=pos+1;
            char ip[20];
            inet_ntop(AF_INET, &(shmem->users[pos_dest].ip_address.sin_addr),ip, INET_ADDRSTRLEN);
            sprintf(msg.msg, "IP_ADDRESS %s:%d",ip,shmem->users[pos_dest].ip_address.sin_port);
            sendto(sock_udp, &msg, sizeof(msg_t),0,(struct sockaddr *) &(shmem->users[pos].ip_address), slen);
            
        } else{
            sprintf(ans->msg, "USER OFFLINE");
            sendto(sock_udp, ans, sizeof(msg_t),0,(struct sockaddr *) &(shmem->users[pos].ip_address), slen);
            return;
        }
    }
}

void cleanup(int signo) {
    while(wait(NULL)!=-1);
    close(sock_udp);
    exit(0);
}

void process_client(int pos, struct sockaddr_in client_ip,int sock_fd) {
    // char password[];
    // ! create new socket for this client
    msg_t password, ans;
    struct sockaddr_in cp = client_ip;
    //socklen_t slen = sizeof(cp);
    // if(recvfrom(sock_fd,&password, sizeof(password),&cp, &slen)==-1) {
    //     error_msg("Problems receiving password");
    // }
    if(msgrcv(msqid, &password, sizeof(password)-sizeof(long), pos+1, 0) == -1) {
        error_msg("Erro na receção da message queue ");
    }

    if(strcmp(password.msg, shmem->users[pos].password)!=0) {
        msg_t password_incorrect;
        password_incorrect.userNo=pos+1;
        sprintf(password_incorrect.msg, "PASSWORD INCORRECT");
        sendto(sock_fd, &password_incorrect, sizeof(password_incorrect),0, (struct sockaddr*) &(shmem->users[pos].ip_address), slen);
        close(sock_fd);
        return;
    }
    shmem->users[pos].online=1;
    int perm = (int) shmem->users[pos].permissions;
    sprintf(ans.msg, "%d\n",perm);
    ans.userNo=pos+1;
    sendto(sock_udp, &ans, sizeof(msg_t),0,(struct sockaddr *) &(shmem->users[pos].ip_address), slen);
    while(1){
        // "P2P" "C2S" "GROUP"
        msg_t type;
        if(msgrcv(msqid, &type, sizeof(type)-sizeof(long), pos+1, 0) == -1) {
            error_msg("Outra coisa qualquer");
        }
    
        if(strcmp(type.msg, "P2P")==0) {
            handle_p2p(pos, &ans, sock_fd);
        }
        else if(strcmp(type.msg, "C2S")==0) {
            // client to server
            handle_c2s(pos, &ans, sock_fd);
            
        } else if(strcmp(type.msg, "GROUP")==0) { // MULTICASTING
            
        }else{
            // erro
        }
        

        // assuming client verifies whether type is allowed
        
    }
}

int main(int argc, char **argv)
{
    signal(SIGINT, cleanup);
    struct sockaddr_in si_server, si_client;
    slen = sizeof(si_client);
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
		printf("Numero da mensagem: %ld\n", buf.userNo); 
            if(msgsnd(msqid, &buf, sizeof(buf)-sizeof(long), 0) == -1) {
                error_msg("Problemas no envio pela message queue: ");
            }
        }
        else{
            int ret = verify_userId(&buf, &si_client);
            printf("Value returned:%d\n", ret); 
            ans.userNo=ret;
            buf.userNo = ret;
            switch(ret) {
                case -1:
                    sprintf(ans.msg, "IP INCORRECT");
                    sendto(sock_udp, &ans, sizeof(msg_t),0,(struct sockaddr *) &si_client, slen);
                    break;
                case -2:
                    sprintf(ans.msg, "NONEXISTENT USER");
                    sendto(sock_udp, &ans, sizeof(msg_t),0,(struct sockaddr *) &si_client, slen);
                    break;
                default:
                    sprintf(ans.msg, "OK");
                    ans.userNo = ret+1; // beware of ZERO
                    sendto(sock_udp, &ans, sizeof(msg_t), 0, (struct sockaddr *)&si_client, slen);
                    // maybe save pid
                    if(fork()==0){
                        // deal_with_client
                        process_client(ret, si_client, sock_udp);
                        exit(0);
                    } 
                    break;
            }
            // Para ignorar o restante conteúdo (anterior do buffer)
            

            
        }
    }
    return 0;
}


