#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user/user.h"

static int
issep(char c)
{
  return strchr(" -\r\t\n,./;", c) != 0;
}

static void
flush_number(int active, int value)
{
  if(active && (value % 5 == 0 || value % 6 == 0))
    printf("%d\n", value);
}

static void
sixfive(int fd)
{
  char c;
  int value;
  int in_number;
  int blocked;

  value = 0;
  in_number = 0;
  blocked = 0;
  while(read(fd, &c, 1) == 1){
    if('0' <= c && c <= '9'){
      if(!blocked){
        value = value * 10 + c - '0';
        in_number = 1;
      }
    } else if(issep(c)){
      flush_number(in_number, value);
      value = 0;
      in_number = 0;
      blocked = 0;
    } else {
      in_number = 0;
      blocked = 1;
      value = 0;
    }
  }

  flush_number(in_number, value);
}

int
main(int argc, char *argv[])
{
  int fd;
  int i;

  if(argc < 2){
    fprintf(2, "usage: sixfive file...\n");
    exit(1);
  }

  for(i = 1; i < argc; i++){
    fd = open(argv[i], O_RDONLY);
    if(fd < 0){
      fprintf(2, "sixfive: cannot open %s\n", argv[i]);
      exit(1);
    }
    sixfive(fd);
    close(fd);
  }

  exit(0);
}
