#include "server.h"

pthread_t admin_pid;
int sock_udp, msqid, admin_fd;
server_info_t shinfo;
int client_port, config_port;
socklen_t slen;

void error_msg(char *msg)
{
    fprintf(stderr, "Error: %s -> %s\n", msg, strerror(errno));
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
        
        user.online=0;
        shinfo.users[i++]=user;
        
    }
    shinfo.no_users=i;
    shinfo.no_groups=0;

    fclose(config);
}


int verify_userId(msg_t* msg, struct sockaddr_in* client_ip) {
    int i;
    char* buf = msg->msg;
    #ifdef DEBUG_LOGIN
    printf("Username received: \"%s\"\n", buf);
    #endif
    for(i = 0; i<shinfo.no_users; i++) {
        #ifdef DEBUG_LOGIN
	    printf("Comparing against \"%s\"\n", shinfo.users[i].userId);
        #endif
        if(strcmp(shinfo.users[i].userId, buf)==0) {
            if(shinfo.users[i].ip_address.sin_addr.s_addr == client_ip->sin_addr.s_addr) {
                shinfo.users[i].ip_address = *client_ip;
                #ifdef DEBUG_LOGIN
		        printf("Returning %d...\n", i);
                #endif
                if (shinfo.users[i].online==1)
                    return -3;
                return i;
            }
            return -1;
        }
    }
    return -2;
} 

int find_user(msg_t* msg) {
    for(int i = 0; i<shinfo.no_users; i++) {
        if(strcmp(shinfo.users[i].userId, msg->msg)==0) {
            return i;
        }
    }
    return -1;
}

void receive_message(msg_t* msg, int pos) { // pos e a posição do utilizador na Array: para ter a pos na message queue e preciso adicionar 1
    if(msgrcv(msqid, msg, sizeof(msg_t)-sizeof(long), pos+1, 0) == -1) { 
        error_msg("receiving from message queue ");
        return;
    }
    if(strcmp(msg->msg, "EXIT_ALL") == 0) {
        printf("Handler for user %d exiting...\n", pos);
        shinfo.users[pos].online=0;
        pthread_exit(NULL);
    }
}


void cleanup(int signo) {
    if (close(sock_udp) == -1)
    {
        perror("ERROR: Failed to close sock_udp\n");
        exit(1);
    }
    if (close(admin_fd) == -1)
    {
        perror("ERROR: Failed to close admin_fd\n");
        exit(1);
    }
    if (msgctl(msqid, IPC_RMID, 0) == -1)
    {
        perror("ERROR: Failed to destroy message queue\n");
        exit(1);
    }
    exit(0);
}

void* process_client(void* args) {
    // int pos, struct sockaddr_in client_ip,int sock_fd
    user_info* aid = (user_info*) args;
    int pos = aid-shinfo.users;
    int sock_fd = sock_udp;
    // struct sockaddr_in client_ip = aid->ip_address;
    // char password[];
    // ! create new socket for this client
    msg_t password, ans;
    //socklen_t slen = sizeof(cp);
    // if(recvfrom(sock_fd,&password, sizeof(password),&cp, &slen)==-1) {
    //     error_msg("Problems receiving password");
    // }
    if(msgrcv(msqid, &password, sizeof(password)-sizeof(long), pos+1, 0) == -1) {
        error_msg("Error receiving from message queue ");
    }

    if(strcmp(password.msg, shinfo.users[pos].password)!=0) {
        msg_t password_incorrect;
        password_incorrect.userNo=pos+1;
        sprintf(password_incorrect.msg, "PASSWORD INCORRECT");
        sendto(sock_fd, &password_incorrect, sizeof(password_incorrect),0, (struct sockaddr*) &(shinfo.users[pos].ip_address), slen);
        close(sock_fd);
        pthread_exit(NULL);
    }
    shinfo.users[pos].online=1;
    int perm = (int) shinfo.users[pos].permissions;
    sprintf(ans.msg, "%d\n",perm);
    ans.userNo=pos+1;
    sendto(sock_udp, &ans, sizeof(msg_t),0,(struct sockaddr *) &(shinfo.users[pos].ip_address), slen);
    while(1){
        // "P2P" "C2S" "GROUP"
        msg_t type;
        /*if(msgrcv(msqid, &type, sizeof(type)-sizeof(long), pos+1, 0) == -1) { // handle EXIT...
            error_msg("Error receiving from message queue ");
        }*/
        receive_message(&type, pos);
    
        if(strcmp(type.msg, "P2P")==0) {
            handle_p2p(pos, &ans, sock_fd);
        }
        else if(strcmp(type.msg, "C2S")==0) {
            handle_c2s(pos, &ans, sock_fd);
        } else if(strcmp(type.msg, "GROUP")==0) { // MULTICASTING
            handle_group(pos, &ans, sock_fd);
        }else{
            // erro
            printf("Incorrect message...\n");
        }
        

        // assuming client verifies whether type is allowed
        
    }
}

int main(int argc, char **argv)
{
    signal(SIGINT, cleanup);
    struct sockaddr_in si_server, si_client;
    slen = sizeof(si_client);
    //printf("Boo\n");
    // get_shared_memory();
    bzero(&shinfo, sizeof(shinfo));
    //printf("Boo\n");
    get_info_from_file();
    //printf("Boo\n");
    if(argc!=4) {
        fprintf(stderr, "Usage: ./server <client-port> <config-port> <configFile>\n");
        return 1;
    }
    
    char configFile[256];
    client_port = atoi(argv[1]);
    config_port = atoi(argv[2]);
    strcpy(configFile, argv[3]);
    pthread_create(&admin_pid, NULL, receive_admin, NULL);
    // pthread_create(&(groups.group[pos_group].thread),NULL, read_group_messages, &(groups.group[pos_group]));
    // wait for client connections -> INITIALISE UDP socket
    // TODO udp connection
    if((msqid = msgget(IPC_PRIVATE, IPC_CREAT|0766))<0) {
        error_msg("Error creating message queue ");
        // cleanup();
        exit(1);
    }
    // Cria um socket para recepção de pacotes UDP
    if ((sock_udp = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1){
        error_msg("Error creating socket");
        exit(1);
    }

    // Preenchimento da socket address structure
    si_server.sin_family = AF_INET;
    si_server.sin_port = htons(client_port);
    inet_pton(AF_INET, SERVER_IP, &(si_server.sin_addr));

    // Associa o socket à informação de endereço
    if (bind(sock_udp, (struct sockaddr *)&si_server, sizeof(si_server)) == -1)
        error_msg("Error binding socket");
    
    while (1) {
        // char buf[256];
        msg_t buf, ans;
        if(recvfrom(sock_udp, &buf, sizeof(msg_t),0,(struct sockaddr *) &si_client, &slen) == -1) {
		    error_msg("Error in recvfrom");
		    exit(1);
		}
        if(buf.userNo !=0) {
		printf("Message number: %ld : %s\n", buf.userNo,  buf.msg); 
            if(msgsnd(msqid, &buf, sizeof(buf)-sizeof(long), 0) == -1) {
                error_msg("Error sending through message queue: ");
            }
        }
        else{
            int ret = verify_userId(&buf, &si_client);
            printf("Value returned:%d\n", ret); 
            ans.userNo=ret+1;
            buf.userNo = ret+1;
            char ip_address_client[20];
            bzero(ip_address_client, sizeof(ip_address_client));
            switch(ret) {
                case -1:
                    sprintf(ans.msg, "IP INCORRECT");
                    sendto(sock_udp, &ans, sizeof(msg_t),0,(struct sockaddr *) &si_client, slen);
                    break;
                case -2:
                    sprintf(ans.msg, "NONEXISTENT USER");
                    sendto(sock_udp, &ans, sizeof(msg_t),0,(struct sockaddr *) &si_client, slen);
                    break;
                case -3:
                    sprintf(ans.msg, "ALREADY ONLINE");
                    sendto(sock_udp, &ans, sizeof(msg_t),0,(struct sockaddr *) &si_client, slen);
                    break;
                default:
                    printf("Boo\n");
                    inet_ntop(AF_INET, &(shinfo.users[ret].ip_address.sin_addr),ip_address_client, INET_ADDRSTRLEN);
                    printf("Boo\n");
                    sprintf(ans.msg, "%s\n", ip_address_client);
                    ans.userNo = ret+1; // beware of ZERO
                    sendto(sock_udp, &ans, sizeof(msg_t), 0, (struct sockaddr *)&si_client, slen);
                    // maybe save pid
                    // pthread_create(&(groups.group[pos_group].thread),NULL, read_group_messages, &(groups.group[pos_group]));
                    pthread_create(&(shinfo.users[ret].thread),NULL, process_client, (void*)&(shinfo.users[ret]));
                    /*if(fork()==0){
                        process_client(ret, si_client, sock_udp);
                        exit(0);
                    } */
                    break;
            }
            // Para ignorar o restante conteúdo (anterior do buffer)
            
            
        }
    }
    cleanup(0);
    return 0;
}


