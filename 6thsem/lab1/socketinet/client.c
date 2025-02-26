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

    for (;;) {
        int type = htonl(READ);
        int rc;
        if (write(socketfd, &type, sizeof(type)) == -1) {
            perror("write()");
            exit(EXIT_FAILURE);
        }
        
        int size = read(socketfd, &rc, sizeof(rc));
        if (size == -1) {
            perror("read()");
            exit(EXIT_FAILURE);
        }
        if (size == 0) {
            printf("server close connection\n");
            close(socketfd);
            exit(EXIT_SUCCESS);
        }
        rc = ntohl(rc);

        if (rc != OK) {
            printf("Error: %d\n", rc);
            close(socketfd);
            exit(EXIT_FAILURE);
        }
        char arr[ARRAY_SIZE + 1];
        size = read(socketfd, arr, sizeof(arr));
        if (size == -1) {
            perror("read()");
            exit(EXIT_FAILURE);
        }
        arr[size] = '\0';
        if (size == 0) {
            printf("server close connection\n");
            close(socketfd);
            exit(EXIT_SUCCESS);
        }
        
        printf("array: %s\n", arr);

        sleep(1);
        
        int freeindex = -1;
        for (int i = 0; i < ARRAY_SIZE; i++) {
            if (arr[i]!= ARRAY_ELEMENT_BUSY) {
                freeindex = i;
                break;
            }
        }
        if (freeindex == -1) {
            printf("No free elements\n");
            close(socketfd);
            exit(EXIT_FAILURE);
        }

        type = htonl(WRITE);
        if (write(socketfd, &type, sizeof(type)) == -1) {
            perror("write()");
            exit(EXIT_FAILURE);
        }
        if (write(socketfd, &freeindex, sizeof(freeindex)) == -1) {
            perror("write()");
            exit(EXIT_FAILURE);
        }

        size = read(socketfd, &rc, sizeof(rc));
        if (size == -1) {
            perror("read()");
            exit(EXIT_FAILURE);
        }
        if (size == 0) {
            printf("server close connection\n");
            close(socketfd);
            exit(EXIT_SUCCESS);
        }
        rc = ntohl(rc);
        if (rc == WRITE_BUSY_ERROR) {
            printf("element %d already busy\n", freeindex);
        } else if (rc != OK) {
            printf("Error: %d\n", rc);
            close(socketfd);
            exit(EXIT_FAILURE);
        } else {
            char result;
            size = read(socketfd, &result, sizeof(result));
            if (size == -1) {
                perror("read()");
                exit(EXIT_FAILURE);
            }
            if (size == 0) {
                printf("server close connection\n");
                close(socketfd);
                exit(EXIT_SUCCESS);
            }
            printf("element %d: %c\n", freeindex, result);
        }
        int useconds = rand() % 3000000;
        usleep(useconds);
    }
    if (close(socketfd) == -1) {
        perror("close()");
        exit(EXIT_FAILURE);
    }
    exit(EXIT_SUCCESS);
}