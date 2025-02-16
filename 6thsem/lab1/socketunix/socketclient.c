#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "socketcs.h"

int socketfd;

void sigint_handler(int signum) {
    printf("Got singal %d.\n", signum);
    if (shutdown(socketfd, SHUT_RDWR) == -1) {
        perror("shutdown()");
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <num1> <op> <num2>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    calculate_in in;
    in.a = strtod(argv[1], NULL);
    in.b = strtod(argv[3], NULL);
    switch (argv[2][0]) {
        case '+':
            in.operation = ADD;
            break;
        case '-':
            in.operation = SUB;
            break;
        case '*':
            in.operation = MUL;
            break;
        case '/':
            in.operation = DIV;
            break;
        default:
            fprintf(stderr, "Invalid operation: %s\n", argv[2]);
            exit(EXIT_FAILURE);
    }

    if (signal(SIGINT, sigint_handler) == -1) {
        perror("signal()");
        exit(EXIT_FAILURE);
    }

    socketfd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (socketfd == -1) {
        perror("socket()");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_un clsa;
    memset(&clsa, 0, sizeof(clsa));
    clsa.sun_family = AF_UNIX;
    snprintf(clsa.sun_path, sizeof(clsa.sun_path), "%d_cli.socket", getpid());
    if (bind(socketfd, (struct sockaddr *) &clsa, sizeof(clsa)) == -1) {
        perror("bind()");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_un sa;
    memset(&sa, 0, sizeof(sa));
    sa.sun_family = AF_UNIX;
    strncpy(sa.sun_path, SOCKET_NAME, sizeof(sa.sun_path) - 1);

    if (sendto(socketfd, &in, sizeof(in), 0, (struct sockaddr *) &sa, sizeof(sa)) == -1) {
        perror("sendto()");
        exit(EXIT_FAILURE);
    }
    calculate_out out;
    int size = recvfrom(socketfd, &out, sizeof(out), 0, NULL, NULL);
    if (size == -1) {
        perror("recvfrom()");
        exit(EXIT_FAILURE);
    } else if (size == 0) {
        printf("Socket shutdown.\n");
    } else {
        if (out.error == OK) {
        printf("%.2f %c %.2f = %.2f\n", in.a, argv[2][0], in.b, out.result);
        close(socketfd);
        exit(EXIT_SUCCESS);
        }
    }

    if (close(socketfd) == -1) {
        perror("close()");
        exit(EXIT_FAILURE);
    }
    if (unlink(clsa.sun_path) == -1) {
        perror("unlink()");
        exit(EXIT_FAILURE);
    }
}