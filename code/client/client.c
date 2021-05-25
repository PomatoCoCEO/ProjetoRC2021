// #include "client.h"
// client.h 
// IP interface
#include "client.h"
const char* permTypes[]={"Client-Server", "P2P" , "GROUP"};
int receive_msg();
int client_to_server();
int peer_to_peer();
int group_comm();

///////////////


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
    } else if(strcmp(msg.msg,"ALREADY ONLINE")==0){
        printf("User is already online!\n");
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


void cleanup(int signo) {
    sprintf(msg.msg, "EXIT_ALL");
    if(sendto(sock_fd, &msg, sizeof(msg_t),0, (struct sockaddr*)&si_server, slen)==-1) {
        error_msg("While sending message");
        return;
    }
    if (close(sock_fd) == -1)
    {
        perror("ERROR: Failed to close sock_fd\n");
        exit(1);
    }
    for(int i = 0; i<groups.no_groups; i++) {
        if (close(groups.group[i].sock_fd) == -1)
        {
            perror("ERROR: Failed to close  group sock_fd\n");
            exit(1);
        }
    }
    exit(1);
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
    local_port_no = (short) atoi(argv[2]);
    slen = sizeof(si_server);
    if((sock_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1) {
        error_msg("In socket creation.");
    }
    msg.userNo = 0;
    // char username[256];
    printf("Greetings! ");
    while(login());
    signal(SIGINT, cleanup);
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
    // send message like "EXIT ALL"
    cleanup(0);
    // user states which kind of communication he wants to engage in

    return 0;
}
