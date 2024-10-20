#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int main()
{
  pid_t childpid;
  if ((childpid = fork()) == -1) {
    perror("can't fork");
    exit(1);
  } else if (childpid == 0) {
    while (1) printf("%d ", getpid());
    return 0;
  } else {
    while (1) printf("%d ", getpid());
    return 0;
  }
}
