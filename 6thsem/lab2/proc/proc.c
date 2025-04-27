#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <limits.h>
#include <string.h>
#include <fcntl.h>
// #define PATH_MAX 1024
#define BUF_SIZE 1024

#define OK 0
#define ERROR 1

int get_environ(int pid, FILE *out) {
    char path[PATH_MAX];
    sprintf(path, "/proc/%d/environ", pid);
    FILE *f = fopen(path, "r");
    if (f == NULL) {
        perror("fopen");
        return -1;
    }
    char line[BUF_SIZE];
    int len;
    while ((len = fread(line, 1, BUF_SIZE - 1, f)) > 0) {
        for (int i = 0; i < len; i++) {
            if (line[i] == '\0') {
                fprintf(out, "\n");
            } else {
                fprintf(out, "%c", line[i]);
            }
        }
    }
    fclose(f);
}

int get_cmdline(int pid, FILE *out) {
    char path[PATH_MAX];
    sprintf(path, "/proc/%d/cmdline", pid);
    FILE *f = fopen(path, "r");
    if (f == NULL) {
        perror("fopen");
        return -1;
    }
    char *line = NULL;
    size_t len = 0;
    if (getline(&line, &len, f) < 0) {
        perror("getline");
        return -1;
    }
    fprintf(out, "%s\n", line);
    free(line);
    fclose(f);
}

int get_cwd(int pid, FILE *out) {
    char path[PATH_MAX];
    char line[BUF_SIZE];
    sprintf(path, "/proc/%d/cwd", pid);
    ssize_t len = readlink (path, line, BUF_SIZE - 1);
    if (len < 0) {
        perror("readlink");
        return ERROR;
    }
    line[len] = '\0';
    fprintf(out, "%s\n", line);
    return OK;
}

int get_exe(int pid, FILE *out) {
    char path[PATH_MAX];
    char line[BUF_SIZE];
    sprintf(path, "/proc/%d/exe", pid);
    ssize_t len = readlink(path, line, BUF_SIZE - 1);
    if (len < 0) {
        perror("readlink");
        return ERROR;
    }
    path[len] = '\0';
    fprintf(out, "%s\n", path);
    return OK;
}

int get_exe_str(int pid, char *out) {
    char path[PATH_MAX];
    sprintf(path, "/proc/%d/exe", pid);
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        perror("open");
        return ERROR;
    }
    ssize_t len = readlink(path, out, BUF_SIZE - 1);
    if (len < 0) {
        perror("readlink");
        return ERROR;
    }
    out[len] = '\0';
    close(fd);
    return OK;
}

int get_fd(int pid, FILE *out) {
    char path[PATH_MAX];
    char line[BUF_SIZE];
    char entry_path[PATH_MAX];
    sprintf(path, "/proc/%d/fd", pid);
    DIR *dir;
    struct dirent *entry;
    int count = 0;
    dir = opendir(path);
    if (dir != NULL) {
        while ((entry = readdir(dir))!= NULL) {
            if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
            {
                sprintf(entry_path, "%s/%s", path, entry->d_name);
                ssize_t len = readlink(entry_path, line, BUF_SIZE - 1);
                if (len < 0) {
                    perror("readlink");
                    return ERROR;
                }
                line[len] = '\0';
                fprintf(out, "%s -> %s\n", entry->d_name, line);
            }
        }
        closedir(dir);
    } else {
        perror("opendir");
    }
    return OK;
}

int get_root(int pid, FILE *out) {
    char path[PATH_MAX];
    char line[BUF_SIZE];
    sprintf(path, "/proc/%d/root", pid);
    ssize_t len = readlink (path, line, BUF_SIZE - 1);
    if (len < 0) {
        perror("readlink");
        return ERROR;
    }
    line[len] = '\0';
    fprintf(out, "%s\n", line);
    return OK;
}

int get_stat(int pid, FILE *out) {
    char path[PATH_MAX];
    char line[BUF_SIZE];
    sprintf(path, "/proc/%d/stat", pid);
    FILE *f = fopen(path, "r");
    if (f == NULL) {
        perror("fopen");
        return -1;
    }
    fgets(line, BUF_SIZE - 1, f);

    char *rows[] = {"pid", "comm", "state", "ppid", "pgrp", "session", "tty_nr", "tpgid", "flags", "minflt", "cminflt", "majflt", "cmajflt", "utime", "stime", "cutime", "cstime", "priority", "nice", "num_threads", "itrealvalue", "starttime", "vsize", "rss", "rsslim", "startcode", "endcode", "startstack", "kstkesp", "kstkeip", "signal", "blocked", "sigignore", "sigcatch", "wchan", "nswap", "cnswap", "exit_signal", "processor", "rt_priority", "policy", "delayacct_blkio_ticks", "guest_time", "cguest_time", "start_data", "end_data", "start_brk", "arg_start", "arg_end", "env_start", "env_end", "exit_code"};
    
    char *row_inf = strtok(line, " ");
    char **row_head = rows;
    while (row_inf!= NULL && row_head < rows + sizeof(rows) / sizeof(char*) - 1) {
        if (strcmp(*row_head, "startcode") == 0 ||
            strcmp(*row_head, "endcode") == 0 ||
            strcmp(*row_head, "startstack") == 0 ||
            strcmp(*row_head, "start_data") == 0 ||
            strcmp(*row_head, "end_data") == 0 ||
            strcmp(*row_head, "start_brk") == 0 ||
            strcmp(*row_head, "arg_start") == 0 ||
            strcmp(*row_head, "arg_end") == 0 ||
            strcmp(*row_head, "env_start") == 0 ||
            strcmp(*row_head, "env_end") == 0
        ) {
            fprintf(out, "%s: 0x%lx\n", *row_head, strtoull(row_inf, NULL, 10));
        } else if (strcmp(*row_head, "vsize") == 0) {
            unsigned long bytes = strtoul(row_inf, NULL, 10);
            double vsize_kb = bytes / (1024.0);
            fprintf(out, "vsize: (%.2f KB) (%d pages)\n", vsize_kb, bytes / 4096);
        } else if (strcmp(*row_head, "rss") == 0) {
            unsigned long pages = strtoul(row_inf, NULL, 10);
            double rss_kb = pages * 4.;
            fprintf(out, "rss: (%.2f KB) (%d pages)\n", rss_kb, pages);
        } else {
            fprintf(out, "%s: %s\n", *row_head, row_inf);
        }
        
        row_inf = strtok(NULL, " ");
        row_head++;
    }
    
    fclose(f);
    return OK;
}

int get_status(int pid, FILE *out) {
    char path[PATH_MAX];
    char line[BUF_SIZE];
    sprintf(path, "/proc/%d/status", pid);
    FILE *f = fopen(path, "r");
    if (f == NULL) {
        perror("fopen");
        return -1;
    }
    while (fgets(line, BUF_SIZE - 1, f) != NULL) {
        fprintf(out, "%s", line);
    }
    fclose(f);
    fprintf(out, "%s\n", line);
    return OK;
}

int get_maps(int pid, FILE *out) {
    char path[PATH_MAX];
    char line[BUF_SIZE];
    unsigned long sum_pages = 0;
    sprintf(path, "/proc/%d/maps", pid);
    FILE *f = fopen(path, "r");
    if (f == NULL) {
        perror("fopen");
        return -1;
    }

    fprintf(out, "address                   n_pages  perms  offset  dev   inode                 pathname\n");
    while (fgets(line, BUF_SIZE - 1, f)!= NULL) {
        char *addresses = strtok(line, " ");
        char *perms = strtok(NULL, " ");
        char *offset = strtok(NULL, " ");
        char *dev = strtok(NULL, " ");
        char *inode = strtok(NULL, " ");
        char *pathname = strtok(NULL, "\n");

        char *start_addr = strtok(addresses, "-");
        char *end_addr = strtok(NULL, "-");

        unsigned long start = strtoul(start_addr, NULL, 16);
        unsigned long end = strtoul(end_addr, NULL, 16);
        unsigned long pages = (end - start) / 4096;
        sum_pages += pages;
        fprintf(out, "%012lx-%012lx %ld %s %s %s %s %s %s\n", start, end, pages, perms, offset, dev, inode, pathname, "");
    }
    sum_pages--;
    double total_size = sum_pages * 4.;

    fprintf(out, "total pages: %ld (%lf KB)\n", sum_pages, total_size);
    fclose(f);
    return OK;
}

int get_pagemap(int pid, FILE *out) {
    char path[PATH_MAX];
    char line[BUF_SIZE];
    sprintf(path, "/proc/%d/maps", pid);
    FILE *maps = fopen(path, "r");
    if (maps == NULL) {
        perror("fopen");
        return -1;
    }

    sprintf(path, "/proc/%d/pagemap", pid);
    FILE *pages = fopen(path, "r");
    if (pages == NULL) {
        perror("fopen");
        return -1;
    }

    fprintf(out, "%s-%s: %s %s %s %s %s\n", "page_start", "page_end", "frame_number", "shared", "soft_dirty", "in_ram", "in_swap");
    while (fgets(line, BUF_SIZE - 1, maps)!= NULL) {
        char *addr = strtok(line, " ");
        if (addr != NULL) {
            char *start_addr = strtok(addr, "-");
            char *end_addr = strtok(NULL, " ");
            if (start_addr != NULL && end_addr != NULL) {
                unsigned long start = strtoul(start_addr, NULL, 16);
                unsigned long end = strtoul(end_addr, NULL, 16);
                for (unsigned long page_addr = start; page_addr < end; page_addr += getpagesize()) {
                    long vpage_info;
                    fseek(pages, (page_addr / getpagesize()) * sizeof(vpage_info), SEEK_SET);
                    fread(&vpage_info, sizeof(vpage_info), 1, pages);
                    long in_ram = vpage_info & ((long) 1 << 63);
                    long in_swap = vpage_info & ((long) 1 << 62);
                    long shared = vpage_info & ((long) 1 << 61);
                    long soft_dirty = vpage_info & ((long) 1 << 55);
                    long frame_number = vpage_info & (((long) 1 << 55) - 1);
                    fprintf(out, "%lx-%lx %lx %s %s %s %s\n", page_addr, page_addr + getpagesize(),
                        frame_number, shared? "Yes" : "No", soft_dirty? "Yes" : "No",
                        in_ram? "Yes" : "No", in_swap? "Yes" : "No");
                }
            }  
        }
    }
    fclose(maps);
    fclose(pages);
}

int get_comm(int pid, FILE *out) {
    char path[PATH_MAX];
    char line[BUF_SIZE];
    sprintf(path, "/proc/%d/comm", pid);
    FILE *f = fopen(path, "r");
    if (f == NULL) {
        perror("fopen");
        return -1;
    }
    fgets(line, BUF_SIZE - 1, f);
    fclose(f);
    fprintf(out, "%s\n", line);
    return OK;
}

int get_task(int pid, FILE *out) {
    char path[PATH_MAX];
    char line[BUF_SIZE];
    char entry_path[PATH_MAX];
    sprintf(path, "/proc/%d/task", pid);
    DIR *dir;
    struct dirent *entry;
    dir = opendir(path);
    if (dir != NULL) {
        while ((entry = readdir(dir))!= NULL) {
            if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
            {
                fprintf(out, "%s\n", entry->d_name);
                // DIR *inner_dir;
                // struct dirent *inner_entry;
                // sprintf(path, "/proc/%d/task/%s", pid, entry->d_name);
                // inner_dir = opendir(path);
                // if (inner_dir != NULL) {
                //     while ((inner_entry = readdir(inner_dir))!= NULL) {
                //         if (strcmp(inner_entry->d_name, ".") != 0 && strcmp(inner_entry->d_name, "..") != 0)
                //         {
                //             fprintf(out, "  %s\n", inner_entry->d_name);
                //         }
                //     }
                //     closedir(inner_dir);
                // } else {
                //     perror("opendir");
                // }
            }
        }
        closedir(dir);
    } else {
        perror("opendir");
    }
    return OK;
}

int get_io(int pid, FILE *out) {
    char path[PATH_MAX];
    char line[BUF_SIZE];
    sprintf(path, "/proc/%d/io", pid);
    FILE *f = fopen(path, "r");
    if (f == NULL) {
        perror("fopen");
        return -1;
    }
    while (fgets(line, BUF_SIZE - 1, f)!= NULL) {
        fprintf(out, "%s", line);
    }
    fclose(f);
    fprintf(out, "%s\n", line);
    return OK;
}

int get_mem(int pid, FILE *out) {
    char path[PATH_MAX];
    char line[BUF_SIZE];
    sprintf(path, "/proc/%d/maps", pid);
    FILE *maps = fopen(path, "r");
    if (maps == NULL) {
        perror("fopen");
        return -1;
    }

    sprintf(path, "/proc/%d/mem", pid);
    FILE *mem = fopen(path, "r");
    if (mem == NULL) {
        perror("fopen");
        return -1;
    }

    while (fgets(line, BUF_SIZE - 1, maps)!= NULL) {
        char *addr = strtok(line, " ");
        char *perms = strtok(NULL, " ");
        char *offset = strtok(NULL, " ");
        char *dev = strtok(NULL, " ");
        char *inode = strtok(NULL, " ");
        char *pathname = strtok(NULL, "\n");
        if (addr != NULL) {
            char *start_addr = strtok(addr, "-");
            char *end_addr = strtok(NULL, " ");
            if (start_addr != NULL && end_addr != NULL) {
                unsigned long start = strtoul(start_addr, NULL, 16);
                unsigned long end = strtoul(end_addr, NULL, 16);
                unsigned long size = end - start;
                char *buffer = malloc(size);
                if (buffer == NULL) {
                    perror("malloc");
                    return -1;
                }                    
                fseek(mem, start , SEEK_SET);
                fread(buffer, size, 1, mem);
                fprintf(out, "region: %s-%s - %lu bytes\n", start_addr, end_addr, size);
                for (long i = 0; i < size; i++) {
                    fprintf(out, "%02x ", buffer[i]);
                    if ((i + 1) % 16 == 0) {
                        fprintf(out, "\n");
                    }
                }
                free(buffer);
                fprintf(out, "\n");
            }  
        }
    }
    fclose(maps);
    fclose(mem);
}

int get_mem_exe(int pid, FILE *out) {
    char path[PATH_MAX];
    char line[BUF_SIZE];
    char exe_buf[BUF_SIZE];
    get_exe_str(pid, exe_buf);
    sprintf(path, "/proc/%d/maps", pid);
    FILE *maps = fopen(path, "r");
    if (maps == NULL) {
        perror("fopen");
        return -1;
    }

    sprintf(path, "/proc/%d/mem", pid);
    FILE *mem = fopen(path, "r");
    if (mem == NULL) {
        perror("fopen");
        return -1;
    }

    while (fgets(line, BUF_SIZE - 1, maps)!= NULL) {
        char *addr = strtok(line, " ");
        char *perms = strtok(NULL, " ");
        char *offset = strtok(NULL, " ");
        char *dev = strtok(NULL, " ");
        char *inode = strtok(NULL, " ");
        char *pathname = strtok(NULL, " ");
        pathname[strlen(pathname) - 1] = '\0';
        if (addr != NULL && pathname && strcmp(pathname, exe_buf) == 0) {
            char *start_addr = strtok(addr, "-");
            char *end_addr = strtok(NULL, " ");
            if (start_addr != NULL && end_addr != NULL) {
                unsigned long start = strtoul(start_addr, NULL, 16);
                unsigned long end = strtoul(end_addr, NULL, 16);
                unsigned long size = end - start;
                char *buffer = malloc(size);
                if (buffer == NULL) {
                    perror("malloc");
                    return -1;
                }                    
                fseek(mem, start , SEEK_SET);
                fread(buffer, size, 1, mem);
                fprintf(out, "region: %s-%s - %lu bytes\n", start_addr, end_addr, size);
                for (long i = 0; i < size; i++) {
                    fprintf(out, "%02x ", buffer[i]);
                    if ((i + 1) % 16 == 0) {
                        fprintf(out, "\n");
                    }
                }
                free(buffer);
                fprintf(out, "\n");
            }  
        }
    }
    fclose(maps);
    fclose(mem);
}


int main(int argc, char *argv[]) {
    if (argc!= 2) {
        fprintf(stderr, "Usage: %s <pid>\n", argv[0]);
        return -1;
    }
    int pid = atoi(argv[1]);
    // pid = getpid();

    FILE *info = fopen("info.txt", "w");
    if (info == NULL) {
        perror("fopen");
        return -1;
    }

    FILE *stat = fopen("stat.txt", "w");
    if (stat == NULL) {
        perror("fopen");
        return -1;
    }

    FILE *mem = fopen("mem.txt", "w");
    if (mem == NULL) {
        perror("fopen");
        return -1;
    }

    FILE *pagemap = fopen("pagemap.txt", "w");
    if (pagemap == NULL) {
        perror("fopen");
        return -1;
    }

    FILE *fd = fopen("fd.txt", "w");
    if (fd == NULL) {
        perror("fopen");
        return -1;
    }

    FILE *task = fopen("task.txt", "w");
    if (task == NULL) {
        perror("fopen");
        return -1;
    }

    FILE *env = fopen("env.txt", "w");
    if (env == NULL) {
        perror("fopen");
        return -1;
    }

    FILE *io = fopen("io.txt", "w");
    if (io == NULL) {
        perror("fopen");
        return -1;
    }
    
    fprintf(info, "PID: %d\n", pid);
    fprintf(info, "Command line: ");
    get_cmdline(pid, info);
    fprintf(info, "CWD: ");
    get_cwd(pid, info);
    fprintf(info, "Executable: ");
    get_exe(pid, info);
    fprintf(info, "Root directory: ");
    get_root(pid, info);
    fprintf(info, "Comm: ");
    get_comm(pid, info);
    

    fprintf(fd, "======File descriptors======\n");
    get_fd(pid, fd);
    fprintf(fd, "============================\n");

    fprintf(task, "======Task======\n");
    get_task(pid, task);
    fprintf(task, "===================\n");

    fprintf(env, "======Environment======\n");
    get_environ(pid, env);
    fprintf(env, "=======================\n");

    fprintf(stat, "======Stat======\n");
    get_stat(pid, stat);
    fprintf(stat, "================\n");

    fprintf(stat, "======Status======\n");
    get_status(pid, stat);
    fprintf(stat, "================\n");

    fprintf(io, "======IO======\n");
    get_io(pid, io);
    fprintf(io, "==============\n");

    fprintf(stat, "======Maps======\n");
    get_maps(pid, stat);
    fprintf(stat, "================\n");

    fprintf(pagemap, "======Pagemap======\n");
    get_pagemap(pid, pagemap);
    fprintf(pagemap, "===================\n");

    fprintf(mem, "======Memory======\n");
    get_mem_exe(pid, mem);
    fprintf(mem, "====================\n");

    fclose(info);
    fclose(stat);
    fclose(mem);
    fclose(fd);
    fclose(task);
    fclose(env);
    fclose(io);
    fclose(pagemap);
}