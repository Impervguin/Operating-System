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

#define WRITE_QUEUE 0
#define READ_QUEUE 1
#define ACTIVE_WRITER 2
#define ACTIVE_READERS 3

#define v 1
#define p -1

int fl = 1;

struct sembuf start_read[] = {
    {READ_QUEUE, v, 0},
    {WRITE_QUEUE, 0, 0},
    {ACTIVE_WRITER, 0, 0},
    {ACTIVE_READERS, v, 0},
    {READ_QUEUE, p, 0}
};

struct sembuf end_read[] = {
    {ACTIVE_READERS, p, 0}
};

struct sembuf start_write[] = {
    {WRITE_QUEUE, v, 0},
    {ACTIVE_WRITER, 0, 0},
    {ACTIVE_READERS, 0, 0},
    {ACTIVE_WRITER, v, 0},
    {WRITE_QUEUE, p, 0}
};

struct sembuf end_write[] = {
    {ACTIVE_WRITER, p, 0}
};

int socketfd;

void process_client(char *buf, int socketfd, int semid) {
    char inbuf[MAX_MESSAGE_SIZE];
    rw_in in;
    int size;
    for (; (size = read(socketfd, inbuf, sizeof(buf))) > 0; ) {
        inbuf[size] = '\0';
        sscanf(inbuf, "%d%d", &in.type, &in.index);
        switch (in.type)
        {
        case READ:
            {
            struct r_out out;
            out.error = OK;
            printf("got read request\n");
            int err = semop(semid, start_read, 5);
            if (err == -1) {
                perror("semop\n");
                exit(1);
            }
            memcpy(out.arr, buf, sizeof(out.arr));
            err = semop(semid, end_read, 1);
            if (err == -1) {
                perror("semop\n");
                exit(1);
            }
            out.error = htonl(out.error);
            if (write(socketfd, &out, sizeof(out)) == -1) {
                perror("write()");
                exit(1);
            }
            break;
            }
        case WRITE:
            {
            struct w_out out;
            out.error = OK;
            printf("got write request, index %d\n", in.index);

            int err = semop(semid, start_write, 5);
            if (err == -1) {
                perror("semop\n");
                exit(1);
            }
            if (in.index < 0 || in.index >= ARRAY_SIZE) {
                out.error = OUT_OF_RANGE_ERROR;
            } else if (buf[in.index] == ARRAY_ELEMENT_BUSY) {
                out.error = WRITE_BUSY_ERROR;
            } else {
                out.result = buf[in.index];
                buf[in.index] = ARRAY_ELEMENT_BUSY;
            }
            err = semop(semid, end_write, 1);
            if (err == -1) {
                perror("semop\n");
                exit(1);
            }
            out.error = htonl(out.error);
            if (write(socketfd, &out, sizeof(out)) == -1) {
                perror("write()");
            }
            break;
            }
        default:
            {
            struct err_out out;
            out.error = htonl(INVALID_OPERATION_ERROR);
            if (write(socketfd, &out, sizeof(out)) == -1) {
                perror("write()");
            }
            }
            break;
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
    for (int i = 0; i < ARRAY_SIZE; i++) {
        buf[i] = 'a' + i % 26;
    }

    printf("Shared memory segment created and attached.\n");

    semid = semget(IPC_PRIVATE, 4, IPC_CREAT | 0666);
    if (semid == -1) {
        perror("semget");
        return 1;
    }
   
    if (semctl(semid, ACTIVE_READERS, SETVAL, 0) == -1) {
        perror("semctl");
        return 1;
    }
    if (semctl(semid, ACTIVE_WRITER, SETVAL, 0) == -1) {
        perror("semctl");
        return 1;
    }
    if (semctl(semid, WRITE_QUEUE, SETVAL, 0) == -1) {
        perror("semctl");
        return 1;
    }
    if (semctl(semid, READ_QUEUE, SETVAL, 0) == -1) {
        perror("semctl");
        return 1;
    }

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
            process_client(buf, clsocketfd, semid);
            close(clsocketfd);
            exit(EXIT_SUCCESS);
        }
    }
}