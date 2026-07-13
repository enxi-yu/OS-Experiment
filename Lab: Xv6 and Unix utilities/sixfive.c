#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int is_digit(char c) {
    return c >= '0' && c <= '9';
}

int is_separator(char c) {
    // 分隔符：空格、制表符、换行、点、斜杠、连字符
    return c == ' ' || c == '\t' || c == '\n' || 
           c == '.' || c == '/' || c == '-';
}

void process_file(char *filename) {
    int fd;
    char c;
    int num = 0;
    int in_number = 0;
    
    fd = open(filename, 0);
    if (fd < 0) {
        fprintf(2, "sixfive: cannot open %s\n", filename);
        return;
    }
    
    // 逐字符读取
    while (read(fd, &c, 1) > 0) {
        if (is_digit(c)) {
            num = num * 10 + (c - '0');
            in_number = 1;
        } else if (is_separator(c)) {
            if (in_number) {
                if (num % 5 == 0 || num % 6 == 0) {
                    printf("%d\n", num);
                }
                num = 0;
                in_number = 0;
            }
        } else {
            // 其他字符（如字母），重置数字状态
            in_number = 0;
            num = 0;
        }
    }
    
    // 文件末尾
    if (in_number) {
        if (num % 5 == 0 || num % 6 == 0) {
            printf("%d\n", num);
        }
    }
    
    close(fd);
}

int main(int argc, char *argv[]) {
    int i;
    
    if (argc < 2) {
        fprintf(2, "Usage: sixfive <files...>\n");
        exit(1);
    }
    
    for (i = 1; i < argc; i++) {
        process_file(argv[i]);
    }
    
    exit(0);
}
