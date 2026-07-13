#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/param.h"

char *exec_cmd[MAXARG];
int exec_mode = 0;

void find(char *path, char *target)
{
    int fd;
    struct stat st;
    struct dirent de;
    char buf[512];
    char *p;
    char *filename;
    int pid;
    int status;
    
    if ((fd = open(path, 0)) < 0) {
        fprintf(2, "find: cannot open %s\n", path);
        return;
    }
    
    if (fstat(fd, &st) < 0) {
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
        return;
    }
    
    if (st.type != T_DIR) {
        for (p = path + strlen(path) - 1; p >= path && *p != '/'; p--)
            ;
        filename = p + 1;
        
        if (strcmp(filename, target) == 0) {
            if (exec_mode) {
                pid = fork();
                if (pid < 0) {
                    fprintf(2, "find: fork failed\n");
                } else if (pid == 0) {
                    int i;
                    for (i = 0; exec_cmd[i] != 0; i++)
                        ;
                    exec_cmd[i] = path;
                    exec_cmd[i + 1] = 0;
                    
                    exec(exec_cmd[0], exec_cmd);
                    fprintf(2, "find: exec failed\n");
                    exit(1);
                } else {
                    wait(&status);
                }
            } else {
                printf("%s\n", path);
            }
        }
        close(fd);
        return;
    }
    
    strcpy(buf, path);
    p = buf + strlen(buf);
    *p++ = '/';
    
    while (read(fd, &de, sizeof(de)) == sizeof(de)) {
        if (de.inum == 0)
            continue;
        if (strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0)
            continue;
        
        memmove(p, de.name, DIRSIZ);
        p[DIRSIZ] = 0;
        
        find(buf, target);
    }
    
    close(fd);
}

int main(int argc, char *argv[])
{
    if (argc < 3) {
        fprintf(2, "Usage: find <directory> <filename> [-exec cmd...]\n");
        exit(1);
    }
    
    int i;
    int exec_pos = -1;
    for (i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-exec") == 0) {
            exec_pos = i;
            break;
        }
    }
    
    if (exec_pos >= 0) {
        exec_mode = 1;
        int j;
        for (j = 0; j < MAXARG - 2 && exec_pos + 1 + j < argc; j++) {
            exec_cmd[j] = argv[exec_pos + 1 + j];
        }
        exec_cmd[j] = 0;
    }
    
    find(argv[1], argv[2]);
    exit(0);
}
