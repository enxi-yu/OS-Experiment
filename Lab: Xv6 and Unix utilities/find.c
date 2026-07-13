#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

void find(char *path, char *target)
{
    int fd;
    struct stat st;
    struct dirent de;
    char buf[512];
    char *p;
    
    // 打开目录
    if ((fd = open(path, 0)) < 0) {
        fprintf(2, "find: cannot open %s\n", path);
        return;
    }
    
    // 获取文件状态
    if (fstat(fd, &st) < 0) {
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
        return;
    }
    
    // 如果是文件而不是目录，比较文件名
    if (st.type != T_DIR) {
        // 从路径中提取文件名
        char *filename;
        for (p = path + strlen(path) - 1; p >= path && *p != '/'; p--)
            ;
        filename = p + 1;
        
        if (strcmp(filename, target) == 0) {
            printf("%s\n", path);
        }
        close(fd);
        return;
    }
    
    // 如果是目录，递归遍历
    // 构建路径缓冲区
    strcpy(buf, path);
    p = buf + strlen(buf);
    *p++ = '/';
    
    // 读取目录内容
    while (read(fd, &de, sizeof(de)) == sizeof(de)) {
        if (de.inum == 0)
            continue;
        
        // 跳过 "." 和 ".."
        if (strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0)
            continue;
        
        // 构建完整路径
        memmove(p, de.name, DIRSIZ);
        p[DIRSIZ] = 0;
        
        // 递归查找
        find(buf, target);
    }
    
    close(fd);
}

int main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(2, "Usage: find <directory> <filename>\n");
        exit(1);
    }
    
    find(argv[1], argv[2]);
    exit(0);
}
