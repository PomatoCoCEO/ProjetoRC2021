#include "client_structs.h"
#include "client_p2p.h"


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
        // !socklen_t slen = sizeof(si_server);
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
        printf("Message from server is incorrect: %s\n",msg.msg);
        // printf("%s\n", msg.msg);
    }
    return 0;
}