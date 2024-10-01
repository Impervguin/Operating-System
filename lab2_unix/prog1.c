#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

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
            sleep(3);
            printf("Child %d: pid=%d, ppid=%d, gr=%d\n", i, getpid(), getppid(), getpgrp());
            exit(EXIT_SUCCESS);
        } else {
            printf("Parent: pid=%d, childpid=%d, gr=%d\n", getpid(), children[i], getpgrp());
        }
    }
    exit(EXIT_SUCCESS);
}