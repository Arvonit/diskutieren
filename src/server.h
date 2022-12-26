#ifndef SERVER_H
#define SERVER_H

#include "util.h"
#include <stdbool.h>

#define BACKLOG 10
#define MAX_CLIENTS 100
#define MAX_CHANNELS 100

typedef struct Channel
{
    char name[CHANNEL_LENGTH];
} Channel;

/**
 * A structure that holds the information about a client.
 */
typedef struct User
{
    int file_descriptor;
    int id;
    bool is_registered;
    bool is_away;
    char name[USERNAME_MAX_LENGTH];
    char nickname[USERNAME_MAX_LENGTH];
    char hostname[USERNAME_MAX_LENGTH];
    Channel *channel;
} User;

/**
 * Create a server socket and listen for incoming connections.
 *
 * @param address The ip address of the server
 * @param port The port the server is listening on
 * @return The server socket's file descriptor
 */
int setup_server(char *address, char *port);

/**
 * Function for each thread to handle a client.
 */
void handle_client(void *arg);

void add_user(User *client);
void remove_user(int id);

/**
 * Send a message to all connected clients except the sender (`sender_id`).
 *
 * @param message The string to send
 * @param id_to_skip The id of the sender
 */
void send_excluding_user(char *message, int sender_id);

int handle_message(char *message, User *user);

Channel *create_channel(char *name);

#endif
