#include <stdlib.h>
#include <sys/stat.h>
#include <dirent.h>
#include <limits.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#define OK 0
#define ERROR 1

typedef int path_func_t(const char *, const struct stat *, int);

int dopath(const char *filename, int depth_level, path_func_t func) {
    struct stat statbuf;
    struct dirent *dirp;
    DIR *dp;
    int ret;
    if (lstat(filename, &statbuf) < 0)
        return ERROR;
    ret = func(filename, &statbuf, depth_level);
    if (!S_ISDIR(statbuf.st_mode)) 
        return ret;
    if (ret != 0)
        return ret;
    if ((dp = opendir(filename)) == NULL) 
        return ERROR;
    if (chdir(filename) == -1) {
        perror("chdir");
        return ERROR;
    }
    while ((dirp = readdir(dp)) != NULL) {
        if (strcmp(dirp->d_name, ".") != 0 && strcmp(dirp->d_name, "..") != 0)
            if ((ret = dopath(dirp->d_name, depth_level + 1, func)) != 0)
                return ret;
    }
    printf("xxx\n");
    if (chdir("..") == -1) {
        perror("chdir");
        return ERROR;
    }
    if (closedir(dp) < 0)
        return ERROR;
    
    return (ret);
}

int print_tree_func(const char *filename, const struct stat *statptr, int depth_level) {
    if (depth_level < 0)
        return ERROR;
    printf("%*s%s\n", depth_level * 2, "", filename);
    return OK;
}

int tree(const char *filename) {
    if (filename == NULL)
        return ERROR;

    return dopath(filename, 0, print_tree_func);
}



int main(int argc, char *argv[]) {
    if (argc!= 2) {
        fprintf(stderr, "Usage: %s <directory_path>\n", argv[0]);
        return ERROR;
    }
    return tree(argv[1]);
}
