#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include "socketcs.h"

int socketfd;

void sigint_handler(int signum) {
    printf("Got singal %d.\n", signum);
    if (shutdown(socketfd, SHUT_RDWR) == -1) {
        perror("shutdown()");
        exit(EXIT_FAILURE);
    }
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

    if (signal(SIGINT, sigint_handler) == -1) {
        perror("signal()");
        exit(EXIT_FAILURE);
    }

    for (;;) {
        calculate_in in;
        calculate_out out;
        struct sockaddr sender;
        socklen_t sender_len;
        int size = recvfrom(socketfd, &in, sizeof(in), 0, &sender, &sender_len);
        if (size == -1) {
            perror("recvfrom()");
            continue;
        } else if (size == 0) {
            printf("Socket shutdown.\n");
            break;
        }
        if (size!= sizeof(in)) {
            out.error = INPUT_ERROR;
        } else {
            out.error = OK;
            switch (in.operation)
            {
            case ADD:
                printf("received %lf + %lf\n", in.a, in.b);
                out.result = in.a + in.b;
                break;
            case SUB:
                printf("received %lf - %lf\n", in.a, in.b);
                out.result = in.a - in.b;
                break;
            case MUL:
             printf("received %lf * %lf\n", in.a, in.b);
                out.result = in.a * in.b;
                break;
            case DIV:
                printf("received %lf / %lf\n", in.a, in.b);
                if (in.b < 1e-6) {
                    out.error = ZERO_DIVISION_ERROR;
                    break;
                }
                out.result = in.a / in.b;
                break;
            default:
                out.error = INVALID_OPERATION_ERROR;
                break;
            }
            size = sendto(socketfd, &out, sizeof(out), 0, &sender, sender_len);
            if (size == -1) {
                perror("sendto()");
            }
        }
    }
    if (close(socketfd) == -1) {
        perror("close()");
        exit(EXIT_FAILURE);
    }
    if (unlink(SOCKET_NAME) == -1) {
        perror("unlink()");
        exit(EXIT_FAILURE);
    }
    printf("Server stopped.\n");
    exit(EXIT_SUCCESS);
}