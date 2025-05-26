#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

void *thread_func(void *arg)
{
    int fd = *(int *)arg;
    char c;
    int fl = 1;
    while(fl)
    {
        fl = read(fd,&c,1);
        if (fl == 1) write(1,&c,1);
    }
    return NULL;
}

int main()
{
  int fd1 = open("alphabet.txt",O_RDONLY);
  int fd2 = open("alphabet.txt",O_RDONLY);

  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

  pthread_t t1, t2;
  if (pthread_create(&t1, &attr,thread_func,&fd1)) {
    perror("pthread_create");
    exit(1);
  }
  if (pthread_create(&t2, &attr,thread_func,&fd2)) {
    perror("pthread_create");
    exit(1);
  }

  return 0;
}