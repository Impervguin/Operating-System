#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "socketcs.h"

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

    int socketfd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (socketfd == -1) {
        perror("socket()");
        exit(EXIT_FAILURE);
    }
    struct sockaddr_un sa;
    memset(&sa, 0, sizeof(sa));
    sa.sun_family = AF_UNIX;
    strncpy(sa.sun_path, SOCKET_NAME, sizeof(sa.sun_path) - 1);
    if (connect(socketfd, (struct sockaddr *) &sa, sizeof(sa)) == -1) {
        perror("connect()");
        exit(EXIT_FAILURE);
    }
    if (write(socketfd, &in, sizeof(in)) == -1) {
        perror("write()");
        exit(EXIT_FAILURE);
    }
    calculate_out out;
    int size = read(socketfd, &out, sizeof(out));
    if (size == -1) {
        perror("read()");
        exit(EXIT_FAILURE);
    }
    if (out.error == OK) {
        printf("%.2f %c %.2f = %.2f\n", in.a, argv[2][0], in.b, out.result);
        close(socketfd);
        exit(EXIT_SUCCESS);
    }
}