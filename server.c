#include "server.h"
#include "util.h"
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

// Global variables
Client *client_list[MAX_CLIENTS];
pthread_mutex_t client_mutex;
unsigned int client_count = 0;
unsigned int client_id = 0;

int main(int argc, char *argv[])
{
    int status, socket_fd, client_fd, yes = 1;
    struct addrinfo hints, *server_info, *p;
    struct sockaddr_storage client_address;
    socklen_t sin_size = sizeof(client_address);
    char ip[INET_ADDRSTRLEN]; // string to hold ipv4 address
    char username[USERNAME_MAX_LENGTH];

    memset(&hints, 0, sizeof(hints)); // Initialize addrinfo to all zeros
    hints.ai_family = AF_UNSPEC;      // We don't care if it's IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;  // TCP socket
    hints.ai_flags = AI_PASSIVE;      // Fill in IP for us

    // Call getaddrinfo to populate results into `server_info`
    if ((status = getaddrinfo(ADDRESS, PORT, &hints, &server_info)) != 0) {
        fprintf(stderr, "[ERROR] getaddrinfo error: %s\n", gai_strerror(status));
        exit(1);
    }

    // Search for the correct server info in the linked list and bind to it
    for (p = server_info; p != NULL; p = p->ai_next) {
        if ((socket_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0) {
            fprintf(stderr, "[WARN] socket error: %s\n", strerror(errno));
            continue;
        }

        // Clear the pesky "Address already in use" error message
        if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0) {
            fprintf(stderr, "[WARN] setsockopt error: %s\n", strerror(errno));
            exit(1);
        }

        if (bind(socket_fd, p->ai_addr, p->ai_addrlen) < 0) {
            close(socket_fd);
            fprintf(stderr, "[WARN] bind error: %s\n", strerror(errno));
            continue;
        }

        break;
    }

    // If we're at the end of the list, we failed to bind
    if (p == NULL) {
        fprintf(stderr, "[ERROR] Failed to bind.\n");
        exit(1);
    }

    // We're done with this struct
    freeaddrinfo(server_info);

    // Listen for incoming connections
    if (listen(socket_fd, BACKLOG) < 0) {
        fprintf(stderr, "[ERROR] listen error: %s\n", strerror(errno));
        close(socket_fd);
        exit(1);
    }

    printf("[INFO] Server is listening on port %s.\n", PORT);

    // Initialize mutex
    if (pthread_mutex_init(&client_mutex, NULL) < 0) {
        fprintf(stderr, "[ERROR] Failed to initialize mutex: %s\n", strerror(errno));
        exit(1);
    }

    while (1) {
        client_fd = accept(socket_fd, (struct sockaddr *)&client_address, &sin_size);
        if (client_fd < 0) {
            fprintf(stderr, "[WARN] accept error: %s\n", strerror(errno));
        }

        // Print client ip address
        inet_ntop(client_address.ss_family,
                  get_in_addr((struct sockaddr *)&client_address),
                  ip,
                  sizeof(ip));
        printf("[INFO] Accepted connection from client with IP %s.\n", ip);

        // Check if max clients is reached
        if ((client_count + 1) == MAX_CLIENTS) {
            printf("Max clients reached.");
            close(client_fd);
            continue;
        }

        // Client settings
        Client *client = (Client *)malloc(sizeof(Client));
        client->fd = client_fd;
        client->id = client_id++;

        // Add client to the queue and fork thread
        pthread_t child;
        add_client(client);
        pthread_create(&child, NULL, (void *)handle_client, (void *)client);

        // Wait one second to alleviate stress from CPU
        sleep(1);
    }

    // Destroy mutex
    pthread_mutex_destroy(&client_mutex);
    // Close socket
    close(socket_fd);
}

void add_client(Client *client)
{
    pthread_mutex_lock(&client_mutex);
    // Add `client` to the first empty spot in the list
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (!client_list[i]) {
            client_list[i] = client;
            break;
        }
    }
    pthread_mutex_unlock(&client_mutex);
}

void remove_client(int id)
{
    pthread_mutex_lock(&client_mutex);
    // Search for client and remove it from the list
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_list[i] && client_list[i]->id == id) {
            // printf("client %d has left\n", clients_list[i]->id);
            client_list[i] = NULL;
            break;
        }
    }
    pthread_mutex_unlock(&client_mutex);
}

void send_to_clients(char *message, int id_to_exclude)
{
    pthread_mutex_lock(&client_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_list[i] && client_list[i]->id != id_to_exclude) {
            if (send(client_list[i]->fd, message, strlen(message), 0) < 0) {
                fprintf(stderr,
                        "[ERROR] Failed to send message to client %d: %s\n",
                        client_list[i]->id,
                        strerror(errno));
                break;
            }
            // printf("[INFO] Sent client %d: %s", clients[i]->id, message);
        }
    }
    pthread_mutex_unlock(&client_mutex);
}

void handle_client(void *arg)
{
    Client *client = (Client *)arg;
    char username[USERNAME_MAX_LENGTH];
    char message[MESSAGE_LENGTH];
    bool disconnect = false;
    client_count++;
    printf("id: %d\n", client->id);

    // Get username from client
    if (recv(client->fd, username, USERNAME_MAX_LENGTH, 0) < 0) {
        fprintf(stderr, "[ERROR] Failed to read username: %s\n", strerror(errno));
        disconnect = true;
    } else {
        strcpy(client->username, username);
        // Tell other clients `client->username` has joined
        sprintf(message, "Welcome, %s!\n", username);
        printf("%s", message);
        send_to_clients(message, client->id);
    }
    memset(message, 0, MESSAGE_LENGTH);

    while (1) {
        if (disconnect) {
            break;
        }

        if (recv(client->fd, message, MESSAGE_LENGTH, 0) < 0) {
            printf("[ERROR] Failed to get message from client %d: %s\n",
                   client->id,
                   strerror(errno));
            disconnect = true;
        } else if (strcmp(message, DISCONNECT_SIGNAL) == 0) {
            memset(message, 0, MESSAGE_LENGTH);
            sprintf(message, "Bye, %s!\n", client->username);
            printf("%s", message);
            send_to_clients(message, client->id);
            disconnect = true;
        } else if (strlen(message) > 0) {
            char buffer[MESSAGE_LENGTH];
            sprintf(buffer, "<%s> %s\n", client->username, message);
            send_to_clients(buffer, client->id);
            printf("%s", buffer);
        }

        memset(message, 0, MESSAGE_LENGTH);
    }

    // Remove client and kill thread
    close(client->fd);
    remove_client(client->id);
    free(client);
    client_count--;
    pthread_detach(pthread_self());
}
