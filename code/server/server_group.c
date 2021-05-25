#include "server_structs.h"
#include "server_group.h"
extern int sock_udp, msqid, admin_fd;
extern server_info_t shinfo;
extern socklen_t slen;

int find_group(char group[]) {
    printf("Comparing against %s\n", group);
    for(int i = 0; i<shinfo.no_groups; i++) {
        if(strcmp(shinfo.groups[i].name, group)==0) {
            return i;
        }
    }
    return -1;
}


void handle_group(int pos, msg_t* ans, int sock_fd) {
    sprintf(ans->msg, "GROUP NAME");
    sendto(sock_udp, ans, sizeof(msg_t),0,(struct sockaddr *) &(shinfo.users[pos].ip_address), slen);
    receive_message(ans, pos);
    char * group_name = ans->msg;
    if(strstr(group_name, "NEW GROUP:") == group_name) { // criação de novo grupo
        /*/*sprintf(ans->msg, "NEW GROUP NAME");
        sendto(sock_udp, ans, sizeof(msg_t),0,(struct sockaddr *) &(shinfo.users[pos].ip_address), slen);
        if(msgrcv(msqid, ans, sizeof(msg_t)-sizeof(long), pos+1, 0) == -1) {
            error_msg("Error receiving from message queue ");
            return;
        }*/
        strtok(ans->msg, ":");
        char* group_name = strtok(NULL, "\n");
        if(group_name == NULL) {
            printf("Group name not obtained!\n");
            return;
        }
        int pos_group = find_group(group_name);
        if(pos_group == -1) {
            int pos_new = shinfo.no_groups;
            // if(pos_new >= NO_MAX_GROUPS) group limit exceeded!
            char ip_address[20];
            bzero(ip_address, sizeof(ip_address));
            sprintf(ip_address, "224.1.0.%d", pos_new);
            inet_pton(AF_INET, ip_address, &(shinfo.groups[pos_new].ip_address.sin_addr));
            shinfo.no_groups++;
            int port_no = 9001+pos_new;
            shinfo.groups[pos_new].ip_address.sin_port = htons(port_no); // ???
            strcpy(shinfo.groups[pos_new].name, group_name);
            sprintf(ans->msg, "IP ADDRESS:%s:%d", ip_address, port_no);
            sendto(sock_udp, ans, sizeof(msg_t),0,(struct sockaddr *) &(shinfo.users[pos].ip_address), slen);
        } else{
            sprintf(ans->msg, "GROUP NAME INVALID");
            sendto(sock_udp, ans, sizeof(msg_t),0,(struct sockaddr *) &(shinfo.users[pos].ip_address), slen);
            return;
        }

    } else if(strstr(ans->msg, "FIND GROUP:")==ans->msg){
        strtok(ans->msg, ":");
        char* group_name = strtok(NULL, ":");
        if(group_name == NULL) {
            printf("Group name not obtained!\n");
            return;
        }
        int pos_group = find_group(group_name);
        if(pos_group >= 0) {
            char ip_address[20]; bzero(ip_address, sizeof(ip_address));
            inet_ntop(AF_INET, &(shinfo.groups[pos_group].ip_address.sin_addr),ip_address, INET_ADDRSTRLEN);
            int port = htons(shinfo.groups[pos_group].ip_address.sin_port);
            sprintf(ans->msg, "IP ADDRESS:%s:%d",ip_address, port);
            sendto(sock_udp, ans, sizeof(msg_t),0,(struct sockaddr *) &(shinfo.users[pos].ip_address), slen);
        }
        else{
            sprintf(ans->msg, "GROUP NOT FOUND");
            sendto(sock_udp, ans, sizeof(msg_t),0,(struct sockaddr *) &(shinfo.users[pos].ip_address), slen);
            return;
        }
    }
    else if(strcmp(group_name, "EXIT") == 0) {
        printf("Exiting group menu.\n");
    }
    // processar -> int find_user
    // int pos_dest = find_user(ans);
   
}