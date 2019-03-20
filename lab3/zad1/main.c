#define _XOPEN_SOURCE
#define _BSD_SOURCE

#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>

typedef struct dirent dirent;

void printUsageAndExit(){
  printf("Invalid parameters! USAGE:\n");
  printf("./main.out [directory]\n");
  exit(1);
}

void traverseDirectory(const char* path){
  dirent *entry = NULL;
  DIR* dir;

  dir = opendir(path);
  if(!dir){
    printf("Invalid directory path: '%s'. Backing up...\n", path);
    return;
  }

  // Perform ls -l call
  pid_t pid = fork();
  if(pid == 0){ // child process
    printf("PATH: %s, PID: %d\n", path, getpid());
    execl("/bin/ls", "ls", "-l", path, NULL);
  }else{
    int s;
    waitpid(pid, &s, WUNTRACED | WCONTINUED);
  }

  while((entry = readdir(dir)) != NULL){
    if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
      continue;

    if(entry->d_type != DT_DIR)
      continue;

    char *dirname = entry->d_name;
    char *new_path = calloc(PATH_MAX + 1, sizeof(char));
    sprintf(new_path, "%s/%s", path, dirname);

    pid_t pid = fork();
    if(pid == 0) { // Child should explore next branch.
      traverseDirectory(new_path);
      exit(0);
    }else { // Wait for children to finish exploring branch
      int s;
      waitpid(pid ,&s, WUNTRACED | WCONTINUED);
    }

    free(new_path);
  }

  closedir(dir);
}

int main(int argc, char **argv){
  if (argc != 2) printUsageAndExit();

  uint32_t len = strlen(argv[1]);
  if(argv[1][len-1] == '/'){
    argv[1][len-1] = 0;
  }

  const char *path = argv[1];
  traverseDirectory(path);
  return 0;
}
