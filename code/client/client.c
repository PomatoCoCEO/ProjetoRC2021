// #include "client.h"
// client.h 
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

typedef struct {
    long userNo;
    char msg[256];
} msg_t;

int sock_fd, allowed, no_user;
msg_t msg;
struct sockaddr_in si_server;
socklen_t slen = sizeof(si_server);
const char* permTypes[] = {"Client-Server", "P2P" , "GROUP"};
char username[256];


int receive_msg();
int client_to_server();
int peer_to_peer();
int group_comm();

///////////////



void error_msg(char *msg)
{
    fprintf(stderr, "Erro: %s -> %s\n", msg, strerror(errno));
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
            error_msg("na receção da mensagem");
            return -1;
        }
        if (strstr(msg.msg,"NEW MESSAGE")!=NULL) {
            printf("%s\n", msg.msg);
            // continue;
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
        error_msg("no envio de mensagem");
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
        error_msg("no envio de mensagem");
        return 2;
    }
    if(receive_msg()!=0) return 2;

    if(strcmp(msg.msg, "OK")==0) {
        // handle type of communication

        if(authentication() == 1)
            return 0;
        else{
            printf("Wrong password! Try again\n");
        }
    } else if(strcmp(msg.msg, "IP INCORRECT") == 0){ // exit
        printf("User logging in from wrong ip address!\n");
    } else if(strcmp(msg.msg,"NONEXISTENT USER")==0){
        printf("User is nonexistent. Try again.\n");
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
        } else 
            return ans;
    } while(1);
    
}

int client_to_server(){
    sprintf(msg.msg, "C2S");
    
    if( sendto(sock_fd, &msg, sizeof(msg_t),0, (struct sockaddr*)&si_server, slen)==-1) {
        error_msg("no envio de mensagem");
        return 2;
    }
    if(receive_msg()!=0) return 2;
    if(strcmp(msg.msg, "DESTINATION USER")==0) {
        printf("Enter username of destination user.\n");
        scanf("%s", msg.msg);
        while(getchar()!='\n');
        if( sendto(sock_fd, &msg, sizeof(msg_t),0, (struct sockaddr*)&si_server, slen)==-1) {
            error_msg("no envio de mensagem");
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
        fgets(msg.msg, 200, stdin);
        if(sendto(sock_fd, &msg, sizeof(msg_t),0, (struct sockaddr*)&si_server, slen)==-1) {
            error_msg("no envio de mensagem");
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
        error_msg("no envio de mensagem");
        return 2;
    }

    if(receive_msg()!=0) return 2;
    if(strcmp(msg.msg, "DESTINATION USER")==0) {
        printf("Enter username of destination user.\n");
        scanf("%s", msg.msg);
        while(getchar()!='\n');
        if( sendto(sock_fd, &msg, sizeof(msg_t),0, (struct sockaddr*)&si_server, slen)==-1) {
            error_msg("no envio de mensagem");
            return 2;
        }
    }else{
        error_msg(" receção de mensagem incorreta para início de comunicação.\n");
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
    else if(strstr(msg.msg, "IP_ADDRESS")!=NULL){ // "IP_ADDRESS <192.136.212.129>:<9000>"

        struct sockaddr_in si_dest;
        printf("%s\n", msg.msg);
        socklen_t slen = sizeof(si_server);
        char * token =strtok(msg.msg, " :");
        token= strtok(NULL, " :");
        int portNo;
        if (token == NULL){
            //erro
            error_msg("endereço de IP do utilizador de destino não obtido");
            return 1;
        }
    
        bzero((void *) &si_dest, sizeof(si_dest));
        si_dest.sin_family = AF_INET;
            
        if(inet_pton(AF_INET, token, &(si_dest.sin_addr))!=1) {
            error_msg("na conversão de endereço IP");
            return 2;
        }
        token= strtok(NULL, " :"); 
        if (token == NULL){
            //erro
            error_msg("porto do utilizador de destino não obtido");
            return 1;
        }
        sscanf(token, "%d", &portNo); // allows for error correction & detection
        //si_dest.sin_port = htons((short) portNo);
        si_dest.sin_port = (short) portNo;
        //send
        printf("Enter message to send (max. 200 characters)\n");
        // scanf("%s", msg.msg);
        fgets(msg.msg, 200, stdin);
        msg_t to_send;
        sprintf(to_send.msg,"NEW MESSAGE FROM %s: %s", username,msg.msg);
        
        if(sendto(sock_fd, &to_send, sizeof(msg_t),0, (struct sockaddr*)&si_dest, slen)==-1) {
            error_msg("no envio de mensagem");
            return 2;
        }
        return 0;
    }
    else{
        //erro
        printf("Message from server is incorrect.\n");
        // printf("%s\n", msg.msg);
    }
    return 0;
}



int group_comm(){
    printf("Insira o nome do grupo para o qual quer comunicar (\'NOVO\' para criar um grupo).\n");
    char group[256];
    fgets(group, 255, stdin);
    if(strcasecmp(group, "NOVO") == 0) {
        
    }
    return 0;
}

int main(int argc, char** argv) { // ./client <server-address> <port>
    // greetings to user
    if(argc<3) {
        printf("Usage: ./client <server-address> <server-port>\n");
        return 0;
    }
    bzero((void *) &si_server, sizeof(si_server));
    si_server.sin_family = AF_INET;
    inet_pton(AF_INET, argv[1], &(si_server.sin_addr));
    si_server.sin_port = htons((short) atoi(argv[2]));


    if((sock_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1) {
        error_msg("na criação do socket.");
    }
    msg.userNo = 0;
    // char username[256];
    printf("Greetings! ");
    while(login());
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
