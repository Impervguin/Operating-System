#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "socketcs.h"

int socketfd;

void sigint_handler() {
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

int main() {
    socketfd = socket(AF_UNIX, SOCK_STREAM, 0);
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

    if (listen(socketfd, 5) == -1) {
        perror("listen()");
        exit(EXIT_FAILURE);
    }

    for (;;) {
        int clsocketfd = accept(socketfd, NULL, NULL);
        if (clsocketfd == -1) {
            perror("accept()");
            continue;
        }
        calculate_in in;
        calculate_out out;
        int size = read(clsocketfd, &in, sizeof(in));
        if (size == -1) {
            perror("read()");
            close(clsocketfd);
            continue;
        }
        if (size!= sizeof(in)) {
            out.error = INPUT_ERROR;
        } else {
            out.error = OK;
            switch (in.operation)
            {
            case ADD:
                out.result = in.a + in.b;
                break;
            case SUB:
                out.result = in.a - in.b;
                break;
            case MUL:
                out.result = in.a * in.b;
                break;
            case DIV:
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
            size = write(clsocketfd, &out, sizeof(out));
            if (size == -1) {
                perror("write()");
            }
            close(clsocketfd);
        }
    }
}