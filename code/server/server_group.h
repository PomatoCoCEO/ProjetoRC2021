#ifndef SERVER_GROUP_H
#define SERVER_GROUP_H
void handle_group(int pos, msg_t* ans, int sock_fd);
int find_group(char group[]);
#endif