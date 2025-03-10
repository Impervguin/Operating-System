#include "rw.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/sem.h>
#include <sys/epoll.h>
#include <sys/times.h>
#include <time.h>

#define WRITE_QUEUE 0
#define READ_QUEUE 1
#define ACTIVE_WRITER 2
#define ACTIVE_READERS 3

#define v 1
#define p -1

#define FUNC_OK 0
#define FUNC_EXIT_SUCCESS 1
#define FUNC_EXIT_FAILURE 2
#define FUNC_EXIT_ARRAY_FULL 3

#define MAX_EVENTS 64

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

struct thread_args {
    int semid;
    int ppid;
    int socketfd;
};

long get_time() {
    return clock();
}

int listenfd;
int array_counter = 0;
char array[ARRAY_SIZE];


int reader(char *buf, int socketfd, int semid) {
    char buffer[ARRAY_SIZE];
    
    int err = semop(semid, start_read, 5);
    if (err == -1) {
        perror("semop\n");
        return FUNC_EXIT_FAILURE;
    }
    memcpy(buffer, buf, sizeof(buffer));
    err = semop(semid, end_read, 1);
    if (err == -1) {
        perror("semop\n");
        return FUNC_EXIT_FAILURE;
    }
    int rc = htonl(OK);
    if (send(socketfd, &rc, sizeof(rc), 0) == -1) {
        perror("write()");
        return FUNC_EXIT_FAILURE;
    }
    if (send(socketfd, &buffer, sizeof(buffer), 0) == -1) {
        perror("write()");
        return FUNC_EXIT_FAILURE;
    }
    return FUNC_OK;
}

int writer(char *buf, int socketfd, int semid, int index) {
    int rc = OK;
    char result = '\0';
    int done = FUNC_OK;
    int err = semop(semid, start_write, 5);
    if (err == -1) {
        perror("semop\n");
        return FUNC_EXIT_FAILURE;
    }
    if (index < 0 || index >= ARRAY_SIZE) {
        rc = OUT_OF_RANGE_ERROR;
    } else if (buf[index] == ARRAY_ELEMENT_OCCUPIED) {
        rc = WRITE_OCCUPIED_ERROR;
    } else {
        result = buf[index];
        buf[index] = ARRAY_ELEMENT_OCCUPIED;
        array_counter++;
    }
    if (array_counter == ARRAY_SIZE) {
        done = FUNC_EXIT_ARRAY_FULL;
    } else {
        printf("write request, index %d\n", index);
    }
    err = semop(semid, end_write, 1);
    if (err == -1) {
        perror("semop\n");
        return FUNC_EXIT_FAILURE;
    }
    rc = htonl(rc);
    if (send(socketfd, &rc, sizeof(rc), 0) == -1) {
        perror("write()");
    }
    if (result != '\0')
        if (send(socketfd, &result, sizeof(result), 0) == -1)
            perror("write()");
    
    return done;
}

int usefdfunc(int semid, int socketfd) {
    int size;
    int type;
    int cpid;
    for (; (size = recv(socketfd, &type, sizeof(type), 0)) > 0; ) {
        type = ntohl(type);
        if (recv(socketfd, &cpid, sizeof(cpid), 0) < 0) {
            perror("read()");
            close(socketfd);
            return FUNC_EXIT_FAILURE;
        }
        switch (type)
        {
        case READ:
            {
                printf("read request\n");
                int rc = reader(array, socketfd, semid);
                if (rc != FUNC_OK) {
                    close(socketfd);
                    return rc;
                }
                break;
            }
        case WRITE:
            {
                int index;
                if ((size = recv(socketfd, &index, sizeof(index), 0)) < 0) {
                    perror("read()");
                    close(socketfd);
                    return FUNC_EXIT_FAILURE;
                } else if (size == 0) {
                    printf("conn closed\n");
                    close(socketfd);
                    return FUNC_EXIT_SUCCESS;
                }
                int done = writer(array, socketfd, semid, index);
                if (done == FUNC_EXIT_ARRAY_FULL) {
                    close(socketfd);
                    kill(getpid(), SIGINT);
                    return FUNC_EXIT_ARRAY_FULL;
                } else if (done != FUNC_OK) {
                    close(socketfd);
                    return done;
                }
                break;
            }
        default:
            {
                printf("unexpected request\n");
                int err = htonl(INVALID_OPERATION_ERROR);
                if (send(socketfd, &err, sizeof(err), 0) == -1) {
                    perror("write()");
                    close(socketfd);
                    return FUNC_EXIT_FAILURE;
                }
            }
            break;
        }
    }
    close(socketfd);
    if (size == -1) {
        perror("read()");
        return FUNC_EXIT_FAILURE;
    }
    if (size == 0) {
        printf("Conn closed\n");
    }
    return FUNC_EXIT_SUCCESS;
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
    key_t semid;
    long start_time, end_time;
    struct epoll_event ev, events[MAX_EVENTS];
    int nfds, epollfd;

    memset(array, ARRAY_ELEMENT_FREE, ARRAY_SIZE);
    for (int i = 0; i < ARRAY_SIZE; i++) {
        array[i] = 'a' + i % 26;
    }

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

    epollfd = epoll_create1(0);
    if (epollfd == -1) {
        perror("epoll_create1");
        exit(EXIT_FAILURE);
    }

    ev.events = EPOLLIN;
    ev.data.fd = listenfd;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, listenfd, &ev) == -1) {
        perror("epoll_ctl: listen_sock");
        exit(EXIT_FAILURE);
    }

    for (;;) {
        nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            perror("epoll_wait");
            exit(EXIT_FAILURE);
        }


        start_time = get_time();
        for (int n = 0; n < nfds; ++n) {
            if (events[n].data.fd == listenfd) {
                int connfd = accept(listenfd, NULL, NULL);
                if (connfd == -1) {
                    perror("accept");
                    exit(EXIT_FAILURE);
                }
                struct epoll_event new_event;
                new_event.data.fd = connfd;
                new_event.events = EPOLLIN | EPOLLOUT | EPOLLET;
                if (epoll_ctl(epollfd, EPOLL_CTL_ADD, connfd, &new_event) == -1) {
                    perror("epoll_ctl: conn_sock");
                    exit(EXIT_FAILURE);
                }
            } else {
                usefdfunc(semid, events[n].data.fd);
                end_time = get_time();
                printf("served %ld jiff\n", end_time - start_time);
            }
        }
}
}