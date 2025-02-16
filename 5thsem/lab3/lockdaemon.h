#define LOCKFILE "/var/run/mydaemon.pid"
#define LOCKMODE (S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)

int lockfile(int fd);
int already_running(void);