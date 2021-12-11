#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>
#include <sys/socket.h>

#define ADDRESS "localhost"
#define PORT "6667"
#define USERNAME_MIN_LENGTH 2
#define USERNAME_MAX_LENGTH 64
#define CHANNEL_LENGTH 128
// #define MESSAGE_LENGTH 1024
// #define DISCONNECT_SIGNAL "$DISCONNECT"
// #define DEBUG 1
#define MESSAGE_LENGTH 1024

void check_error(int status, int line);
int send_all(int socket, const void *buffer, size_t length, int flags);
int receive_all(int socket_fd, void *buffer, size_t length, int flags);
void *get_in_addr(struct sockaddr *sa);
void strupr(char *string);
// int debug(const char *format, ...);

#endif
