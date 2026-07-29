#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "user/user.h"
#include "kernel/riscv.h"

#define SCAN_PAGES 32

static char marker[] = "This may help.";

static int
is_alnum(char c)
{
  return ('0' <= c && c <= '9') ||
         ('a' <= c && c <= 'z') ||
         ('A' <= c && c <= 'Z');
}

int
main(int argc, char *argv[])
{
  char *base;
  uint64 len = strlen(marker);
  uint64 nbytes = SCAN_PAGES * PGSIZE;

  base = sbrk(nbytes);
  if(base == SBRK_ERROR)
    exit(1);

  for(uint64 i = 0; i + 16 + len < nbytes; i++){
    if(memcmp(base + i, marker, len) != 0)
      continue;

    char *secret = base + i + 16;
    if(!is_alnum(secret[0]))
      continue;

    printf("%s\n", secret);
    exit(0);
  }

  exit(1);
}
