#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

struct thread_arg {
    int fd;
    int i;
};

void *thread_func(void *arg)
{
    struct thread_arg *args = (struct thread_arg *)arg;
    int fd = args->fd;
    int i = args->i;

    for(char c = 'a'; c <= 'z'; c++)
    {
      if (c % 2 == i) {
          write(fd, &c, 1);
      }
    }
    return NULL;
}

int main() 
{
  int fd1 = open("q.txt",O_RDWR);
  int fd2 = open("q.txt",O_RDWR);

  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

  struct thread_arg args1;
  args1.fd = fd1;
  args1.i = 0;
  pthread_t t1, t2;
  if (pthread_create(&t1, &attr,thread_func,&args1)) {
    perror("pthread_create");
    exit(1);
  }

  struct thread_arg args2;
  args2.fd = fd2;
  args2.i = 1;
  if (pthread_create(&t2, &attr,thread_func,&args2)) {
    perror("pthread_create");
    exit(1);
  }
  
  close(fd1);
  close(fd2);
  return 0;
}