#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "rw.h"

int main(int argc, char **argv) {

    int socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketfd == -1) {
        perror("socket()");
        exit(EXIT_FAILURE);
    }
    struct sockaddr_in sa;
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port = htons(PORT);
    sa.sin_addr.s_addr = INADDR_ANY;

    if (connect(socketfd, (struct sockaddr *) &sa, sizeof(sa)) == -1) {
        perror("connect()");
        exit(EXIT_FAILURE);
    }
    printf("Connect\n");
    for (;;) {
        rw_in in;
        rw_out out;
        in.type = htonl(READ);
        if (write(socketfd, &in, sizeof(in)) == -1) {
            perror("write()");
            exit(EXIT_FAILURE);
        }
        int size = read(socketfd, &out, sizeof(out));
        if (size == -1) {
            perror("read()");
            exit(EXIT_FAILURE);
        }
        if (size != sizeof(out)) {
            perror("Invalid format");
            exit(EXIT_FAILURE);
        }
        printf("Read: %d\n", size);
        sleep(1);
    }
    close(socketfd);
}