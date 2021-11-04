#include "util.h"
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

// void check_error(int status, int line)
// {
//     if (status < 0) {
//         printf("[ERROR] Line %d, Socket %d: %s\n", line, getpid(), strerror(errno));
//         exit(-1);
//     }
// }

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

// TODO: Verify that these two methods actually work, they've been a bit inconsistent
int send_all(int socket_fd, const void *buffer, size_t length, int flags)
{
    ssize_t n;
    const void *pointer = buffer;
    while (length > 0) {
        n = send(socket_fd, pointer, length, flags);
        if (n <= 0) {
            return -1;
        }
        pointer += n;
        length -= n;
    }
    return 0;
}

int receive_all(int socket_fd, void *buffer, size_t length, int flags)
{
    ssize_t n;
    void *pointer = buffer;
    while (length > 0) {
        n = recv(socket_fd, pointer, length, flags);
        if (n <= 0) {
            return -1;
        }
        pointer += n;
        length -= n;
    }
    return 0;
}
