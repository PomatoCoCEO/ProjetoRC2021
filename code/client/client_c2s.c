#include "client_structs.h"
#include "client_c2s.h"

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
        if(receive_msg()!=0) return 2;
        if(strcmp(msg.msg, "MESSAGE SENT")==0) {
            printf("Message sent successfully\n");
        }
        else{
            printf("There might have been problems sending your message...\n");
        }
        return 0;
    }
    else{
        printf("Received incorrect message from server: %s\n", msg.msg);
    }
    return 0;
}