#include <stdio.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>

#define ELEMENTS 32

#define SLEEP_RANDOM_MAX 2
#define SLEEP_PRODUCE_MAX 1
#define SLEEP_CONSUME_MAX 3

#define SEMAPHORE_EMPTY 0
#define SEMAPHORE_FULL 1
#define SEMAPHORE_BINARY 2

#define CONSUMER_COUNT 3
#define PRODUCER_COUNT 3

#define p -1
#define v 1

int fl = 1;

struct sembuf start_consume[] = {{SEMAPHORE_FULL, p, 0}, {SEMAPHORE_BINARY, p, 0}};
struct sembuf end_consume[] = {{SEMAPHORE_BINARY, v, 0}, {SEMAPHORE_EMPTY, v, 0}};

struct sembuf start_produce[] = {{SEMAPHORE_EMPTY, p, 0}, {SEMAPHORE_BINARY, p, 0}};
struct sembuf end_produce[] = {{SEMAPHORE_BINARY, v, 0}, {SEMAPHORE_FULL, v, 0}};

void signal_handler(int signo, siginfo_t *info, void *context) {
    printf("PID %d signal %d\n", getpid(), signo);
    fl = 0;
}

void consumer(int semid, char *buf, char **consumep) {
    srand(getpid());
    
    while (fl) {
        sleep(rand() % SLEEP_CONSUME_MAX + 1);
        int err = semop(semid, start_consume, 2);
        if (err == -1) {
            if (errno == EINTR) {
                printf("PID %d caught interrupt on block\n", getpid());
                exit(1);
            }
            perror("semop\n");
            exit(1);
        }

        char symb = **consumep;
        (*consumep)++;
        if (*consumep - buf == ELEMENTS) {
            *consumep = buf;
        }

        printf("%d get %c\n", getpid(), symb);

        err = semop(semid, end_consume, 2);
        if (err == -1) {
            perror("semop\n");
            exit(1);
        }
    }
}

void producer(int semid, char *letter, char *buf, char **producep) {
    srand(getpid());

    while (fl) {
        sleep(rand() % SLEEP_PRODUCE_MAX + 1);
        int err = semop(semid, start_produce, 2);
        if (err == -1) {
            if (errno == EINTR) {
                printf("PID %d caught interrupt on block\n", getpid());
                exit(1);
            }
            perror("semop\n");
            exit(1);
        }

        char symb = (*letter)++;
        if (*letter == 'z' + 1) {
            *letter = 'a';
        }

        *((*producep)++) = symb;
        if (*producep - buf == ELEMENTS) {
            *producep = buf;
        }
        
        printf("%d put %c\n", getpid(), symb);

        err = semop(semid, end_produce, 2);
        if (err == -1) {
            perror("semop\n");
            exit(1);
        }
    }
}


int main() {
    key_t shmid, semid;
    struct sembuf sem_op;

    struct sigaction act = { 0 };
    act.sa_sigaction = &signal_handler;

    if (sigaction(SIGINT, &act, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

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

    char *letter = buf;
    *letter = 'a';
    buf++;
    
    char **producep = (char **)buf;
    buf += sizeof(char *);
    char **consumep = (char **)buf;
    buf += sizeof(char *);
    *producep = buf;
    *consumep = buf;

    semid = semget(IPC_PRIVATE, 3, IPC_CREAT | 0666);
    if (semid == -1) {
        perror("semget");
        return 1;
    }

    if (semctl(semid, SEMAPHORE_EMPTY, SETVAL, ELEMENTS) == -1) {
        perror("semctl");
        return 1;
    }
    if (semctl(semid, SEMAPHORE_BINARY, SETVAL, 1) == -1) {
        perror("semctl");
        return 1;
    }
    if (semctl(semid, SEMAPHORE_FULL, SETVAL, 0) == -1) {
        perror("semctl");
        return 1;
    }

    pid_t childpid[CONSUMER_COUNT + PRODUCER_COUNT];
    for (int i = 0; i < CONSUMER_COUNT; i++) {
        childpid[i] = fork();
        if (childpid[i] == -1) {
            perror("fork");
            return 1;
        } else if (childpid[i] == 0) {
            consumer(semid, buf, consumep);
            return 0;
        }
    }

    for (int i = CONSUMER_COUNT; i < CONSUMER_COUNT + PRODUCER_COUNT; i++) {
        childpid[i] = fork();
        if (childpid[i] == -1) {
            perror("fork");
            return 1;
        } else if (childpid[i] == 0) {
            producer(semid, letter, buf, producep);
            return 0;
        }
    }

    pid_t cpid = 0;
    int wstatus = 0;
    while (cpid != -1) {
        cpid = wait(&wstatus);  
        if (cpid == -1) {
            if (errno == EINTR) {
                cpid=0;
            } else if (errno != ECHILD) {
                printf("Error: wait %d\n", wstatus);
                exit(EXIT_FAILURE);
            }
        } else {
            if (WIFEXITED(wstatus)) {
                printf("Child pid=%d exited, status=%d\n", cpid, WEXITSTATUS(wstatus));
            } else if (WIFSIGNALED(wstatus)) {
                printf("Child pid=%d killed by signal %d\n", cpid, WTERMSIG(wstatus));
            } else if (WIFSTOPPED(wstatus)) {
                printf("Child pid=%d stopped by signal %d\n", cpid, WSTOPSIG(wstatus));
            } else if (WIFCONTINUED(wstatus)) {
                printf("Child pid=%d continued\n", cpid);
            }
        }
    }

    if (shmctl(shmid, IPC_RMID, 0) == -1) {
        perror("shmctl");
        return 1;
    }

    if (semctl(semid, IPC_RMID, 0) == -1) {
        perror("semctl");
        return 1;
    }

    return 0; 
}