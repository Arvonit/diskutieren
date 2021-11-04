#ifndef CLIENT_H
#define CLIENT_H

#include "util.h"

typedef struct Server
{
    int fd;
} Server;

void send_handler(void *arg);
void receive_handler(void *arg);
void get_username(char buffer[USERNAME_MAX_LENGTH]);
int connect_to_server(char *address, char *port);

#endif
