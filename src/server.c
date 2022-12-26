#include "server.h"
#include "response.h"
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
// TODO: Get rid of these, this is a bad practice lol
User *users[MAX_CLIENTS];
Channel *channels[MAX_CHANNELS];
pthread_mutex_t users_mutex;
pthread_mutex_t channels_mutex;
unsigned int user_count = 0;
unsigned int user_id = 0;

int main(int argc, char *argv[])
{
    int socket_fd, client_fd;
    char ip[INET6_ADDRSTRLEN], username[USERNAME_MAX_LENGTH];
    struct sockaddr_storage client_address;
    socklen_t client_address_size = sizeof(client_address);

    socket_fd = setup_server(ADDRESS, PORT);

    // Initialize mutex
    pthread_mutex_init(&users_mutex, NULL);
    pthread_mutex_init(&channels_mutex, NULL);

    while (true) {
        // Wait for clients to connect and accept them
        client_fd = accept(socket_fd, (struct sockaddr *)&client_address, &client_address_size);
        check_error(client_fd, __LINE__);

        // Check if max clients is reached
        // TODO: Make this more robust
        if ((user_count + 1) == MAX_CLIENTS) {
            printf("Max clients reached.\n");
            close(client_fd);
            continue;
        }

        // Add new user to list
        User *client = (User *)malloc(sizeof(User));
        client->file_descriptor = client_fd;
        client->id = user_id++;
        strcpy(client->hostname, "localhost");
        client->is_registered = false;
        client->is_away = false;
        add_user(client);

        // Add client to the queue and fork thread
        pthread_t child;
        pthread_create(&child, NULL, (void *)handle_client, (void *)client);

        // Wait one second to alleviate stress from CPU
        // sleep(1);
    }

    // Destroy mutex
    pthread_mutex_destroy(&users_mutex);
    pthread_mutex_destroy(&channels_mutex);
    // Close socket
    close(socket_fd);

    return EXIT_SUCCESS;
}

int setup_server(char *address, char *port)
{
    int status, socket_fd, client_fd, yes = 1;
    struct addrinfo hints, *server_info, *p;

    memset(&hints, 0, sizeof(hints)); // Initialize addrinfo to all zeros
    hints.ai_family = AF_UNSPEC;      // We don't care if it's IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;  // TCP socket
    hints.ai_flags = AI_PASSIVE;      // Fill in IP for us

    // Call getaddrinfo to populate results into `server_info`
    if ((status = getaddrinfo(address, port, &hints, &server_info)) != 0) {
        fprintf(stderr, "[ERROR] getaddrinfo error: %s\n", gai_strerror(status));
        exit(EXIT_FAILURE);
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
            exit(EXIT_FAILURE);
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
        exit(EXIT_FAILURE);
    }

    // We're done with this struct
    freeaddrinfo(server_info);

    // Listen for incoming connections
    if (listen(socket_fd, BACKLOG) < 0) {
        fprintf(stderr, "[ERROR] listen error: %s\n", strerror(errno));
        close(socket_fd);
        exit(EXIT_FAILURE);
    }

    printf("[INFO] Server is listening on port %s.\n", PORT);

    return socket_fd;
}

void add_user(User *user)
{
    pthread_mutex_lock(&users_mutex);
    // Add `client` to the first empty spot in the list
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (!users[i]) {
            users[i] = user;
            break;
        }
    }
    pthread_mutex_unlock(&users_mutex);
}

void remove_user(int id)
{
    pthread_mutex_lock(&users_mutex);
    // Search for client and remove it from the list
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (users[i] && users[i]->id == id) {
            // printf("client %d has left\n", clients_list[i]->id);
            users[i] = NULL;
            break;
        }
    }
    pthread_mutex_unlock(&users_mutex);
}

void handle_client(void *arg)
{
    User *user = (User *)arg;
    char username[USERNAME_MAX_LENGTH];
    char message[MESSAGE_LENGTH];
    bool disconnect = false;
    int bytes;
    user_count++;

    while (true) {
        // Read message from client
        if ((bytes = recv(user->file_descriptor, message, MESSAGE_LENGTH, 0)) < 0) {
            printf("[ERROR] Failed to get message from client %d: %s\n", user->id, strerror(errno));
            break;
        } else if (bytes == 0) {
            break;
        }

        // Parse message from client
        if (handle_message(message, user)) {
            break;
        }

        memset(message, 0, MESSAGE_LENGTH);
    }

    // Remove client and kill thread
    close(user->file_descriptor);
    remove_user(user->id);
    free(user);

    // Surround decrement of user count with mutex because two users could be leaving at the same
    // time
    pthread_mutex_lock(&users_mutex);
    // printf("User %d has left. There are %d users left.\n", user->id, --user_count);
    user_count--;
    pthread_mutex_unlock(&users_mutex);

    pthread_detach(pthread_self());
}

bool channel_exists(char *channel_name)
{
    bool exists = false;
    pthread_mutex_lock(&channels_mutex);
    for (int i = 0; i < MAX_CHANNELS; i++) {
        if (channels[i] && strcmp(channels[i]->name, channel_name) == 0) {
            exists = true;
            break;
        }
    }
    pthread_mutex_unlock(&channels_mutex);
    return exists;
}

bool nickname_exists(char *nickname)
{
    bool exists = false;
    pthread_mutex_lock(&users_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (users[i] && strcmp(users[i]->nickname, nickname) == 0) {
            exists = true;
            break;
        }
    }
    pthread_mutex_unlock(&users_mutex);
    return exists;
}

Channel *create_channel(char *name)
{
    Channel *channel = (Channel *)malloc(sizeof(Channel));
    strcpy(channel->name, name);
    return channel;
}

void add_channel(Channel *channel)
{
    pthread_mutex_lock(&channels_mutex);
    // Add `channel` to the first empty spot in the list
    for (int i = 0; i < MAX_CHANNELS; i++) {
        if (!channels[i]) {
            channels[i] = channel;
            break;
        }
    }
    pthread_mutex_unlock(&channels_mutex);
}

Channel *get_channel(char *name)
{
    pthread_mutex_lock(&channels_mutex);
    for (int i = 0; i < MAX_CHANNELS; i++) {
        if (channels[i] && strcmp(channels[i]->name, name) == 0) {
            pthread_mutex_unlock(&channels_mutex);
            return channels[i];
        }
    }
    pthread_mutex_unlock(&channels_mutex);
    return NULL;
}

User *get_user(char *nickname)
{
    pthread_mutex_lock(&users_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (users[i] && strcmp(users[i]->nickname, nickname) == 0) {
            pthread_mutex_unlock(&users_mutex);
            return users[i];
        }
    }
    pthread_mutex_unlock(&users_mutex);
    return NULL;
}

void send_excluding_user(char *message, int sender_id)
{
    pthread_mutex_lock(&users_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (users[i] && users[i]->id != sender_id) {
            if (send(users[i]->file_descriptor, message, strlen(message), 0) < 0) {
                fprintf(stderr,
                        "[ERROR] Failed to send message to client %d: %s\n",
                        users[i]->id,
                        strerror(errno));
                break;
            }
        }
    }
    pthread_mutex_unlock(&users_mutex);
}

void send_to_user(char *message, User *user)
{
    if (send(user->file_descriptor, message, strlen(message), 0) < 0) {
        fprintf(stderr,
                "[ERROR] Failed to send message to client %d: %s\n",
                user->id,
                strerror(errno));
    }
}

void send_to_channel(char *message, Channel *channel, User *sender)
{
    pthread_mutex_lock(&users_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (users[i] && users[i] != sender && users[i]->channel == channel) {
            if (send(users[i]->file_descriptor, message, strlen(message), 0) < 0) {
                fprintf(stderr,
                        "[ERROR] Failed to send message to client %d: %s\n",
                        users[i]->id,
                        strerror(errno));
                break;
            }
        }
    }
    pthread_mutex_unlock(&users_mutex);
}

int handle_message(char *message, User *user)
{
    char request[MESSAGE_LENGTH];
    char message_copy[MESSAGE_LENGTH];
    strcpy(message_copy, message);
    message_copy[strcspn(message_copy, "\r\n")] = '\0'; // Strip new-line from string

    // Print client's request to stdout for logging
    write(STDOUT_FILENO, message, strlen(message));

    char *parameters = message_copy;
    char *command = strsep(&parameters, " ");
    char origin[MESSAGE_LENGTH];
    snprintf(origin, MESSAGE_LENGTH, "%s!%s@%s", user->nickname, user->nickname, user->hostname);
    char *server_origin = "localhost";

    message[strcspn(message, "\r\n")] = '\0'; // Strip new-line from string

    bool disconnect = false;

    if (strcmp(command, "NICK") == 0) {
        // If there are no parameters, return `ERR_NONICKNAMEGIVEN`
        if (parameters == NULL) {
            snprintf(request,
                     MESSAGE_LENGTH,
                     ":%s %s :No nickname given\r\n",
                     server_origin,
                     ERR_NONICKNAMEGIVEN);
            send_to_user(request, user);
        } else {
            char *nickname = strsep(&parameters, " ");

            // Return an error if nickname is already taken
            if (nickname_exists(nickname)) {
                snprintf(request,
                         MESSAGE_LENGTH,
                         ":%s %s %s :Nickname is already in use\r\n",
                         "localhost",
                         ERR_NICKNAMEINUSE,
                         nickname);
                send_to_user(request, user);
            } else {
                if (user->is_registered) {
                    // If the user is already registered, then tell everyone else that they changed
                    // their nickname

                    snprintf(request,
                             MESSAGE_LENGTH,
                             ":%s %s :%s has changed their name to %s\r\n",
                             origin,
                             message,
                             user->nickname,
                             nickname);
                    send_excluding_user(request, user->id);
                    send_to_user(request, user);
                } else {
                    // If the user is not registered, make them registered and send them a welcome
                    // message
                    snprintf(request,
                             MESSAGE_LENGTH,
                             ":%s %s :Welcome to the Internet Relay Network, %s\r\n",
                             server_origin,
                             RPL_WELCOME,
                             nickname);
                    user->is_registered = true;
                    send_to_user(request, user);
                }
                strcpy(user->nickname, nickname);
            }
        }

    } else if (strcmp(command, "JOIN") == 0) {
        // char *channel;
        // Search through channels
        if (parameters == NULL) {
            snprintf(request,
                     MESSAGE_LENGTH,
                     ":%s %s :Not enough parameters\r\n",
                     server_origin,
                     ERR_NEEDMOREPARAMS);
            send_to_user(request, user);
        } else {
            if (!user->is_registered) {
                snprintf(request,
                         MESSAGE_LENGTH,
                         ":%s %s :You have not registered\r\n",
                         server_origin,
                         ERR_NOTREGISTERED);
                send_to_user(request, user);
            } else {
                char *channel_name = strsep(&parameters, " ");

                snprintf(request, MESSAGE_LENGTH, ":%s %s\r\n", origin, message);

                if (channel_exists(channel_name)) {
                    Channel *channel = get_channel(channel_name);
                    user->channel = channel;

                    send_to_channel(request, channel, user);
                    send_to_user(request, user);
                } else {
                    Channel *channel = create_channel(channel_name);
                    user->channel = channel;
                    add_channel(channel);
                    send_to_user(request, user);
                }
            }
        }
    } else if (strcmp(command, "AWAY") == 0) {
        snprintf(request, MESSAGE_LENGTH, ":%d %s\r\n", user->id, message);

        if (user->is_registered) {
            // Toggle away
            if (user->is_away) {
                // Reply with RPL_UNAWAY
                snprintf(request,
                         MESSAGE_LENGTH,
                         ":%s %s :You are no longer marked as being away\r\n",
                         server_origin,
                         RPL_UNAWAY);
                send_to_user(request, user);
                user->is_away = false;
            } else {
                // Reply with RPL_NOWAWAY
                snprintf(request,
                         MESSAGE_LENGTH,
                         ":%s %s :You have been marked as being away\r\n",
                         server_origin,
                         RPL_NOWAWAY);
                send_to_user(request, user);
                user->is_away = true;
            }

            if (user->channel != NULL) {
                snprintf(request,
                         MESSAGE_LENGTH,
                         ":%s AWAY :%s has been marked as away\r\n",
                         origin,
                         user->nickname);
                send_to_channel(request, user->channel, user);
            }
        } else {
            snprintf(request,
                     MESSAGE_LENGTH,
                     ":%s %s :You have not registered\r\n",
                     server_origin,
                     ERR_NOTREGISTERED);
            send_to_user(request, user);
        }

        // Add origin to message and relay it to all other users for now
        // send_excluding_user(request, user->id);
    } else if (strcmp(command, "LIST") == 0) {
        // snprintf(request, MESSAGE_LENGTH, ":%s %s\r\n", origin, message);

        if (!user->is_registered) {
            snprintf(request,
                     MESSAGE_LENGTH,
                     ":%s %s :You have not registered\r\n",
                     server_origin,
                     ERR_NOTREGISTERED);
            send_to_user(request, user);
        } else {
            pthread_mutex_lock(&channels_mutex);
            for (int i = 0; i < MAX_CHANNELS; i++) {
                if (channels[i]) {
                    int num_users = 0;
                    pthread_mutex_lock(&users_mutex);
                    for (int j = 0; j < MAX_CLIENTS; j++) {
                        if (users[j] && users[j]->channel == channels[i]) {
                            num_users++;
                        }
                    }
                    pthread_mutex_unlock(&users_mutex);

                    snprintf(request,
                             MESSAGE_LENGTH,
                             ":%s %s %s %d\r\n",
                             server_origin,
                             RPL_LIST,
                             channels[i]->name,
                             num_users);
                    send_to_user(request, user);
                }
            }
            pthread_mutex_unlock(&channels_mutex);

            snprintf(request,
                     MESSAGE_LENGTH,
                     ":%s %s :End of LIST\r\n",
                     server_origin,
                     RPL_LISTEND);
            send_to_user(request, user);
        }

    } else if (strcmp(command, "KICK") == 0) {
        if (parameters == NULL) {
            snprintf(request,
                     MESSAGE_LENGTH,
                     ":%s %s :Not enough parameters\r\n",
                     server_origin,
                     ERR_NEEDMOREPARAMS);
            send_to_user(request, user);
        } else {
            if (!user->is_registered) {
                snprintf(request,
                         MESSAGE_LENGTH,
                         ":%s %s :You have not registered\r\n",
                         server_origin,
                         ERR_NOTREGISTERED);
                send_to_user(request, user);
            } else {
                char *channel_name = strsep(&parameters, " ");
                char *target = strsep(&parameters, " ");

                if (target == NULL) {
                    snprintf(request,
                             MESSAGE_LENGTH,
                             ":%s %s :Not enough parameters\r\n",
                             server_origin,
                             ERR_NEEDMOREPARAMS);
                    send_to_user(request, user);
                } else {
                    if (channel_exists(channel_name)) {
                        Channel *channel = get_channel(channel_name);

                        if (user->channel == channel) {
                            if (nickname_exists(target)) {
                                User *target_user = get_user(target);

                                if (target_user->channel == channel) {
                                    snprintf(request,
                                             MESSAGE_LENGTH,
                                             ":%s PART %s\r\n",
                                             origin,
                                             target_user->nickname);
                                    send_to_channel(request, channel, user);
                                    send_to_user(request, user);

                                    target_user->channel = NULL;
                                } else {
                                    // ERR_USERNOTINCHANNEL
                                    snprintf(request,
                                             MESSAGE_LENGTH,
                                             ":%s %s :%s is not on that channel\r\n",
                                             server_origin,
                                             ERR_USERNOTINCHANNEL,
                                             target);
                                    send_to_user(request, user);
                                }
                            } else {
                                // ERR_NOSUCHNICK
                                snprintf(request,
                                         MESSAGE_LENGTH,
                                         ":%s %s :No such nick\r\n",
                                         server_origin,
                                         ERR_NOSUCHNICK);
                                send_to_user(request, user);
                            }
                        } else {
                            // ERR_NOTONCHANNEL
                            snprintf(request,
                                     MESSAGE_LENGTH,
                                     ":%s %s :You're not on that channel\r\n",
                                     server_origin,
                                     ERR_NOTONCHANNEL);
                            send_to_user(request, user);
                        }
                    } else {
                        // ERR_NOSUCHCHANNEL
                        snprintf(request,
                                 MESSAGE_LENGTH,
                                 ":%s %s :No such channel\r\n",
                                 server_origin,
                                 ERR_NOSUCHCHANNEL);
                        send_to_user(request, user);
                    }
                }
            }
        }
    } else if (strcmp(command, "PART") == 0) {
        if (parameters == NULL) {
            snprintf(request,
                     MESSAGE_LENGTH,
                     ":%s %s :Not enough parameters\r\n",
                     server_origin,
                     ERR_NEEDMOREPARAMS);
            send_to_user(request, user);
        } else {
            if (!user->is_registered) {
                snprintf(request,
                         MESSAGE_LENGTH,
                         ":%s %s :You have not registered\r\n",
                         server_origin,
                         ERR_NOTREGISTERED);
                send_to_user(request, user);
            } else {
                char *channel_name = strsep(&parameters, " ");
                if (!channel_exists(channel_name)) {
                    snprintf(request,
                             MESSAGE_LENGTH,
                             ":%s %s :No such channel\r\n",
                             server_origin,
                             ERR_NOSUCHCHANNEL);
                    send_to_user(request, user);
                } else {
                    Channel *channel = get_channel(channel_name);
                    if (user->channel != channel) {
                        snprintf(request,
                                 MESSAGE_LENGTH,
                                 ":%s %s :You're not on that channel\r\n",
                                 server_origin,
                                 ERR_NOTONCHANNEL);
                        send_to_user(request, user);
                    } else {
                        snprintf(request,
                                 MESSAGE_LENGTH,
                                 ":%s PART %s\r\n",
                                 origin,
                                 user->nickname);
                        send_to_channel(request, user->channel, user);
                        send_to_user(request, user);
                        user->channel = NULL;
                    }
                }
            }
        }

        // Add origin to message and relay it to all other users for now
        // send_excluding_user(request, user->id);
    } else if (strcmp(command, "PRIVMSG") == 0) {
        if (parameters == NULL) {
            snprintf(request,
                     MESSAGE_LENGTH,
                     ":%s %s :Not enough parameters\r\n",
                     server_origin,
                     ERR_NEEDMOREPARAMS);
            send_to_user(request, user);
        } else {
            if (!user->is_registered) {
                snprintf(request,
                         MESSAGE_LENGTH,
                         ":%s %s :You have not registered\r\n",
                         server_origin,
                         ERR_NOTREGISTERED);
                send_to_user(request, user);
            } else {
                char *target = strsep(&parameters, " ");
                if (target[0] == ':') {
                    // This means a target was not sent as ":" is the start of a message

                    // // If the user is not part of a channel, send them a different error message
                    // if (!user->channel) {

                    // }
                    snprintf(request,
                             MESSAGE_LENGTH,
                             ":%s %s :No recipient given\r\n",
                             server_origin,
                             ERR_NORECIPIENT);
                    send_to_user(request, user);
                } else if (target[0] == '#') {
                    if (parameters == NULL) {
                        snprintf(request,
                                 MESSAGE_LENGTH,
                                 ":%s %s :No text to send\r\n",
                                 server_origin,
                                 ERR_NOTEXTTOSEND);
                        send_to_user(request, user);
                    } else if (channel_exists(target)) {
                        snprintf(request, MESSAGE_LENGTH, ":%s %s\r\n", origin, message);
                        Channel *channel = get_channel(target);
                        send_to_channel(request, channel, user);
                    } else {
                        snprintf(request,
                                 MESSAGE_LENGTH,
                                 ":%s %s :No such channel\r\n",
                                 server_origin,
                                 ERR_NOSUCHCHANNEL);
                        send_to_user(request, user);
                    }
                } else {
                    if (parameters == NULL) {
                        snprintf(request,
                                 MESSAGE_LENGTH,
                                 ":%s %s :No text to send\r\n",
                                 server_origin,
                                 ERR_NOTEXTTOSEND);
                        send_to_user(request, user);
                    } else if (nickname_exists(target)) {
                        User *target_user = get_user(target);
                        if (!target_user->is_away) {
                            snprintf(request, MESSAGE_LENGTH, ":%s %s\r\n", origin, message);
                            send_to_user(request, target_user);
                        } else {
                            snprintf(request,
                                     MESSAGE_LENGTH,
                                     ":%s %s :%s is currently away\r\n",
                                     server_origin,
                                     RPL_AWAY,
                                     target);
                            send_to_user(request, user);
                        }
                    } else {
                        snprintf(request,
                                 MESSAGE_LENGTH,
                                 ":%s %s :No such nick\r\n",
                                 server_origin,
                                 ERR_NOSUCHNICK);
                        send_to_user(request, user);
                    }
                }
            }
        }
    } else if (strcmp(command, "QUIT") == 0) {
        snprintf(request, MESSAGE_LENGTH, ":%s %s\r\n", origin, message);
        send_excluding_user(request, user->id);
        // send_to_user(request, user);

        disconnect = true;
    } else {
        snprintf(request,
                 MESSAGE_LENGTH,
                 ":%s %s :Unknown command\r\n",
                 "localhost",
                 ERR_UNKNOWNCOMMAND);
        send_to_user(request, user);
    }

    // Also relay request to stdout for logging
    write(STDOUT_FILENO, request, strlen(request));

    return disconnect;
}
