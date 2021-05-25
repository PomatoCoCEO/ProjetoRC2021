#include "server_structs.h"
#include "server_p2p.h"
extern int sock_udp, msqid, admin_fd;
extern server_info_t shinfo;
extern socklen_t slen;

void handle_p2p(int pos, msg_t* ans, int sock_fd){
    sprintf(ans->msg, "DESTINATION USER");
    sendto(sock_udp, ans, sizeof(msg_t),0,(struct sockaddr *) &(shinfo.users[pos].ip_address), slen);

    receive_message(ans, pos);
    // processar -> int find_user
    int pos_dest = find_user(ans);
    printf("pos_dest = %d\n", pos_dest);
    if(pos_dest == -1) {
        sprintf(ans->msg, "USER NOT FOUND");
        sendto(sock_udp, ans, sizeof(msg_t),0,(struct sockaddr *) &(shinfo.users[pos].ip_address), slen);
        return;
    }else{
        if(shinfo.users[pos_dest].online==1)
        {
            // char buf[256];
            msg_t msg;
            msg.userNo=pos+1;
            char ip[20];
            inet_ntop(AF_INET, &(shinfo.users[pos_dest].ip_address.sin_addr),ip, INET_ADDRSTRLEN);
            sprintf(msg.msg, "IP ADDRESS:%s:%d", ip, shinfo.users[pos_dest].ip_address.sin_port);
            printf("%s\n", msg.msg);
            sendto(sock_udp, &msg, sizeof(msg_t),0,(struct sockaddr *) &(shinfo.users[pos].ip_address), slen);
            
        } else{
            sprintf(ans->msg, "USER OFFLINE");
            sendto(sock_udp, ans, sizeof(msg_t),0,(struct sockaddr *) &(shinfo.users[pos].ip_address), slen);
            return;
        }
    }
}

