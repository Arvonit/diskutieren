#include "client.h"
#include "response.h"
#include "util.h"
#include <arpa/inet.h>
#include <assert.h>
#include <editline/readline.h>
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

bool disconnect = false;
char channel[100] = "";
char nickname[100] = "";

int main(int argc, char *argv[])
{
    int socket_fd;
    // char username[USERNAME_MAX_LENGTH];

    // Get username from command-line argument
    if (argc != 2) {
        puts("Usage: irc <username>");
        exit(EXIT_FAILURE);
    }
    char *username = argv[1];

    // Ask for username
    // get_username(username);

    // Connect to server
    socket_fd = connect_to_server(ADDRESS, PORT);
    printf("Connected to server.\n");

    // Send nickname to server
    char request[MESSAGE_LENGTH];
    snprintf(request, MESSAGE_LENGTH, "NICK %s\r\n", username);
    check_error(send(socket_fd, request, MESSAGE_LENGTH, 0), __LINE__);

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

    return EXIT_SUCCESS;
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
        exit(EXIT_FAILURE);
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
        exit(EXIT_FAILURE);
    }

    // We're done with this struct
    freeaddrinfo(server_info);

    return socket_fd;
}

void send_handler(void *arg)
{
    int socket_fd = *(int *)arg;

    while (true) {
        if (disconnect) {
            break;
        }
        size_t message_length = MESSAGE_LENGTH;

        // char *message = (char *)malloc(sizeof(char) * message_length);
        // printf("> ");
        // fflush(stdout);
        // getline(&message, &message_length, stdin);
        // message[strcspn(message, "\n")] = '\0'; // Strip new-line from string

        char *message = readline("> ");

        handle_message(message, socket_fd);
        if (disconnect) {
            free(message);
        }

        // Add message to editline history
        add_history(message);

        // Free message string
        free(message);
    }

    disconnect = true;
}

void trim_trailing(char *str)
{
    int index, i;

    // Set default index
    index = -1;

    // Find last index of non-white space character
    i = 0;
    while (str[i] != '\0') {
        if (str[i] != ' ' && str[i] != '\t' && str[i] != '\n') {
            index = i;
        }

        i++;
    }

    // Mark next character to last non-white space character as NULL
    str[index + 1] = '\0';
}

void handle_message(char *message, int socket_fd)
{
    char request[MESSAGE_LENGTH];
    char message_copy[MESSAGE_LENGTH];
    trim_trailing(message);
    strcpy(message_copy, message);

    // If message is not a command, make the request a PRIVMSG
    if (message[0] != '/') {
        if (strcmp(channel, "")) {
            snprintf(request, MESSAGE_LENGTH, "PRIVMSG %s :%s\r\n", channel, message);
        } else {
            snprintf(request, MESSAGE_LENGTH, "PRIVMSG :%s\r\n", message);
        }
    } else {
        // Remove leading slash from command
        memmove(message_copy, message_copy + 1, strlen(message_copy));

        // Make the command capital
        char *parameters = message_copy;
        char *command = strsep(&parameters, " ");
        assert(command != NULL);
        strupr(command);

        if (strcmp(command, "QUIT") == 0) {
            disconnect = true;
        }

        // Format command properly
        if (parameters == NULL) {
            if (strcmp(command, "PART") == 0) {
                snprintf(request, MESSAGE_LENGTH, "%s %s\r\n", command, channel);
            } else {
                snprintf(request, MESSAGE_LENGTH, "%s\r\n", command);
            }
        } else {
            if (strcmp(command, "PRIVMSG") == 0) {
                char *target = strsep(&parameters, " ");
                char *msg = parameters;
                if (msg != NULL) {
                    snprintf(request, MESSAGE_LENGTH, "%s %s :%s\r\n", command, target, msg);
                } else {
                    snprintf(request, MESSAGE_LENGTH, "%s %s\r\n", command, target);
                }
            } else {
                snprintf(request, MESSAGE_LENGTH, "%s %s\r\n", command, parameters);
            }
        }
    }

    // Send request to server if message has content
    if (strcmp(message, "") != 0) {
        if (send(socket_fd, request, MESSAGE_LENGTH, 0) < 0) {
            fprintf(stderr, "[ERROR] Failed to send request: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
    }
}

void receive_handler(void *arg)
{
    int socket_fd = *(int *)arg;
    // size_t message_length = MESSAGE_LENGTH;
    char message[MESSAGE_LENGTH];
    int messages_received = 0;

    while (true) {
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

        handle_response(message, socket_fd);

        // // Clear the current line, print message from server, and restore prompt
        // printf("\33[2K\r"); // Clear the current line
        // printf("%s", message);
        // char *sub;
        //     printf("> ");
        // fflush(stdout);

        if (messages_received > 0) {
            printf("> ");
        }
        fflush(stdout);
        memset(message, 0, MESSAGE_LENGTH);

        if (messages_received == 0) {
            messages_received++;
        }
    }
    disconnect = true;
    // printf("receive done.\n");
}

void handle_response(char *message, int socket_fd)
{
    // if (message[0] == ':') {
    //     char *source = strtok(message, " ");
    // }
    // char *command = strtok(message, NULL);

    // // Parse respone and do things based on that
    // int response_code = -1;
    // char *response = "";
    // // If the response for a command the user entered is not 0, then there was something wrong
    // if (response_code != 0) {
    // }
    // // If a command doesn't have a response (-1), then it is simply a relay from the server
    // if (response_code == -1) {
    //     // Clear the current line, print message from server, and restore prompt
    //     printf("\33[2K\r"); // Clear the current line
    //     printf("%s", response);
    //     printf("> ");
    //     fflush(stdout);
    // }
    // // A successful response is silent in IRC
    // puts(message);

    char output[MESSAGE_LENGTH];
    char message_copy[MESSAGE_LENGTH];
    strcpy(message_copy, message);
    message_copy[strcspn(message_copy, "\r\n")] = '\0'; // Strip new-line from string

    char *parameters = message_copy;
    char *source = strsep(&parameters, " ");
    char *command = strsep(&parameters, " ");
    char *server_source = "localhost";

    if (is_reply(command) || is_error(command)) {
        // This is formatted differently
        if (strcmp(command, RPL_LIST) == 0) {
            char *channel = strsep(&parameters, " ");
            char *num_users = strsep(&parameters, " ");
            snprintf(output, MESSAGE_LENGTH, "[[SERVER]] %s %s\n", channel, num_users);
        } else if (strcmp(command, RPL_LISTEND) == 0) {
            strcpy(output, "---\n");
        } else if (strcmp(command, RPL_WELCOME) == 0) {
            char *text = parameters;
            char *ptr;
            char *nick;
            while ((ptr = strsep(&text, " "))) {
                nick = ptr;
            }
            strcpy(nickname, nick);
            snprintf(output,
                     MESSAGE_LENGTH,
                     "[[SERVER]] Welcome to the Internet Relay Network, %s!\n",
                     nickname);
        } else if (strcmp(command, ERR_NICKNAMEINUSE) == 0) {
            char *nick = strsep(&parameters, " ");
            snprintf(output, MESSAGE_LENGTH, "[[SERVER]] Nickname %s is already in use\n", nick);
        } else {
            char *text = parameters;
            memmove(text, text + 1, strlen(text)); // Remove ":" from the message
            snprintf(output, MESSAGE_LENGTH, "[[SERVER]] %s\n", text);
        }
    } else if (strcmp(command, "PRIVMSG") == 0) {
        char *sender = strsep(&source, "!");
        char *target = strsep(&parameters, " ");

        char *text = parameters;
        memmove(text, text + 1, strlen(text));       // Remove ":" from the message
        memmove(sender, sender + 1, strlen(sender)); // Remove ":" from the sender

        // Format DM's differently than channel messages
        if (target[0] == '#') {
            snprintf(output, MESSAGE_LENGTH, "<%s> %s\n", sender, text);
        } else {
            snprintf(output, MESSAGE_LENGTH, "[%s] %s\n", sender, text);
        }
    } else if (strcmp(command, "NICK") == 0) {
        char *sender = strsep(&source, "!");
        memmove(sender, sender + 1, strlen(sender)); // Remove ":" from the sender
        char *nick = strsep(&parameters, " ");
        // Update nickname
        if (strcmp(sender, nickname) == 0) {
            strcpy(nickname, nick);
            // puts(nickname);
        } else {
            snprintf(output,
                     MESSAGE_LENGTH,
                     "[[SERVER]] %s has changed their nickname to %s\n",
                     sender,
                     nick);
        }
    } else if (strcmp(command, "JOIN") == 0) {
        char *sender = strsep(&source, "!");
        memmove(sender, sender + 1, strlen(sender)); // Remove ":" from the sender
        if (strcmp(sender, nickname) != 0) {
            snprintf(output, MESSAGE_LENGTH, "[[SERVER]] %s has joined the channel\n", sender);
        } else {
            char *channel_name = strsep(&parameters, " ");
            strcpy(channel, channel_name);
            // puts("changed channel name");
        }
    } else if (strcmp(command, "AWAY") == 0) {
        char *sender = strsep(&source, "!");
        memmove(sender, sender + 1, strlen(sender)); // Remove ":" from the sender

        if (strcmp(sender, nickname) != 0) {
            snprintf(output, MESSAGE_LENGTH, "[[SERVER]] %s has been marked as away\n", sender);
        }
    } else if (strcmp(command, "PART") == 0) {
        // char *sender = strsep(&source, "!");
        char *target = strsep(&parameters, " ");
        if (strcmp(target, nickname) != 0) {
            snprintf(output, MESSAGE_LENGTH, "[[SERVER]] %s has left the channel\n", target);
        } else {
            strcpy(channel, "");
        }
    } else if (strcmp(command, "QUIT") == 0) {
        char *sender = strsep(&source, "!");
        memmove(sender, sender + 1, strlen(sender)); // Remove ":" from the sender

        snprintf(output, MESSAGE_LENGTH, "[[SERVER]] %s has left the server\n", sender);
    } else {
        message[strcspn(message, "\r\n")] = '\0'; // Strip new-line from string
        snprintf(output, MESSAGE_LENGTH, "[[SERVER]] %s\n", message);
        strcpy(output, message);
    }

    // if (strcmp(command, "JOIN") == 0) {
    //     channel = parameters;
    // } else if (strcmp(command, "PART") == 0) {
    //     channel = NULL;
    // }

    printf("\33[2K\r"); // Clear the current line
    printf("%s", output);
    memset(output, 0, strlen(output));
}
