// #include "client.h"
// client.h 
// IP interface
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
socklen_t slen = sizeof(si_server);
const char* permTypes[] = {"Client-Server", "P2P" , "GROUP"};
group_array groups;
char username[256], local_ip[20];
struct ip_mreq group;


int receive_msg();
int client_to_server();
int peer_to_peer();
int group_comm();

///////////////

int find_group_pos(char* name) {
    for(int i = 0; i<groups.no_groups; i++) {
        printf("Comparing against %s\n",groups.group[i].name );
        if(strcmp(name, groups.group[i].name)==0) return i;
    }
    return -1;
}

void error_msg(char *msg)
{
    fprintf(stderr, "Error: %s -> %s\n", msg, strerror(errno));
    exit(1);
}

void wrong_command(int fd, char *error_msg) // answer to client 
{
    write(fd, error_msg, 1 + strlen(error_msg));
}

int receive_msg() {
    while(1){
        // loop until receiving a message that's not from a user (C2S, P2P, GROUP)
        if(recvfrom(sock_fd, &msg, sizeof(msg_t) , 0, (struct sockaddr*) &si_server, &slen)==-1) {
            error_msg("While receiving message");
            return -1;
        }
        printf("Message:%s\n", msg.msg);
        if (strstr(msg.msg,"NEW MESSAGE")!=NULL) {
            printf("%s\n", msg.msg);
        }
        else {
            printf("userNo %ld\n", msg.userNo);
            return 0;
        }
        // else continues... 
    }
}

int authentication(){
    printf("Enter password:\n");
    scanf("%s", msg.msg);
    while(getchar()!='\n'); // avoid any other spaces & such
    
    if(sendto(sock_fd, &msg, sizeof(msg_t),0, (struct sockaddr*)&si_server, slen)==-1) {
        error_msg("While sending message.");
    }
    if(receive_msg()!=0) return -1;
    if(strcmp(msg.msg, "PASSWORD INCORRECT") == 0) {
        return 2;
    }
    allowed = atoi(msg.msg);
    
    no_user = msg.userNo;
    printf("USERNO: %d", no_user);
    return 1;
}

int login() {
    printf("Enter your username.\n");
    scanf("%s",msg.msg);
    strcpy(username, msg.msg);
    while(getchar()!='\n'); // avoid any other spaces & such
    msg.userNo = 0; 
    if( sendto(sock_fd, &msg, sizeof(msg_t),0, (struct sockaddr*)&si_server, slen)==-1) {
        error_msg("While sending message.");
        return 2;
    }
    if(receive_msg()!=0) return 2;

    if(strcmp(msg.msg, "IP INCORRECT") == 0){ // exit
        printf("User logging in from wrong ip address!\n");
    } else if(strcmp(msg.msg,"NONEXISTENT USER")==0){
        printf("User is nonexistent. Try again.\n");
    }else {
        //convert ip to 
        strncpy(local_ip, msg.msg,20);

        if(authentication() == 1)
            return 0;
        else{
            printf("Wrong password! Try again\n");
        }
    } 
    return 1;
}

void show_permissions() {
    if(allowed == 0){
        printf("You have no permission whatsoever to engage in any type of communication!\n");
        return;
    }
    printf("You have the following permissions:\n");
    for(int i = 2; i>=0; i--) {
        if(allowed&(1<<i)) {
            printf("\t%s ", permTypes[2-i]);
        }
    }
    printf("\n");
}

int menu() {
    int ans; 
    do{
        printf("Choose which type of communication you would like to engage in: (%d)\n", allowed);
        // const char* abrev={"C2S","P2P", "GRP"};

        for(int i = 0; i<=3; i++) {
            if(i == 0) 
                printf("\t0 -> Exit\n");
            else if (1 & (allowed >> (3-i))){
                printf("\t%d -> %s\n",i, permTypes[i-1]);
            }
        }
    
        if (scanf("%d", &ans)!=1 || ans<0 || ans > 3 ||(ans!=0 && (((allowed>>(3-ans))&1)==0))){
            printf("Invalid input... Try again\n");
            while(getchar()!='\n');
            continue;
        } else{
            while(getc(stdin)!='\n');
            return ans;
        } 
    } while(1);
    
}

int client_to_server(){
    sprintf(msg.msg, "C2S");
    
    if( sendto(sock_fd, &msg, sizeof(msg_t),0, (struct sockaddr*)&si_server, slen)==-1) {
        error_msg("While sending message");
        return 2;
    }
    if(receive_msg()!=0) return 2;
    if(strcmp(msg.msg, "DESTINATION USER")==0) {
        printf("Enter username of destination user.\n");
        scanf("%s", msg.msg);
        while(getchar()!='\n');
        if( sendto(sock_fd, &msg, sizeof(msg_t),0, (struct sockaddr*)&si_server, slen)==-1) {
            error_msg("While sending message");
            return 2;
        }
    }else{
        // erro
    }

    if(receive_msg()!=0) return 2;
    if(strcmp(msg.msg, "USER NOT FOUND")==0){
        printf("Username not found\n");
        return 1;
    }
    else if(strcmp(msg.msg, "USER OFFLINE")==0){
        printf("User not online\n");
        return 1;
    }
    else if(strcmp(msg.msg, "ENTER MESSAGE")==0){
        printf("Enter message to send (max. 200 characters)\n");
        //while(getc(stdin)!='\n');
        fgets(msg.msg, 200, stdin);
        if(sendto(sock_fd, &msg, sizeof(msg_t),0, (struct sockaddr*)&si_server, slen)==-1) {
            error_msg("While sending message");
            return 2;
        }
        return 0;
    }
    else{
        //erro
    }
    return 0;
}

int peer_to_peer(){
    sprintf(msg.msg, "P2P");
    if( sendto(sock_fd, &msg, sizeof(msg_t),0, (struct sockaddr*)&si_server, slen)==-1) {
        error_msg("While sending message");
        return 2;
    }

    if(receive_msg()!=0) return 2;
    if(strcmp(msg.msg, "DESTINATION USER")==0) {
        printf("Enter username of destination user.\n");
        scanf("%s", msg.msg);
        while(getchar()!='\n');
        if( sendto(sock_fd, &msg, sizeof(msg_t),0, (struct sockaddr*)&si_server, slen)==-1) {
            error_msg("While sending message");
            return 2;
        }
    } else {
        error_msg("Received incorrect message.\n");
        // erro
    }

    if(receive_msg()!=0) return 2;
    printf("Boo1\n");
    if(strcmp(msg.msg, "USER NOT FOUND")==0){
        printf("Username not found\n");
        return 1;
    }
    else if(strcmp(msg.msg, "USER OFFLINE")==0){
        printf("User not online\n");
        return 1;
    }
    else if(strstr(msg.msg, "IP ADDRESS")!=NULL){ // "IP ADDRESS:<192.136.212.129>:<9000>"
        struct sockaddr_in si_dest;
        printf("%s\n", msg.msg);
        socklen_t slen = sizeof(si_server);
        char * token =strtok(msg.msg, ":");
        token= strtok(NULL, ":");
        int portNo;
        if (token == NULL){
            //erro
            error_msg("Destination user's IP address not received.");
            return 1;
        }
    
        bzero((void *) &si_dest, sizeof(si_dest));
        si_dest.sin_family = AF_INET;
        printf("token: %s\n", token);
        if(inet_pton(AF_INET, token, &(si_dest.sin_addr))!=1) {
            error_msg("While converting IP address.");

            return 2;
        }
        token= strtok(NULL, ":"); 
        if (token == NULL){
            //erro
            error_msg("Destination user's port not received.");
            return 1;
        }
        sscanf(token, "%d", &portNo); // allows for error correction & detection
        //si_dest.sin_port = htons((short) portNo);
        si_dest.sin_port = (short) portNo;
        //send
        printf("Enter message to send (max. 200 characters)\n");
        // scanf("%s", msg.msg);
        // while(getc(stdin)!='\n');
        fgets(msg.msg, 200, stdin);
        // strtok(msg.msg, "\n\r");
        msg_t to_send;
        sprintf(to_send.msg,"NEW MESSAGE FROM %s: %s", username,msg.msg);
        
        if(sendto(sock_fd, &to_send, sizeof(msg_t),0, (struct sockaddr*)&si_dest, slen)==-1) {
            error_msg("While sending message");
            return 2;
        }
        printf("Message has been sent\n");
        return 0;
    }
    else{
        //erro
        printf("Message from server is incorrect: %s\n");
        // printf("%s\n", msg.msg);
    }
    return 0;
}

int menu_group_comm(){
    int ans; 
    do{
        printf("Choose what you want to do\n0- Exit\n1- Join or create a new group\n2- Send a message to one of your groups\n");

        // const char* abrev={"C2S","P2P", "GRP"};
    
        if (scanf("%d", &ans)!=1 || ans<0 || ans > 2 ){
            printf("Invalid input... Try again\n");
            while(getchar()!='\n');
            continue;
        } else {
            while(getc(stdin)!='\n');
            return ans;
        }
    } while(1);
}

void * read_group_messages(void * p){
    group_t * g =((group_t *)p );
    msg_t received_msg;
    struct sockaddr_in si_origin;
    while(1){ 
        if(recvfrom(g->sock_fd, &received_msg, sizeof(msg_t) , 0, (struct sockaddr*) &si_origin, &slen)==-1) {
            error_msg("While receiving message");
            return -1;
        }
        printf("%s\n", received_msg.msg);
    }
}

int join_group(){
    sprintf(msg.msg, "GROUP");
    if( sendto(sock_fd, &msg, sizeof(msg_t),0, (struct sockaddr*)&si_server, slen)==-1) {
        error_msg("While sending message");
        return 2;
    }

    if(receive_msg()!=0) return 2;
    if(strcmp(msg.msg, "GROUP NAME")!=0) {
        printf("Invalid server message - expected GROUP NAME\n");
        return 1;
    }
    printf("Enter the name of the group you want to join (\'NEW\' to create a new one).\n");
    char group_name[256];
    //while(getc(stdin)!='\n');
    fgets(group_name, 255, stdin);
    strtok(group_name, "\n\r");
    printf("String obtained: \'%s\'\n", group_name);
    if(strcasecmp(group_name, "NEW") == 0) { // criar novo grupo
        printf("Enter new group name.\n");
        fgets(group_name, 255, stdin);
        strtok(group_name, "\n\r");
        if(strcasecmp(group_name, "NEW")==0) {
            printf("Group name \'NEW\' is invalid.\n");
            return 1;
        }
        if(find_group_pos(group_name)>=0) {
            printf("You already belong to this group.\n");
            return 1;
        }
        
        sprintf(msg.msg, "NEW GROUP:%s", group_name);
        if(sendto(sock_fd, &msg, sizeof(msg_t),0, (struct sockaddr*)&si_server, slen)==-1) {
            error_msg("While sending message");
            return 2;
        }
        if(receive_msg()!=0) return 2;

        if (strcasecmp(msg.msg,"GROUP NAME INVALID")==0){
            printf("Group name invalid\n");
            return 1;
        }
        // servidor verifica se grupo tem nome válido (pode ser já existente e n pode ter o nome 'NEW GROUP: ')
        // pode responder OK:(IP multicast:porto)
        // ou INVALID NAME; ou ALREADY EXISTS
    }
    else{
        sprintf(msg.msg, "FIND GROUP:%s", group_name);
        if(sendto(sock_fd, &msg, sizeof(msg_t),0, (struct sockaddr*)&si_server, slen)==-1) {
            error_msg("While sending message");
            return 2;
        }
        if(receive_msg()!=0) return 2;

        if (strcasecmp(msg.msg,"GROUP NOT FOUND")==0){
            printf("Group not found\n");
            return 1;
        }
    }
    if (strstr(msg.msg,"IP ADDRESS")!=NULL) {
            printf("%s\n", msg.msg);
            //convert multicast address from string 
            int pos_group= find_group_pos(msg.msg);
            if(pos_group == -1 ){
                pos_group=groups.no_groups++;
            } else{
                printf("You already belong to that group\n");
                return 2;
            }
            printf("%s\n", msg.msg);
            socklen_t slen = sizeof(struct sockaddr_in);
            char * token =strtok(msg.msg, ":");
            token= strtok(NULL, ":");
            if (token == NULL){
                //erro
                error_msg("Group's IP address not received.");
                return 1;
            }

            printf("token=%s\n", token);
        
            bzero((void *) &(groups.group[pos_group].ip_address), sizeof(struct sockaddr_in));
            groups.group[pos_group].ip_address.sin_family = AF_INET;
            if(inet_pton(AF_INET, token, &(groups.group[pos_group].ip_address.sin_addr))!=1) {
                error_msg("While converting IP address.");
                return 2;
            }
            char multicast_ip[20];
            strcpy(multicast_ip, token);
            token= strtok(NULL, ":"); 
            if (token == NULL){
                //erro
                error_msg("Group port not received.");
                return 1;
            }
            int portNo;
            sscanf(token, "%d", &portNo); // allows for error correction & detection
            //si_dest.sin_port = htons((short) portNo);
            groups.group[pos_group].ip_address.sin_port = (short)ntohs(portNo);

            if(( groups.group[pos_group].sock_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
                error_msg("Creating the socket");
            }
            //connect to group
            struct sockaddr_in group_local_sock;
            bzero(&group_local_sock, sizeof(group_local_sock));
            group_local_sock.sin_family = AF_INET;
            group_local_sock.sin_port = ntohs(portNo);
            group_local_sock.sin_addr.s_addr = INADDR_ANY;
            if(bind(groups.group[pos_group].sock_fd, (struct sockaddr*)&group_local_sock, sizeof(group_local_sock))<0)
            {
                perror("Binding datagram socket error");
                close(groups.group[pos_group].sock_fd);
                exit(1);
            }

            inet_pton(AF_INET, multicast_ip, &(group.imr_multiaddr));
            group.imr_interface = localInter;
            if(setsockopt(groups.group[pos_group].sock_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&group, sizeof(group)) < 0)
            {
                error_msg("Adding multicast group error");
                return 1;
            }
            else
                printf("Adding multicast group...OK.\n");
            printf("Group name: %s", group_name);
            strcpy(groups.group[pos_group].name, group_name);
            // now to make it able to
            inet_pton(AF_INET, local_ip, &(localInter));
            if(setsockopt(groups.group[pos_group].sock_fd, IPPROTO_IP, IP_MULTICAST_IF, (char *)&localInter, sizeof(localInter)) < 0)
            {
                perror("Setting local interface error");
                exit(1);
            }

            int multicastTTL = 255;
            if (setsockopt(groups.group[pos_group].sock_fd, IPPROTO_IP, IP_MULTICAST_TTL, (void *) &multicastTTL, sizeof(multicastTTL)) < 0) {
                error_msg("Setting TTL socket option");
                return 1;
            }
            int reuse = 1;

            if(setsockopt(groups.group[pos_group].sock_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse)) < 0)
            {
                perror("Setting SO_REUSEADDR error");
                close(groups.group[pos_group].sock_fd);
                exit(1);
            }

            pthread_create(&(groups.group[pos_group].thread),NULL, read_group_messages, &(groups.group[pos_group]));
            return 0;
    }else{
        error_msg("Received incorrect message from server");
    }
    return 0;
}


int send_group_msg(){
    /*
    if(receive_msg()!=0) return 2;
    if(strcmp(msg.msg, "GROUP NAME")!=0) {
        printf("Invalid server message - expected GROUP NAME\n");
        return 1;
    }
    */
    printf("Enter the name of the group you want to communicate to .\n");
    char group_name[256];
    
    fgets(group_name, 255, stdin);
    strtok(group_name, "\n\r");
    int pos_group= find_group_pos(group_name);
    if (pos_group==-1){
        printf("Invalid name\n");
        return 2;
    }
   
    printf("Enter message to send (max. 200 characters)\n");
    // scanf("%s", msg.msg);
    fgets(msg.msg, 200, stdin);
    //strtok(msg.msg, "\n\r");
    msg_t to_send;
    sprintf(to_send.msg,"NEW MESSAGE FROM %s IN GROUP %s: %s", username, group_name,msg.msg);
    if(sendto(groups.group[pos_group].sock_fd, &to_send, sizeof(msg_t),0, (struct sockaddr*)&(groups.group[pos_group].ip_address), slen)==-1) { // all are IPv4
        error_msg("While sending message");
        return 2;
    }
    
    return 0;

}

int group_comm(){

    
    
    int res=menu_group_comm();

    if (res==1){
        join_group();
    }
    else{
        send_group_msg();
    }
    return 0;
}

void cleanup(int signo) {
    for(int i = 0; i<groups.no_groups; i++) close(groups.group[i].sock_fd);
    close(sock_fd);
    exit(1);
}

int main(int argc, char** argv) { // ./client <server-address> <port>
    // greetings to user
    if(argc<3) {
        printf("Usage: ./client <server-address> <server-port>\n");
        return 0;
    }
    signal(SIGINT, cleanup);
    bzero((void *) &si_server, sizeof(si_server));
    si_server.sin_family = AF_INET;
    inet_pton(AF_INET, argv[1], &(si_server.sin_addr));
    si_server.sin_port = htons((short) atoi(argv[2]));
    local_port_no = (short) atoi(argv[2]);

    if((sock_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1) {
        error_msg("In socket creation.");
    }
    msg.userNo = 0;
    // char username[256];
    printf("Greetings! ");
    while(login());

    /*if((allowed & 1)){
        inet_pton(AF_INET, local_ip, &(localInter));
        if(setsockopt(sock_fd, IPPROTO_IP, IP_MULTICAST_IF, (char *)&localInter, sizeof(localInter)) < 0)
        {
            perror("Setting local interface error");
            exit(1);
        }

        int multicastTTL = 255;
        if (setsockopt(sock_fd, IPPROTO_IP, IP_MULTICAST_TTL, (void *) &multicastTTL, sizeof(multicastTTL)) < 0) {
            error_msg("Setting TTL socket option");
            return 1;
        }
        /*
            int reuse = 1;

            if(setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse)) < 0)
            {
                perror("Setting SO_REUSEADDR error");
                close(sd);
                exit(1);
            }
    }*/
    show_permissions();
    int res=1;
    do {
        res=menu();
        switch(res){
            case 0:
                // leave
                printf("Exiting...\n");
                break;
            case 1:
                // C2S
                client_to_server();
                break;
            case 2:
                // P2P
                peer_to_peer();
                break;
            case 3:
                // GROUP
                group_comm();
                break;
        }
    } while(res!=0);
    // connection to server to login
    
    // user states which kind of communication he wants to engage on

    return 0;
}
