#include "client_structs.h"
#include "client_group.h"


int find_group_pos(char* name) {
    for(int i = 0; i<groups.no_groups; i++) {
        printf("Comparing against %s\n",groups.group[i].name );
        if(strcmp(name, groups.group[i].name)==0) return i;
    }
    return -1;
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
            close(g->sock_fd);
            pthread_exit(NULL);
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
    if(find_group_pos(group_name)>=0) {
        sprintf(msg.msg, "EXIT");
        printf("You already belong to that group!\n");
        if( sendto(sock_fd, &msg, sizeof(msg_t),0, (struct sockaddr*)&si_server, slen)==-1) {
        error_msg("While sending message");
        return 2;
    }
        return 1;
    }
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
            // !socklen_t slen = sizeof(struct sockaddr_in);
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
                error_msg("Error setting local interface error");
                exit(1);
            }

            int multicastTTL = 255;
            if (setsockopt(groups.group[pos_group].sock_fd, IPPROTO_IP, IP_MULTICAST_TTL, (void *) &multicastTTL, sizeof(multicastTTL)) < 0) {
                error_msg("Error setting TTL socket option");
                return 1;
            }
            int reuse = 1;

            if(setsockopt(groups.group[pos_group].sock_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse)) < 0)
            {
                perror("Error setting SO_REUSEADDR error");
                close(groups.group[pos_group].sock_fd);
                exit(1);
            }

            pthread_create(&(groups.group[pos_group].thread),NULL, read_group_messages, &(groups.group[pos_group]));
            return 0;
    }else{
        printf("Received incorrect message from server: %s", msg.msg);
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
    else if (res==2){
        send_group_msg();
    }
    return 0;
}