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
    
    struct sockaddr_in sa;
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port = htons(PORT);
    sa.sin_addr.s_addr = INADDR_ANY;
    

    for (;;) {                                                                                                                                                                                                                                                                                                                  
        int socketfd = socket(AF_INET, SOCK_STREAM, 0);
        if (socketfd == -1) {
            perror("socket()");
            exit(EXIT_FAILURE);
        }
        if (connect(socketfd, (struct sockaddr *) &sa, sizeof(sa)) == -1) {
            perror("connect()");
            exit(EXIT_FAILURE);
        }
        int type = htonl(READ);
        int rc;
        if (send(socketfd, &type, sizeof(type), 0) == -1) {                                                                                                                                                                                                                               
            perror("write()");
            exit(EXIT_FAILURE);
        }
        
        int size = recv(socketfd, &rc, sizeof(rc), 0);
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
        size = recv(socketfd, arr, sizeof(arr), 0);
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
            if (arr[i]!= ARRAY_ELEMENT_OCCUPIED) {
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
        if (send(socketfd, &type, sizeof(type), 0) == -1) {
            perror("write()");
            exit(EXIT_FAILURE);
        }
        if (send(socketfd, &freeindex, sizeof(freeindex), 0) == -1) {
            perror("write()");
            exit(EXIT_FAILURE);
        }

        size = recv(socketfd, &rc, sizeof(rc), 0);
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
        if (rc == WRITE_OCCUPIED_ERROR) {
            printf("element %d already occupied\n", freeindex);
        } else if (rc != OK) {
            printf("Error: %d\n", rc);
            close(socketfd);
            exit(EXIT_FAILURE);
        } else {
            char result;
            size = recv(socketfd, &result, sizeof(result), 0);
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
        if (close(socketfd) == -1) {
            perror("close()");
            exit(EXIT_FAILURE);
        }
        int useconds = rand() % 3000000;
        usleep(useconds);
    }
    exit(EXIT_SUCCESS);
}