#include <stdio.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>


#define WRITE_QUEUE 0
#define READ_QUEUE 1
#define ACTIVE_WRITER 2
#define ACTIVE_READERS 3

#define READERS_COUNT 3
#define WRITERS_COUNT 3

#define SLEEP_READ_MAX 2
#define SLEEP_WRITE_MAX 2

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

void signal_handler(int signo, siginfo_t *info, void *context) {
    printf("PID %d signal %d\n", getpid(), signo);
    fl = 0;
}

void reader(key_t semid, char *var) {
    srand(getpid());
    
    while (fl) {
        sleep(rand() % SLEEP_READ_MAX + 1);
        int err;
        err = semop(semid, start_read, 5);
        if (err == -1) {
            if (errno == EINTR) {
                printf("PID %d caught interrupt on block\n", getpid());
                exit(1);
            }
            perror("semop\n");
            exit(1);
        }
        
        printf("%d read %c\n", getpid(), *var);

        err = semop(semid, end_read, 1);
        if (err == -1) {
            perror("semop\n");
            exit(1);
        }
    }
}

void writer(key_t semid, char *var) {
    srand(getpid());
    while (fl) {
        sleep(rand() % SLEEP_WRITE_MAX + 1);
        int err = semop(semid, start_write, 5);
        if (err == -1) {
            if (errno == EINTR) {
                printf("PID %d caught interrupt on block\n", getpid());
                exit(1);
            }
            perror("semop\n");
            exit(1);
        }
        

        (*var)++;
        if (*var == 'z' + 1) {
            *var = 'a';
        }

        printf("%d wrote %c\n", getpid(), *var);
        
        err = semop(semid, end_write, 1);
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

    semid = semget(IPC_PRIVATE, 4, IPC_CREAT | 0666);
    if (semid == -1) {
        perror("semget");
        return 1;
    }

    *buf = 'a';
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


    pid_t childpid[WRITERS_COUNT + READERS_COUNT];
    for (int i = 0; i < READERS_COUNT; i++) {
        childpid[i] = fork();
        if (childpid[i] == -1) {
            perror("fork");
            return 1;
        } else if (childpid[i] == 0) {
            reader(semid, buf);
            return 0;
        }
    }

    for (int i = READERS_COUNT; i < WRITERS_COUNT + READERS_COUNT; i++) {
        childpid[i] = fork();
        if (childpid[i] == -1) {
            perror("fork");
            return 1;
        } else if (childpid[i] == 0) {
            writer(semid, buf);
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
