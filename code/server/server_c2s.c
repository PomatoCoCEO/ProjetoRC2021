#include "server_structs.h"
#include "server_c2s.h"
extern int sock_udp, msqid, admin_fd;
extern server_info_t shinfo;
extern socklen_t slen;


void handle_c2s(int pos, msg_t* ans, int sock_fd) {
    sprintf(ans->msg, "DESTINATION USER");
    ans->userNo = pos+1;
    sendto(sock_udp, ans, sizeof(msg_t),0,(struct sockaddr *) &(shinfo.users[pos].ip_address), slen);

    /*if(msgrcv(msqid, ans, sizeof(msg_t)-sizeof(long), pos+1, 0) == -1) { // it's a pointer
        error_msg("Error receiving from message queue ");
        return;
    }*/
    receive_message(ans, pos);
    // processar -> int find_user
    int pos_dest = find_user(ans);
    printf("pos_dest = %d\n", pos_dest);
    if(pos_dest == -1) {
        sprintf(ans->msg, "USER NOT FOUND");
        sendto(sock_udp, ans, sizeof(msg_t),0,(struct sockaddr *) &(shinfo.users[pos].ip_address), slen);
        return;
    }else{
        if(shinfo.users[pos_dest].online ==1)
        {
            sprintf(ans->msg, "ENTER MESSAGE");
            sendto(sock_udp, ans, sizeof(msg_t),0,(struct sockaddr *) &(shinfo.users[pos].ip_address), slen);
            receive_message(ans, pos);

            send_msg_to_user(pos, pos_dest, ans); // NEW MESSAGE from paulocorte: (...)
            sprintf(ans->msg, "MESSAGE SENT");
            sendto(sock_udp, ans, sizeof(msg_t),0,(struct sockaddr *) &(shinfo.users[pos].ip_address), slen);
        } else{
            sprintf(ans->msg, "USER OFFLINE");
            sendto(sock_udp, ans, sizeof(msg_t),0,(struct sockaddr *) &(shinfo.users[pos].ip_address), slen);
            return;
        }
    }
        
}

void send_msg_to_user(int pos, int pos_dest, msg_t * msg) {
    // pos está correto
    msg_t buf;
    //bzero(buf, sizeof(buf));
    sprintf(buf.msg, "NEW MESSAGE FROM %s: %s",shinfo.users[pos].userId, msg->msg);
    buf.userNo= pos_dest+1;
    sendto(sock_udp, &buf, sizeof(msg_t),0,(struct sockaddr *) &(shinfo.users[pos_dest].ip_address), slen);
}