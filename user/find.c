#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"
#include "user/user.h"

#define MAXPATH 512
#define MAXEXECARGS 10

static char *
basename(char *path)
{
  char *p;

  for(p = path + strlen(path); p >= path && *p != '/'; p--)
    ;
  return p + 1;
}

static int
isdot(char *name)
{
  return strcmp(name, ".") == 0 || strcmp(name, "..") == 0;
}

static void
runexec(char *path, char **cmdv, int cmdc)
{
  char *argv[MAXEXECARGS];
  int i;

  if(cmdc + 1 >= MAXEXECARGS){
    fprintf(2, "find: too many exec arguments\n");
    exit(1);
  }

  for(i = 0; i < cmdc; i++)
    argv[i] = cmdv[i];
  argv[cmdc] = path;
  argv[cmdc + 1] = 0;

  if(fork() == 0){
    exec(argv[0], argv);
    fprintf(2, "find: exec %s failed\n", argv[0]);
    exit(1);
  }
  wait(0);
}

static void
maybe_match(char *path, char *name, char **cmdv, int cmdc)
{
  if(strcmp(basename(path), name) != 0)
    return;

  if(cmdc == 0)
    printf("%s\n", path);
  else
    runexec(path, cmdv, cmdc);
}

static void
find(char *path, char *name, char **cmdv, int cmdc)
{
  char buf[MAXPATH], *p;
  int fd;
  struct dirent de;
  struct stat st;

  fd = open(path, O_RDONLY);
  if(fd < 0){
    fprintf(2, "find: cannot open %s\n", path);
    return;
  }
  if(fstat(fd, &st) < 0){
    fprintf(2, "find: cannot stat %s\n", path);
    close(fd);
    return;
  }

  maybe_match(path, name, cmdv, cmdc);

  if(st.type != T_DIR){
    close(fd);
    return;
  }

  if(strlen(path) + 1 + DIRSIZ + 1 > sizeof(buf)){
    fprintf(2, "find: path too long\n");
    close(fd);
    return;
  }

  strcpy(buf, path);
  p = buf + strlen(buf);
  *p++ = '/';
  while(read(fd, &de, sizeof(de)) == sizeof(de)){
    if(de.inum == 0)
      continue;
    memmove(p, de.name, DIRSIZ);
    p[DIRSIZ] = 0;
    if(isdot(p))
      continue;
    find(buf, name, cmdv, cmdc);
  }

  close(fd);
}

int
main(int argc, char *argv[])
{
  int execi;

  if(argc < 3){
    fprintf(2, "usage: find path name [-exec cmd args...]\n");
    exit(1);
  }

  for(execi = 3; execi < argc; execi++){
    if(strcmp(argv[execi], "-exec") == 0)
      break;
  }

  if(execi == argc){
    find(argv[1], argv[2], 0, 0);
  } else {
    if(execi + 1 >= argc){
      fprintf(2, "find: missing command after -exec\n");
      exit(1);
    }
    find(argv[1], argv[2], &argv[execi + 1], argc - execi - 1);
  }

  exit(0);
}
