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

  pthread_t t;
  if (pthread_create(&t, &attr,thread_func,&fd1)) {
    perror("pthread_create");
    exit(1);
  }

  char c;
  int fl = 1;
  while(fl)
  {
      fl = read(fd2,&c,1);
      if (fl == 1) write(1,&c,1);
  }
  return 0;
}