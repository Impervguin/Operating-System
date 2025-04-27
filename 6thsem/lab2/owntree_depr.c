#include <stdlib.h>
#include <sys/stat.h>
#include <dirent.h>
#include <limits.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#define OK 0
#define ERROR 1


int print_cwd_tree(int indent)
{
    struct stat statbuf;
    struct dirent *dirp;
    DIR *dp;
    int ret;
    if (lstat(".", &statbuf) < 0)
        return ERROR;
    
    if ((dp = opendir(".")) == NULL) 
        return ERROR;
    while ((dirp = readdir(dp)) != NULL) {
        if (strcmp(dirp->d_name, ".") == 0 || strcmp(dirp->d_name, "..") == 0)
            continue; 
        printf("%*c%s\n", indent, ' ', dirp->d_name);
        if (lstat(dirp->d_name, &statbuf) < 0)
            return ERROR;
        if (S_ISDIR(statbuf.st_mode)) {
            chdir(dirp->d_name);
            ret = print_cwd_tree(indent + 4);
            if (ret!= 0)
                return ret;
            chdir("..");
        }
    }
    if (closedir(dp) < 0)
        return 2;
    return OK;
}

int tree(char *path) {
    char *return_path;
    return_path = getcwd(NULL, 0);
    if (return_path == NULL)
        return ERROR;
    if (chdir(path) == -1) {
        perror("chdir");
        return ERROR;
    }
    print_cwd_tree(0);
    if (chdir(return_path) == -1) {
        perror("chdir");
        return ERROR;
    }
    free(return_path);
    return OK;
}

int main(int argc, char *argv[]) {
    if (argc!= 2) {
        fprintf(stderr, "Usage: %s <directory_path>\n", argv[0]);
        return ERROR;
    }
    return tree(argv[1]);
}