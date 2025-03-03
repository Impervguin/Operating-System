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

int listenfd;
int *array_counter;

void reader(char *buf, int socketfd, int semid) {
    char buffer[ARRAY_SIZE];
    
    int err = semop(semid, start_read, 5);
    if (err == -1) {
        perror("semop\n");
        exit(EXIT_FAILURE);
    }
    memcpy(buffer, buf, sizeof(buffer));
    err = semop(semid, end_read, 1);
    if (err == -1) {
        perror("semop\n");
        exit(EXIT_FAILURE);
    }
    int rc = htonl(OK);
    if (write(socketfd, &rc, sizeof(rc)) == -1) {
        perror("write()");
        exit(EXIT_FAILURE);
    }
    if (write(socketfd, &buffer, sizeof(buffer)) == -1) {
        perror("write()");
        exit(EXIT_FAILURE);
    }
}

int writer(char *buf, int socketfd, int semid, int index) {
    int rc = OK;
    char result = '\0';
    int done;
    int err = semop(semid, start_write, 5);
    if (err == -1) {
        perror("semop\n");
        exit(EXIT_FAILURE);
    }
    if (index < 0 || index >= ARRAY_SIZE) {
        rc = OUT_OF_RANGE_ERROR;
    } else if (buf[index] == ARRAY_ELEMENT_OCCUPIED) {
        rc = WRITE_OCCUPIED_ERROR;
    } else {
        result = buf[index];
        buf[index] = ARRAY_ELEMENT_OCCUPIED;
        (*array_counter)++;
    }
    if (*array_counter == ARRAY_SIZE) {
        done = 1;
    } else {
        printf("write request, index %d\n", index);
    }
    err = semop(semid, end_write, 1);
    if (err == -1) {
        perror("semop\n");
        exit(EXIT_FAILURE);
    }
    rc = htonl(rc);
    if (write(socketfd, &rc, sizeof(rc)) == -1) {
        perror("write()");
    }
    if (result != '\0')
        if (write(socketfd, &result, sizeof(result)) == -1)
            perror("write()");
    
    return done;
}

void childfunc(char *buf, int socketfd, int semid, int ppid) {
    int size;
    int type;
    for (; (size = read(socketfd, &type, sizeof(type))) > 0; ) {
        type = ntohl(type);
        switch (type)
        {
        case READ:
            {
                printf("read request\n");
                reader(buf, socketfd, semid);
                break;
            }
        case WRITE:
            {
                int index;
                if ((size = read(socketfd, &index, sizeof(index))) < 0) {
                    perror("read()");
                    close(socketfd);
                    exit(EXIT_FAILURE);
                } else if (size == 0) {
                    printf("conn closed\n");
                    close(socketfd);
                    exit(EXIT_FAILURE);
                }
                int done = writer(buf, socketfd, semid, index);
                if (done) {
                    kill(ppid, SIGINT);
                    close(socketfd);
                    exit(EXIT_SUCCESS);
                }
                break;
            }
        default:
            {
                printf("unexpected request\n");
                int err = htonl(INVALID_OPERATION_ERROR);
                if (write(socketfd, &err, sizeof(err)) == -1) {
                    perror("write()");
                    close(socketfd);
                    exit(EXIT_FAILURE);
                }
            }
            break;
        }
    }
    close(socketfd);
    if (size == -1) {
        perror("read()");
        exit(EXIT_FAILURE);
    }
    if (size == 0) {
        printf("Conn closed\n");
    }
    exit(EXIT_SUCCESS);
}

void sigint_handler() {
    if (close(listenfd) == -1) {
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
    array_counter = (int *) buf + ARRAY_SIZE;
    *array_counter = 0;

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

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd == -1) {
        perror("socket()");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in sa;
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port = htons(PORT);
    sa.sin_addr.s_addr = INADDR_ANY;
    if (bind(listenfd, (struct sockaddr *) &sa, sizeof(sa)) == -1) {
        perror("bind()");
        exit(EXIT_FAILURE);
    }
    if (listen(listenfd, 5) == -1) {
        perror("listen()");
        exit(EXIT_FAILURE);
    }
    printf("Server started on port %d.\n", PORT);
    if (signal(SIGINT, sigint_handler) == (__sighandler_t) -1) {
        perror("signal()");
        exit(EXIT_FAILURE);
    }
    for (;;) {
        int connfd = accept(listenfd, NULL, NULL);
        if (connfd == -1) {
            perror("accept()");
            exit(EXIT_FAILURE);
        }
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork()");
            close(connfd);
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            close(listenfd);
            childfunc(buf, connfd, semid, getppid());
        }
    }
}