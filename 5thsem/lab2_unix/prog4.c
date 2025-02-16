#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#define CHILDREN_COUNT 3
#define MESS1 "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
#define MESS2 "yyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy"
#define MESS3 "zzz"

 
int main(void) {
    pid_t children[CHILDREN_COUNT];
    char *messages[CHILDREN_COUNT] = {MESS1, MESS2, MESS3};
    int fd[2];

    if (pipe(fd) == -1) {
        printf("Error: pipe() failed.");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < CHILDREN_COUNT; i++) {
        children[i] = fork();
        if (children[i] == -1) {
            printf("Error: fork() failed.");
            exit(EXIT_FAILURE);
        } else if (children[i] == 0) {
            // printf("Child %d: pid=%d, ppid=%d, gr=%d\n", i, getpid(), getppid(), getpgrp());
            close(fd[0]); // Close reading end of pipe
            if (write(fd[1], messages[i], strlen(messages[i]) + 1) == -1) {
                printf("Error: write() for child with id %d failed.", getpid());
                exit(EXIT_FAILURE);
            }
            // printf("Child %d message sent\n", getpid());
            exit(EXIT_SUCCESS);
        } else {
            // printf("Parent: pid=%d, childpid=%d, gr=%d\n", getpid(), children[i], getpgrp());
        }
    }
    int wstatus = 0;
    pid_t childpid = 0;
    while (childpid != -1) {
        childpid = wait(&wstatus);
        if (childpid == -1) {
            if (errno == ECHILD) {
                break; // No more children
            }
            printf("Error: waitpid %d\n", wstatus);
            exit(EXIT_FAILURE);
        }

        if (WIFEXITED(wstatus)) {
            printf("Child with pid=%d exited, status=%d\n", childpid, WEXITSTATUS(wstatus));
        } else if (WIFSIGNALED(wstatus)) {
            printf("Child with pid=%d killed by signal %d\n", childpid, WTERMSIG(wstatus));
        } else if (WIFSTOPPED(wstatus)) {
            printf("Child with pid=%d stopped by signal %d\n", childpid, WSTOPSIG(wstatus));
        } else if (WIFCONTINUED(wstatus)) {
            printf("Child with pid=%d continued\n", childpid);
        }
    }

    close(fd[1]); // Close writing end of pipe
    char buffer[256];
    int count = 1;
    for (int i = 0; i < CHILDREN_COUNT; i++) {
        count = read(fd[0], buffer, strlen(messages[i]) + 1);
        if (count == -1) {
            printf("Error: read\n");
            exit(EXIT_FAILURE);
        }
        printf("pid %d: %s\n",children[i], buffer);
    }
    count = count = read(fd[0], buffer, 1);
    if (count == -1) {
        printf("Error: read\n");
        exit(EXIT_FAILURE);
    }
    if (count != 0) {
        printf("Error: lost data in pipe\n");
        exit(EXIT_FAILURE);
    } else {
        printf("Pipe empty\n");
    }
    exit(EXIT_SUCCESS);
}