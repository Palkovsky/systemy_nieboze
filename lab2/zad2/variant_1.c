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

typedef struct dirent dirent;

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

char *timeToString(time_t time){
  char *buff = calloc(50, sizeof(char));
  strftime(buff, 50, "%d.%m.%Y-%H:%M:%S", localtime(&time));
  return buff;
}

char* fileTypeToString(unsigned char id){
  switch(id){
  case DT_REG:
    return "file";
  case DT_DIR:
    return "dir";
  case DT_FIFO:
    return "fifo";
  case DT_SOCK:
    return "sock";
  case DT_CHR:
    return "char dev";
  case DT_BLK:
    return "block dev";
  case DT_LNK:
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

void traverseDirectory(const char* path, uint8_t operator, time_t timestamp){
  dirent *entry = NULL;
  struct stat fileinfo;
  DIR* dir;

  dir = opendir(path);
  if(!dir){
    printf("Invalid directory path: '%s'. Backing up...\n", path);
    return;
  }


  while((entry = readdir(dir)) != NULL){
    if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
      continue;

    char *absolute_path;
    char *file_type;
    uint64_t file_size;
    time_t last_access;
    time_t last_update;

    char *filename = entry->d_name;
    char *new_path = calloc(PATH_MAX + 1, sizeof(char));
    sprintf(new_path, "%s/%s", path, filename);

    if(entry->d_type == DT_LNK){
      char *parent_dir = realpath(path, NULL);
      absolute_path = calloc(PATH_MAX + 1, sizeof(char));
      strcpy(absolute_path, parent_dir);
      strcat(absolute_path, "/");
      strcat(absolute_path, entry->d_name);
      free(parent_dir);
    } else {
      absolute_path = (char *) realpath(new_path, NULL);
    }

    unsigned char f_type_id = entry->d_type;
    file_type = fileTypeToString(f_type_id);

    if(lstat(absolute_path, &fileinfo) < 0){
      printf("ERROR: %s\n", strerror(errno));
      free(new_path);
      free(absolute_path);
      continue;
    }

    file_size = fileinfo.st_size;
    last_access = fileinfo.st_atime;
    last_update = fileinfo.st_mtime;
    char* last_access_str = timeToString(last_access);
    char* last_update_str = timeToString(last_update);

    if(shouldPrint(last_update, operator, timestamp) == 1){
      printf("%s | %s | %lu bytes | %s | %s\n",
             absolute_path,
             file_type,
             file_size,
             last_access_str,
             last_update_str);
    }

    if(f_type_id == DT_DIR && f_type_id != DT_LNK){
      traverseDirectory(new_path, operator, timestamp);
    }

    free(new_path);
    free(absolute_path);
    free(last_access_str);
    free(last_update_str);
  }


  if(errno < 0){
    printf("ERROR: %s\n", strerror(errno));
  }
  closedir(dir);
}

int main(int argc, char **argv){
  if (argc != 4) printUsageAndExit();

  uint32_t len = strlen(argv[1]);
  if(argv[1][len-1] == '/'){
    argv[1][len-1] = 0;
  }

  const char *path = argv[1];
  uint8_t operator = parseOperator(argv[2]);
  time_t timestamp = parseDate(argv[3]);

  traverseDirectory(path, operator, timestamp);

  return 0;
}
