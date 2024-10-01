#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>

#define CHILDREN_COUNT 3

int main(void) {
    pid_t children[CHILDREN_COUNT];

    for (int i = 0; i < CHILDREN_COUNT; i++) {
        children[i] = fork();
        if (children[i] == -1) {
            printf("Error: fork() failed.");
            exit(EXIT_FAILURE);
        } else if (children[i] == 0) {
            printf("Child %d: pid=%d, ppid=%d, gr=%d\n", i, getpid(), getppid(), getpgrp());
            exit(EXIT_SUCCESS);
        } else {
            printf("Parent: pid=%d, childpid=%d, gr=%d\n", getpid(), children[i], getpgrp());
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
    exit(EXIT_SUCCESS);
}