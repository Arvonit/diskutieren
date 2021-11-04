#include "client.h"
#include "util.h"
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

// TODO:
// - Terminate sending thread when server has shut down

// Global variable to tell threads to exit
bool disconnect = false;

int main(int argc, char *argv[])
{
    int socket_fd;
    char username[USERNAME_MAX_LENGTH];

    // Ask for username
    get_username(username);

    // Connect to server
    socket_fd = connect_to_server(ADDRESS, PORT);
    printf("Connected to server.\n");

    // Send username to server
    if (send(socket_fd, username, sizeof(username), 0) < 0) {
        fprintf(stderr, "[ERROR] Failed to send username: %s\n", strerror(errno));
        close(socket_fd);
        exit(1);
    }

    // Spawn threads to send and recieve messages
    pthread_t send_thread, receive_thread;
    if (pthread_create(&send_thread, NULL, (void *)send_handler, &socket_fd) < 0) {
        perror("[ERROR] pthread error\n");
    }
    if (pthread_create(&receive_thread, NULL, (void *)receive_handler, &socket_fd) < 0) {
        perror("[ERROR] pthread error\n");
    }

    // Wait for threads to finish
    pthread_join(receive_thread, NULL);
    pthread_join(send_thread, NULL);

    close(socket_fd);
}

int connect_to_server(char *address, char *port)
{
    int status, socket_fd, yes = 1;
    struct addrinfo hints, *server_info, *p;
    char ip[INET6_ADDRSTRLEN];

    // Create a socket
    memset(&hints, 0, sizeof(hints)); // Initialize the struct to all zeros
    hints.ai_family = AF_UNSPEC;      // We don't care if it's IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;  // TCP socket

    // Call getaddrinfo to populate results into `server_info`
    if ((status = getaddrinfo(ADDRESS, PORT, &hints, &server_info)) != 0) {
        fprintf(stderr, "[ERROR] getaddrinfo error: %s\n", gai_strerror(status));
        exit(1);
    }

    // Search for correct server in linked list
    for (p = server_info; p != NULL; p = p->ai_next) {
        if ((socket_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0) {
            printf("[INFO] socket error: %s\n", strerror(errno));
            continue;
        }

        if (connect(socket_fd, p->ai_addr, p->ai_addrlen) < 0) {
            printf("[INFO] connect error: %s\n", strerror(errno));
            close(socket_fd);
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "[ERROR] Failed to connect\n");
        exit(1);
    }

    // Print out ip address that we connected to
    // inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), ip, sizeof(ip));
    // printf("[INFO] Connected to %s:%s.\n", ip, port);

    // We're done with this struct
    freeaddrinfo(server_info);

    return socket_fd;
}

void send_handler(void *arg)
{
    int socket_fd = *(int *)arg;

    while (1) {
        if (disconnect) {
            break;
        }
        size_t message_length = MESSAGE_LENGTH;
        char *message = (char *)malloc(sizeof(char) * message_length);
        printf("> ");
        fflush(stdout);
        getline(&message, &message_length, stdin);
        message[strcspn(message, "\n")] = '\0'; // Strip new-line from string

        // Send current message to server
        if (send(socket_fd, message, message_length, 0) < 0) {
            fprintf(stderr, "[ERROR] Failed to send message: %s\n", strerror(errno));
            free(message);
            break;
        }

        // Exit if message is the disconnect signal
        if (strcmp(message, DISCONNECT_SIGNAL) == 0) {
            free(message);
            break;
        }

        // Free string
        free(message);
    }

    disconnect = true;
    // printf("sender done.\n");
}

void receive_handler(void *arg)
{
    int socket_fd = *(int *)arg;
    // size_t message_length = MESSAGE_LENGTH;
    char message[MESSAGE_LENGTH];

    while (1) {
        int bytes = 0;
        if (disconnect) {
            break;
        }
        if ((bytes = recv(socket_fd, message, MESSAGE_LENGTH, 0)) < 0) {
            printf("\33[2K\r"); // Clear the current line
            fprintf(stderr, "[ERROR] Failed to recieve message: %s\n", strerror(errno));
            break;
        } else if (bytes == 0) {
            printf("\33[2K\r"); // Clear the current line
            // printf("Lost connection to server.\n");
            break;
        }
        printf("\33[2K\r"); // Clear the current line
        printf("%s", message);
        printf("> ");
        fflush(stdout);
        memset(message, 0, MESSAGE_LENGTH);
    }
    disconnect = true;
    // printf("receive done.\n");
}

void get_username(char buffer[USERNAME_MAX_LENGTH])
{
    while (1) {
        printf("Enter your username: ");
        char *username = NULL;
        size_t length = USERNAME_MAX_LENGTH;
        getline(&username, &length, stdin);
        username[strcspn(username, "\n")] = '\0'; // Strip new-line from username

        if (strlen(username) >= USERNAME_MIN_LENGTH && strlen(username) < USERNAME_MAX_LENGTH) {
            strcpy(buffer, username);
            free(username);
            break;
        }

        printf("Username must be less than %d and more than characters %d long. Please try "
               "again.\n",
               USERNAME_MAX_LENGTH,
               USERNAME_MIN_LENGTH);
        free(username);
    }
}
