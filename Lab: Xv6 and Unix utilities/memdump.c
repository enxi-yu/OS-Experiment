#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"

void memdump(char *fmt, char *data);

int
main(int argc, char *argv[])
{
  if(argc == 1){
    printf("Example 1:\n");
    int a[2] = { 61810, 2025 };
    memdump("ii", (char*) a);
    
    printf("Example 2:\n");
    memdump("S", "a string");
    
    printf("Example 3:\n");
    char *s = "another";
    memdump("s", (char *) &s);

    struct sss {
      char *ptr;
      int num1;
      short num2;
      char byte;
      char bytes[8];
    } example;
    
    example.ptr = "hello";
    example.num1 = 1819438967;
    example.num2 = 100;
    example.byte = 'z';
    strcpy(example.bytes, "xyzzy");
    
    printf("Example 4:\n");
    memdump("pihcS", (char*) &example);
    
    printf("Example 5:\n");
    memdump("sccccc", (char*) &example);
  } else if(argc == 2){
    // format in argv[1], up to 512 bytes of data from standard input.
    char data[512];
    int n = 0;
    memset(data, '\0', sizeof(data));
    while(n < sizeof(data)){
      int nn = read(0, data + n, sizeof(data) - n);
      if(nn <= 0)
        break;
      n += nn;
    }
    memdump(argv[1], data);
  } else {
    printf("Usage: memdump [format]\n");
    exit(1);
  }
  exit(0);
}

void memdump(char *fmt, char *data)
{
    char *ptr = data;  // 当前读取位置
    int i;
    
    for (i = 0; fmt[i] != 0; i++) {
        switch (fmt[i]) {
            case 'i': {
                // 读取 4 字节作为 32 位整数
                int val = *(int *)ptr;
                printf("%d\n", val);
                ptr += 4;
                break;
            }
            case 'p': {
                // 读取 8 字节作为 64 位整数（十六进制）
                long val = *(long *)ptr;
                printf("%lx\n", val);
                ptr += 8;
                break;
            }
            case 'h': {
                // 读取 2 字节作为 16 位整数
                short val = *(short *)ptr;
                printf("%d\n", val);
                ptr += 2;
                break;
            }
            case 'c': {
                // 读取 1 字节作为 ASCII 字符
                char val = *(char *)ptr;
                printf("%c\n", val);
                ptr += 1;
                break;
            }
            case 's': {
                // 读取 8 字节作为指针，然后打印指向的字符串
                char *str_ptr = *(char **)ptr;
                printf("%s\n", str_ptr);
                ptr += 8;
                break;
            }
            case 'S': {
                // 打印剩余所有字节作为字符串（直到遇到 '\0'）
                printf("%s\n", ptr);
                // S 是最后一种格式，处理完就结束
                return;
            }
            default:
                // 忽略未知格式字符
                break;
        }
    }
}
