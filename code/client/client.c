#include "client.h"


void error_msg(char *msg)
{
    fprintf(stderr, "Erro: %s -> %s\n", msg, strerror(errno));
    exit(1);
}

void wrong_command(int fd, char *error_msg) // answer to client 
{
    write(fd, error_msg, 1 + strlen(error_msg));
}

int authentication(){
    printf("Enter password:\n");
    scanf("%s", msg.msg);
    while(getchar()!='\n'); // avoid any other spaces & such
    
    if(sendto(sock_fd, &msg, sizeof(msg_t),0, (struct sockaddr*)&si_server, slen)==-1) {
        error_msg("no envio de mensagem");
    }
    
    if(recvfrom(sock_fd, &msg, sizeof(msg_t) , 0, (struct sockaddr*) &si_server, &slen)==-1) {
        error_msg("na receção da mensagem");
    }
    if(strcmp(msg.msg, "PASSWORD INCORRECT") == 0) {
        return 2;
    }
    allowed = atoi(msg.msg);
    return 1;
}

int login() {
    printf("Enter your username.\n");
    scanf("%s",msg.msg);
    while(getchar()!='\n'); // avoid any other spaces & such
    msg.userNo = 0; 
    if( sendto(sock_fd, &msg, sizeof(msg_t),0, (struct sockaddr*)&si_server, slen)==-1) {
        error_msg("no envio de mensagem");
        return 2;
    }

    if(recvfrom(sock_fd, &msg, sizeof(msg_t) , 0, (struct sockaddr*) &si_server, &slen)==-1) {
        error_msg("na receção da mensagem");
        return 2;
    }

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
            printf("\t%s ", permTypes[i]);
        }
    }
    printf("\n");

}

int menu() {
    int ans; 
    do{
        printf("Choose which type of communication you would like to engage in:\n");
        // const char* abrev={"C2S","P2P", "GRP"};
        for(int i = 0; i<=3; i++) {
            if(i == 0) 
                printf("\t0 -> Exit\n");
            else
                printf("\t%d -> %s\n",i, permTypes[i]);
        }
    
        if (scanf("%d", &ans)!=1){
            printf("Invalid input... Try again\n");
            while(getchar()!='\n');
            continue;
        }
    } while(ans<0 || ans>3);
    return ans;
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
    char username[256];
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
                // client_to_server();
                break;
            case 2:
                // P2P
                // peer_to_peer();
                break;
            case 3:
                // GROUP
                // group_comm();
                break;
        }
    } while(res!=0);
    // connection to server to login
    
    // user states which kind of communication he wants to engage on

    

    return 0;
}
