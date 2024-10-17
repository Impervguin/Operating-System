#include "lockdaemon.h"
#include "daemon.h"
#include <pthread.h>
#include <syslog.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <stdio.h>


sigset_t mask;

void *thr_fn(void *arg) {
    int err, signo;
    for (;;) {
        err = sigwait(&mask, &signo);
        if (err != 0) {
            syslog(LOG_ERR, "ошибка вызова функции sigwait");
            exit(1);
        }
        switch (signo) {
        case SIGHUP:
            syslog(LOG_INFO, "сигнал SIGHUP");
            break;
        case SIGTERM:
            syslog(LOG_INFO, "сигнал SIGTERM; выход");
            exit(0);
        default:
            syslog(LOG_INFO, "непредвиденный сигнал %d\n", signo);
        }
    }
    return(0);
}

int main(int argc, char *argv[]) {
    int err;
    pthread_t tid;
    char *cmd = "daemon3";
    struct sigaction sa;
    // printf("test\n");
    // if ((cmd = strrchr(argv[0], '/')) == NULL)
    //     cmd = argv[0];
    // else
    //     cmd++;
    /*
    * Перейти в режим демона.
    */
    daemonize(cmd);
    /*
    * Убедиться, что ранее не была запущена другая копия демона.
    */
    if (already_running()) {
        syslog(LOG_ERR, "демон уже запущен");
        exit(1);
    }
    /*
    * Восстановить действие по умолчанию для сигнала SIGHUP
    * и заблокировать все сигналы.
    */
    sa.sa_handler = SIG_DFL;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGHUP, &sa, NULL) < 0)
    {
        syslog(LOG_ERR, "%s: невозможно восстановить действие SIG_DFL для SIGHUP", cmd);
        exit(1);
    }
    sigfillset(&mask);
    if ((err = pthread_sigmask(SIG_BLOCK, &mask, NULL)) != 0)
    {
        syslog(LOG_ERR, "%s: ошибка выполнения операции SIG_BLOCK", cmd);
        exit(1);
    }
    /*
    * Создать поток для обработки SIGHUP и SIGTERM.
    */
    err = pthread_create(&tid, NULL, thr_fn, 0);
    if (err != 0) {
        syslog(LOG_ERR, "%s: ошибка создания потока", cmd);
        exit(1);
    }
    
    time_t raw_time;
    struct tm *timeinfo;
    
    for (;;)
    {
        sleep(10);
        time(&raw_time);
        timeinfo = localtime(&raw_time);
        syslog(LOG_INFO, "текущее время: %s", asctime(timeinfo));
    }
    exit(0);
}