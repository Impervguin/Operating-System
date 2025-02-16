#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

#define CHILDREN_COUNT 1
#define MESSAGE_MAX_SIZE 128

int main() {
    pid_t children[CHILDREN_COUNT];
    int socketfd[2];
    int err = socketpair(AF_UNIX, SOCK_STREAM, 0, socketfd);
    if (err != 0) {
        perror("socketpair()");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < CHILDREN_COUNT; i++) {
        children[i] = fork();
        if (children[i] == -1) {
            perror("fork()");
            exit(EXIT_FAILURE);
        } else if (children[i] == 0) {
            sleep(2);
            close(socketfd[0]);
            char message[MESSAGE_MAX_SIZE];
            if (read(socketfd[1], message, sizeof(message)) == -1) {
                perror("child read()");
                exit(EXIT_FAILURE);
            }
            printf("child receive: %s\n", message);
            int pid = getpid();
            int size = sprintf(message, "Hello from %d!", pid); 
            if (write(socketfd[1], message, size) == -1) {
                perror("child write()");
                exit(EXIT_FAILURE);
            }
            printf("child sent: %s\n", message);
            close(socketfd[1]);
            exit(EXIT_SUCCESS);
        } else {
            char message[MESSAGE_MAX_SIZE];
            int size = sprintf(message, "Hello from parent!");
            if (write(socketfd[0], message, size) == -1) {
                perror("parent write()");
                exit(EXIT_FAILURE);
            }
            printf("parent sent: %s\n", message);
            if ((size = read(socketfd[0], message, MESSAGE_MAX_SIZE)) == -1) {
                perror("parent read()");
                exit(EXIT_FAILURE);
            }
            message[size] = '\0';
            printf("parent receive: %s\n", message);
        }
    }
    exit(EXIT_SUCCESS);
}
