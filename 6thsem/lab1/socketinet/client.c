#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include "rw.h"

int main(int argc, char **argv) {
    srand(time(NULL));
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
        char message[128];
        rw_in in;
        in.type = READ;
        int messize = sprintf(message, "%d %d", in.type, in.index);

        if (write(socketfd, &message, messize) == -1) {
            perror("write()");
            exit(EXIT_FAILURE);
        }
        struct r_out r_out;
        int size = read(socketfd, &r_out, sizeof(r_out));
        if (size == -1) {
            perror("read()");
            exit(EXIT_FAILURE);
        }

        r_out.error = ntohl(r_out.error);
        if (size!= sizeof(r_out)) {
            perror("Invalid format");
            exit(EXIT_FAILURE);
        }
        if (r_out.error == OK) {
            printf("Received: %s\n", r_out.arr);
        } else {
            printf("Error: %d\n", r_out.error);
            continue;
        }
        sleep(1);
        
        int freeindex = -1;
        // for (int i = 0; i < ARRAY_SIZE * 2; i++) {
        //     int i = rand() % ARRAY_SIZE;
        //     if (r_out.arr[i] != ARRAY_ELEMENT_BUSY) {
        //         freeindex = i;
        //         break;
        //     }
        // }
        for (int i = 0; i < ARRAY_SIZE; i++) {
            if (r_out.arr[i]!= ARRAY_ELEMENT_BUSY) {
                freeindex = i;
                break;
            }
        }
        if (freeindex == -1) {
            printf("No free elements\n");
            continue;
        }

        in.type = WRITE;
        in.index = freeindex;
        messize = sprintf(message, "%d %d", in.type, in.index);
        printf("%s\n", message);

        if (write(socketfd, &message, messize) == -1) {
            perror("write()");
            exit(EXIT_FAILURE);
        }
        struct w_out w_out;
        size = read(socketfd, &w_out, sizeof(w_out));
        if (size == -1) {
            perror("read()");
            exit(EXIT_FAILURE);
        }
        w_out.error = ntohl(w_out.error);
        if (size!= sizeof(w_out)) {
            perror("Invalid format");
            exit(EXIT_FAILURE);
        }
        if (w_out.error == OK) {
            printf("Wrote on %d %c\n", freeindex, w_out.result);
        } else if (w_out.error == WRITE_BUSY_ERROR) {
            printf("Index %d already busy\n", freeindex);
        } else {
            printf("Error: %d\n", w_out.error);
        }
        sleep(1);
    }
    close(socketfd);
}