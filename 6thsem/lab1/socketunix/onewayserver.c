#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "oneway.h"

int socketfd;

// void sigint_handler() {
//     if (close(socketfd) == -1) {
//         perror("close()");
//         exit(EXIT_FAILURE);
//     }
//     if (unlink(SOCKET_NAME) == -1) {
//         perror("unlink()");
//         exit(EXIT_FAILURE);
//     }
//     printf("Server stopped.\n");
//     exit(EXIT_SUCCESS);
// }

void sigalrm_handler(int signum) {
    // if (close(socketfd) == -1) {
    //     perror("close()");
    //     exit(EXIT_FAILURE);
    // }
    // if (unlink(SOCKET_NAME) == -1) {
    //     perror("unlink()");
    //     exit(EXIT_FAILURE);
    // }
    printf("Signal %d. Server stopped.\n", signum);
    return;
    // exit(EXIT_SUCCESS);
}

int main() {
    socketfd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (socketfd == -1) {
        perror("socket()");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_un sa;
    memset(&sa, 0, sizeof(sa));
    sa.sun_family = AF_UNIX;
    strncpy(sa.sun_path, SOCKET_NAME, sizeof(sa.sun_path) - 1);

    if (bind(socketfd, (struct sockaddr *) &sa, sizeof(sa)) == -1) {
        perror("bind()");
        exit(EXIT_FAILURE);
    }

    // if (signal(SIGINT, sigint_handler) == -1) {
    //     perror("signal()");
    //     exit(EXIT_FAILURE);
    // }

    if (signal(SIGALRM, sigalrm_handler) == -1) {
        perror("signal()");
        exit(EXIT_FAILURE);
    }
    alarm(2);

    for (;;) {
        char in[MAX_MESSAGE_SIZE];
        struct sockaddr sender;
        socklen_t sender_len = sizeof(sender);
        int size = recvfrom(socketfd, &in, sizeof(in), 0, NULL, NULL);
        printf("%d\n", size);
        if (size < 0) {
            perror("recvfrom()");
        } else {
            in[size] = '\0';
            printf("received: %s\n", in);
        }
    }
}