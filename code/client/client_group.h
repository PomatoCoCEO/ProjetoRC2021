#ifndef CLIENT_GROUP_H
#define CLIENT_GROUP_H

int find_group_pos(char* name);
int menu_group_comm();
void * read_group_messages(void * p);
int join_group();
int send_group_msg();
int group_comm();


#endif