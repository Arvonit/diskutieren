#ifndef CLIENT_H
#define CLIENT_H

#include "util.h"

void send_handler(void *arg);
void receive_handler(void *arg);
void get_username(char buffer[USERNAME_MAX_LENGTH]);
int connect_to_server(char *address, char *port);
void handle_response(char *message, int socket_fd);
void handle_message(char *message, int socket_fd);

#endif
