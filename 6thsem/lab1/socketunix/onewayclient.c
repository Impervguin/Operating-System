#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "oneway.h"

int main() {
    int socketfd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (socketfd == -1) {
        perror("socket()");
        exit(EXIT_FAILURE);
    }
    struct sockaddr_un sa;
    memset(&sa, 0, sizeof(sa));
    sa.sun_family = AF_UNIX;
    strncpy(sa.sun_path, SOCKET_NAME, sizeof(sa.sun_path) - 1);
    char in[MAX_MESSAGE_SIZE];
    snprintf(in, MAX_MESSAGE_SIZE, "%d", getpid());
    if (sendto(socketfd, &in, sizeof(in), 0, (struct sockaddr *) &sa, sizeof(sa)) == -1) {
        perror("sendto()");
        exit(EXIT_FAILURE);
    }
    printf("send: %s\n", in);
    close(socketfd);
}