#ifndef SERVER_H
#define SERVER_H

#include "util.h"

#define BACKLOG 10
#define MAX_CLIENTS 100

typedef struct Client
{
    int fd;
    int id;
    char username[USERNAME_MAX_LENGTH];
} Client;

void handle_client(void *arg);
void add_client(Client *client);
void remove_client(int id);
void send_to_clients(char *message, int id_to_exclude);
void handle_client(void *arg);

#endif
