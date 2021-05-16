#include "server.h"

void print_user_info(int fd, user_info* user) {
    const char* aid[2]={"No", "Yes"};
    char ip_addr[16];
    char msg[512];
    inet_ntop(AF_INET, &(user->ip_address.sin_addr),ip_addr, INET_ADDRSTRLEN);
    char perm[3];
    for(int i = 2; i>=0; i--) {
        if(user->permissions & (1<<i) ) {
            perm[i]=1;
        }
        else perm[i]=0;
    }
    sprintf(msg,"%s %s %s %s %s %s\n", user->userId, ip_addr, user->password, aid[perm[2]], aid[perm[1]], aid[perm[0]]);
    write (fd, msg, strlen(msg));
}

int remove_from_list(char * name){
    for (int i=0; i< shmem->no_users; ++i){
        if (strcasecmp(name, shmem->users[i].userId)==0){
            shmem->users[i] = shmem->users[--shmem->no_users];
            bzero(&(shmem->users[shmem->no_users]), sizeof(user_info));
            return 0;
        }
    }
    return -1;
}

int process_command(int admin_fd_socket, char instruction[])
{
    char* command;
    // memset(command, 0, 256);
    command = strtok(instruction, " \r\n");
    printf("Command: \"%s\"\n", command);
    if (strcmp(command, "LIST") == 0)
    {
        if(shmem->no_users==0) {
            wrong_command(admin_fd_socket,"No users in database\n");
            return 2;
        }
        char msg[]="User-id IP Password Cliente-Servidor P2P Grupo\n";
        write(admin_fd_socket, msg, strlen(msg));
        for(int i = 0; i<shmem->no_users; i++) {
            print_user_info(admin_fd_socket, &(shmem->users[i]));
        }
        return 0;
    }
    else if (strcmp(command, "ADD") == 0)
    {
        // * ADD <User-id> <IP> <Password> <Cliente-Servidor> <P2P> <Grupo>
        user_info new;
        char *next_token = strtok(NULL, " \r\n");
        if (next_token == NULL)
        {
            //wrong command
            wrong_command(admin_fd_socket,"Use ADD <User-id> <IP> <Password> <Cliente-Servidor> <P2P> <Grupo>\n");
            return 2;
        }
        strcpy(new.userId, next_token); // ID
        next_token = strtok(NULL, " \r\n");
        if (next_token == NULL) // IP
        {
            //wrong command
            wrong_command(admin_fd_socket,"Use ADD <User-id> <IP> <Password> <Cliente-Servidor> <P2P> <Grupo>\n");
            return 2;
        }
        inet_pton(AF_INET, next_token, &(new.ip_address.sin_addr));
        new.ip_address.sin_family = AF_INET;
        //strcpy(new.ip, next_token); // ip
        next_token = strtok(NULL, " \r\n");
        if (next_token == NULL)
        {
            //wrong command
            wrong_command(admin_fd_socket,"Use ADD <User-id> <IP> <Password> <Cliente-Servidor> <P2P> <Grupo>\n");
            return 2;
        }
        strcpy(new.password, next_token);
        unsigned char perm = 0;
        for (int i = 2; i >= 0; i--)
        {
            next_token = strtok(NULL, " \r\n");
            if (next_token == NULL)
            {
                //wrong command
                wrong_command(admin_fd_socket,"Wrond command. Use: ADD <User-id> <IP> <Password> <Cliente-Servidor> <P2P> <Grupo>\n");
                return 2;
            }
            if (strcasecmp(next_token, "YES") == 0)
            {
                perm |= (1 << i);
            }
            else if (strcasecmp(next_token, "NO") == 0)
            {
            }
            else
            {
                wrong_command(admin_fd_socket, "Use yes or no for permissions!\n");
                return 2;
            }
        }
        if (strtok(NULL, " \n\r")!= NULL){
             wrong_command(admin_fd_socket, "Too much information\n");
            return 2;
        }
        new.permissions = perm;
        // write "User added:"
        for(int i = 0; i<shmem->no_users;i++) {
            if(strcmp(new.userId , shmem->users[i].userId)==0) {
                wrong_command(admin_fd_socket, "The user you are trying to add already exists!\n");
                return 2;
            }
        }
        shmem->users[shmem->no_users++]=new;
        char* msg = "User added successfully:\n";
        write(admin_fd_socket, msg, strlen(msg));
        print_user_info(admin_fd_socket, &new);
        return 0;
    }
    else if (strcmp(command, "DEL") == 0)
    {
        //strtok
        char *next_token = strtok(NULL, " \r\n");
        if (next_token == NULL)
        {
            //wrong command
            wrong_command(admin_fd_socket,"Use ADD <User-id> <IP> <Password> <Cliente-Servidor> <P2P> <Grupo>\n");
            return 2;
        }

        if (remove_from_list(next_token)==-1){
            wrong_command(admin_fd_socket, "UderId not found");
        }
        char* msg = "User deleted successfully...\n";
        write(admin_fd_socket, msg, strlen(msg));
        return 0;
    }
    else if (strcmp(command, "QUIT") == 0)
    {
        // leave
        char* msg = "Quitting...\n";
        write(admin_fd_socket, msg, strlen(msg));
        return 1;
    }
    else
    {
        wrong_command(admin_fd_socket,"Use ADD, LIST, DEL or QUIT!\n");
        //wrong command
        return 2;
    }
}




void receive_admin()
{

    struct sockaddr_in server_addr, admin_addr;
    size_t admin_addr_size;
    //  signal(SIGINT, cleanup);

    bzero((void *)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    inet_pton(AF_INET, SERVER_IP, &(server_addr.sin_addr));
    server_addr.sin_port = htons(config_port);

    // for reading administration commands -> INITIALISE TCP SOCKET
    int admin_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (admin_fd < 0)
        error_msg("Problems obtaining TCP socket for admin!\n");

    if (bind(admin_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
        error_msg("na funcao bind");
    //only one admin
    if (listen(admin_fd, 2) < 0)
        error_msg("na funcao listen");
    admin_addr_size = sizeof(admin_addr);


    //char ip_server_tcp[INET_ADDRSTRLEN];

    // ! inet_ntop(AF_INET, &((amdin_addr.sin_addr)), ip, INET_ADDRSTRLEN);
    printf("Server accepting connections in (IP:port) %s:%d\n", SERVER_IP,config_port);

    // while (1) // supõe-se que
    int ret;
    int admin_socket_fd;
    admin_socket_fd = accept(admin_fd, (struct sockaddr *)&(admin_addr), (socklen_t *)&admin_addr_size);
    char buffer[256];
    do {
        while (waitpid(-1, NULL, WNOHANG) > 0)
            ;
        int g = read(admin_socket_fd, buffer, 256);
        buffer[g]='\0';
        ret = process_command(admin_socket_fd, buffer);
        printf("ret = %d\n", ret);
    } while(ret !=1);
    write_info_to_file();
    // accept commands
}



void write_info_to_file(){
    FILE * config= fopen (CONFIG_FILE, "w");
    char ip[16];
    printf("Number of users: %d\n", shmem->no_users);
    for (int i=0; i<shmem->no_users; ++i){
        inet_ntop( AF_INET,&((shmem->users[i].ip_address.sin_addr)), ip, INET_ADDRSTRLEN );
        fprintf(config, "%s %s %s %d\n", shmem->users[i].userId, ip, shmem->users[i].password, (int) shmem->users[i].permissions );
    }
    fclose(config);
}
