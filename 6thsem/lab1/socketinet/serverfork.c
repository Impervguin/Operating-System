#include "rw.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/sem.h>
#include <sys/shm.h>

int socketfd;

void process_client(char *buf, int socketfd) {
    rw_in in;
    rw_out out;
    out.error = OK;
    int size;
    for (; (size = read(socketfd, &in, sizeof(in))) > 0; ) {
        if (size!= sizeof(in)) {
            out.error = INVALID_FORMAT_ERROR;
            if (write(socketfd, &out, sizeof(out)) == -1) {
                perror("write()");
                close(socketfd);
            } 
        } else {
            switch (in.type)
            {
            case READ:
                printf("Read\n");
                break;
            case WRITE:
                printf("Write\n");
                break;
            default:
                out.error = INVALID_OPERATION_ERROR;
                break;
            }
            if (write(socketfd, &out, sizeof(out)) == -1) {
                perror("write()");
            }
        }
    }
    if (size == -1) {
        perror("read()");
        close(socketfd);
        return;
    }
    if (size == 0) {
        printf("Conn closed\n");
        return;
    }
    
    close(socketfd);
}

void sigint_handler() {
    if (close(socketfd) == -1) {
        perror("close()");
        exit(EXIT_FAILURE);
    }
    printf("Server stopped.\n");
    exit(EXIT_SUCCESS);
}

int main() {
    key_t shmid, semid;

    // Create shared memory segment
    shmid = shmget(IPC_PRIVATE, getpagesize(), IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget");
        return 1;
    }

    // Attach shared memory segment to process address space
    char *buf = shmat(shmid, NULL, 0);
    if (buf == (void *)-1) {
        perror("shmat");
        return 1;
    }
    memset(buf, ARRAY_ELEMENT_FREE, ARRAY_SIZE);
    printf("Shared memory segment created and attached.\n");

    socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketfd == -1) {
        perror("socket()");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in sa;
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port = htons(PORT);
    sa.sin_addr.s_addr = INADDR_ANY;
    if (bind(socketfd, (struct sockaddr *) &sa, sizeof(sa)) == -1) {
        perror("bind()");
        exit(EXIT_FAILURE);
    }
    if (listen(socketfd, 5) == -1) {
        perror("listen()");
        exit(EXIT_FAILURE);
    }
    printf("Server started on port %d.\n", PORT);
    if (signal(SIGINT, sigint_handler) == -1) {
        perror("signal()");
        exit(EXIT_FAILURE);
    }
    for (;;) {
        int clsocketfd = accept(socketfd, NULL, NULL);
        if (clsocketfd == -1) {
            perror("accept()");
            continue;
        }
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork()");
            close(clsocketfd);
            continue;
        } else if (pid == 0) {
            close(socketfd);
            process_client(buf, clsocketfd);
            close(clsocketfd);
            exit(EXIT_SUCCESS);
        }
    }
}