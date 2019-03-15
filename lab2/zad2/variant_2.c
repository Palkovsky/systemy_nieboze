#define _XOPEN_SOURCE 500
#define _BSD_SOURCE

#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <ftw.h>
#include <libgen.h>

// Globals
uint8_t operator;
time_t timestamp;

void printUsageAndExit(){
  printf("Invalid parameters! USAGE:\n");
  printf("main.out [directory] [<, =, >] [DD:MM:YYYY-HH:MM:SS]\n");
  exit(1);
}

uint8_t parseOperator(const char* operator){
  if(strcmp(operator, ">") == 0) return 0;
  if(strcmp(operator, "<") == 0) return 1;
  if(strcmp(operator, "=") == 0) return 2;
  printf("Invalid operation symbol.\n");
  exit(3);
}

time_t parseDate(const char* datetime){
  struct tm tm;
  time_t timestamp;

  if (strptime(datetime, "%d.%m.%Y-%H:%M:%S", &tm) == NULL){
    printf("Invalid date format! Use: DD:MM:YYYY-HH:MM:SS\n");
    exit(4);
  }

  timestamp = mktime(&tm);
  if(timestamp < 0){
    printf("Unable to convert date to epoch.\n");
    exit(4);
  }

  return timestamp;
}

char *timestampToString(time_t time){
  char *buff = calloc(50, sizeof(char));
  strftime(buff, 50, "%d.%m.%Y-%H:%M:%S", localtime(&time));
  return buff;
}

char* fileTypeToString(unsigned char id){
  switch(id){
  case FTW_F:
    return "file";
  case FTW_D:
    return "dir";
  case FTW_DNR:
    return "dir";
  case FTW_SL:
    return "slink";
  case FTW_SLN:
    return "slink";
 }
  return "unknown";
}

int8_t shouldPrint(time_t op1, uint8_t operator, time_t op2){
  if(operator == 0){ // >
    if(op1 > op2) return 1;
  } else if(operator == 1){ // <
    if (op1 < op2) return 1;
  } else if(operator == 2){ // =
    if (op1 == op2) return 1;
  }
  return 0;
}

int printEntry(const char* path, const struct stat *fileinfo, int f_type_id, struct FTW *ftwbuf) {
  char *absolute_path;
  char *file_type;
  uint64_t file_size;
  time_t last_access;
  time_t last_update;

  if(f_type_id == FTW_SL || f_type_id == FTW_SLN){
    char *path_cpy = calloc(strlen(path) + 1, sizeof(char));
    strcpy(path_cpy, path);

    char *base_name  = basename(path_cpy);
    char *parent_dir = dirname(path_cpy);
    char *full_path = realpath(parent_dir, NULL);

    absolute_path = calloc(PATH_MAX + 1, sizeof(char));
    strcpy(absolute_path, full_path);
    strcat(absolute_path, "/");
    strcat(absolute_path, base_name);

    free(full_path);
    free(path_cpy);
  } else {
    absolute_path = (char *) realpath(path, NULL);
  }

  file_type = fileTypeToString(f_type_id);

  file_size = fileinfo->st_size;
  last_access = fileinfo->st_atime;
  last_update = fileinfo->st_mtime;
  char* last_access_str = timestampToString(last_access);
  char* last_update_str = timestampToString(last_update);

  if(shouldPrint(last_update, operator, timestamp) == 1){
    printf("%s | %s | %lu bytes | %s | %s\n",
           absolute_path,
           file_type,
           file_size,
           last_access_str,
           last_update_str);
  }

  free(absolute_path);
  free(last_access_str);
  free(last_update_str);

  return 0;
}

int main(int argc, char **argv){
  if (argc != 4) printUsageAndExit();

  uint32_t len = strlen(argv[1]);
  if(argv[1][len-1] == '/'){
    argv[1][len-1] = 0;
  }

  const char *path = argv[1];
  operator = parseOperator(argv[2]);
  timestamp = parseDate(argv[3]);

  if(nftw(path, printEntry, 50, FTW_PHYS) < 0){
    printf("Error traversing: %s", path);
  }

  return 0;
}
