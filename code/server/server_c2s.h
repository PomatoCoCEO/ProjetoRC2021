#ifndef SERVER_C2S_H
#define SERVER_C2S_H
void handle_c2s(int pos, msg_t* ans, int sock_fd);
void send_msg_to_user(int pos, int pos_dest, msg_t * msg);
#endif