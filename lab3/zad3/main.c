#define _GNU_SOURCE

#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>

#define ARCHIVE "archiwum"

// Globals
int32_t max_cpu_time;
int32_t max_vm;

// Custom structs
typedef struct watch_entry {
  char* path;
  char* name;
  FILE* fp;
  uint32_t interval;
  pid_t pid;
  struct watch_entry* next;
} watch_entry;

void print_usage_and_exit(){
  printf("Invalid parameters! USAGE:\n");
  printf("prog [list_file] [seconds] [mode - 0/1] [max_cpu_time_seconds] [max_vm_megabytes]\n");
  exit(1);
}

uint64_t filesize(FILE *f){
  fseek(f, 0, SEEK_END);
  uint64_t sz = ftell(f);
  fseek(f, 0, SEEK_SET);
  return sz;
}

double timeval_to_double(struct timeval *tv){
  return tv->tv_sec + (double) tv->tv_usec/1000000.0;
}

char *filepath_with_timestamp(const char *filename, time_t timestamp){
  char *time_buff = calloc(50, sizeof(char));
  strftime(time_buff, 50, "%Y-%m-%d_%H-%M-%S", localtime(&timestamp));
  char *buff = calloc(strlen(filename) + 100, sizeof(char));
  sprintf(buff, "%s/%s_%s", ARCHIVE, filename, time_buff);
  free(time_buff);
  return buff;
}

watch_entry* parse_list(FILE *file){
  char*        line = NULL;
  size_t       bufsize = 0;
  ssize_t      bytes_read;
  watch_entry* head = NULL;
  watch_entry* cur  = NULL;
  uint32_t     i = 0;
  struct stat  stat;

  while ((bytes_read = getline(&line, &bufsize, file)) != -1) {
    char   *token;
    char   *filename;
    char   *filepath;
    int32_t seconds;

    i++;

    token    = strtok(line, " ,");
    filename = calloc(strlen(token) + 1, sizeof(char));
    strcpy(filename, token);

    token    = strtok(NULL, " ,");
    filepath = calloc(strlen(token) + 1, sizeof(char));
    strcpy(filepath, token);

    token    = strtok(NULL, " ,");
    seconds  = strtol(token, NULL, 10);

    uint8_t err = 0;

    // Validations here
    if(strcmp(strtok(line, " "), "\n") == 1){ //if there's more than endline => it's invalid
      printf("Invalid line format at: %d\n", i);
      err = 1;
    }

    if(seconds <= 0){
      printf("Negative amount of seconds in line %d.\n", i);
      err = 1;
    }

    if(filepath[strlen(filepath) - 1] == '/'){
      filepath[strlen(filepath) - 1] = 0;
    }

    char *full_path = calloc(strlen(filename) + strlen(filepath) + 10, sizeof(char));
    sprintf(full_path, "%s/%s", filepath, filename);
    free(filepath);

    FILE* fp = fopen(full_path, "r");
    if(!fp){
      printf("Unable to open %s specified at line %d.\n", full_path, i);
      err = 1;
    }

    // We can watch only regular files
    lstat(full_path, &stat);
    if(!S_ISREG(stat.st_mode)){
      printf("%s is not regular file. Line %d\n", full_path, i);
      err = 1;
    }

    if(err == 1){
      free(filename);
      free(full_path);
      continue;
    }

    watch_entry *entry = calloc(1, sizeof(watch_entry));
    entry->path     = full_path;
    entry->name     = filename;
    entry->fp       = fp;
    entry->interval = seconds;

    if(head == NULL) head = cur = entry;
    else {
      cur->next = entry;
      cur       = entry;
    }
  }

  return head;
}

void dispose_entries(watch_entry *head){
  if(head == NULL) return;
  fclose(head->fp);
  free(head->path);
  free(head->name);
  dispose_entries(head->next);
}

int watch_for_modification_mode0(watch_entry *entry, int32_t seconds){
  uint64_t start_time = time(NULL);
  uint32_t copies = 0;
  uint64_t last_update;
  struct stat stat;

  // Load whole file into memory
  uint64_t filesz = filesize(entry->fp);
  uint8_t  *buff = calloc(filesz, sizeof(uint8_t));
  if(buff == NULL){
    printf("Unable to allocate space for buffer. PID %d. Exiting...\n", getpid());
    return 0;
  }
  fread(buff, sizeof(uint8_t), filesz, entry->fp);

  // Read last modification date
  lstat(entry->path, &stat);
  last_update = stat.st_mtime;

  while(time(NULL) - start_time < seconds){
    lstat(entry->path, &stat);
    if(stat.st_mtime > last_update){
      // Update date
      last_update = stat.st_mtime;
      copies++;

      char *archived_path = filepath_with_timestamp(entry->name, last_update);
      printf("%s modified ==> %s, PID: %d\n", entry->path, archived_path, getpid());

      // zapis here
      FILE *f = fopen(archived_path, "w");
      if(!f){
        printf("Unable to create copy of modified %s\n", entry->path);
      } else {
        fwrite(buff, sizeof(uint8_t), filesz, f);
        fclose(f);
      }
      free(archived_path);

      // Reload file into memory again
      filesz = filesize(entry->fp);
      buff = realloc(buff, filesz * sizeof(uint8_t));
      if(buff == NULL){
        printf("Unable to allocate space for buffer. PID %d.  Exiting...\n", getpid());
        return 0;
      }
      fread(buff, sizeof(uint8_t), filesz, entry->fp);
    }

    sleep(entry->interval);
  }

  return copies;
}

int watch_for_modification_mode1(watch_entry *entry, int32_t seconds){
  uint64_t start_time = time(NULL);
  uint32_t copies = 0;
  uint64_t last_update;
  struct stat stat;

  // Read last modification date
  lstat(entry->path, &stat);
  last_update = stat.st_mtime;

  while(time(NULL) - start_time < seconds){
    lstat(entry->path, &stat);
    if(stat.st_mtime != last_update){
      // Update date
      last_update = stat.st_mtime;
      copies++;

      char *archived_path = filepath_with_timestamp(entry->name, last_update);
      printf("%s modified ==> %s, PID: %d\n", entry->path, archived_path, getpid());

      pid_t pid = fork();
      if(pid == 0){
        execl("/bin/cp", "cp", entry->path, archived_path, NULL);
      } else {
        int s;
        waitpid(pid, &s, 0);
      }
      free(archived_path);
    }
    sleep(entry->interval);
  }

  return copies;
}

void print_rusage(int copies){
  struct rusage rusage_self;
  struct rusage rusage_children;
  getrusage(RUSAGE_SELF, &rusage_self);
  getrusage(RUSAGE_CHILDREN, &rusage_children);

  // rusage struct has more fields, but i find these ones most interesting
  printf("============== PROCESS %d SUMMARY ====================\n", getpid());
  printf("Copies made: %d\n", copies);
  printf("User Time[Self]: %lfs\n", timeval_to_double(&rusage_self.ru_utime));
  printf("System Time[Self]: %lfs\n", timeval_to_double(&rusage_self.ru_stime));
  printf("User Time[Children]: %lfs\n", timeval_to_double(&rusage_children.ru_utime));
  printf("System Time[Children]: %lfs\n", timeval_to_double(&rusage_children.ru_stime));
  printf("Shared memory size[Total]: %ldkB\n", rusage_self.ru_ixrss + rusage_children.ru_ixrss);
  printf("Data size[Total]: %ldkB\n", rusage_self.ru_idrss + rusage_children.ru_idrss);
  printf("Stack size[Total]: %ldkB\n", rusage_self.ru_isrss + rusage_children.ru_isrss);
  printf("Context Switches[Total]: %ld\n", rusage_self.ru_nvcsw + rusage_self.ru_nivcsw + rusage_children.ru_nvcsw + rusage_children.ru_nivcsw);
}

void start_processes(watch_entry *head, int32_t seconds, int8_t mode){
  if(head == NULL)  return;

  pid_t pid = fork();
  if(pid == 0){ // child
    head->pid = getpid();

    // Set user-specified hard limits
    struct rlimit limit;
    getrlimit(RLIMIT_CPU, &limit);
    limit.rlim_max = max_cpu_time;
    if(limit.rlim_cur > limit.rlim_max) limit.rlim_cur = limit.rlim_max;
    if(setrlimit(RLIMIT_CPU, &limit) < 0){
      printf("Unable to set specified cpu time limit for PID %d. %s\n", getpid(), strerror(errno));
      exit(0);
    }

    getrlimit(RLIMIT_AS, &limit);
    limit.rlim_max = max_vm;
    if(limit.rlim_cur > limit.rlim_max) limit.rlim_cur = limit.rlim_max;
    if(setrlimit(RLIMIT_AS, &limit) < 0){
      printf("Unable to set specified adress space limit for PID %d. %s\n", getpid(), strerror(errno));
      exit(0);
    }

    int copies;
    if(mode == 0) copies =  watch_for_modification_mode0(head, seconds);
    else          copies  = watch_for_modification_mode1(head, seconds);
    print_rusage(copies);
    exit(copies);
  }else start_processes(head->next, seconds, mode);
}

void wait_for_processes(watch_entry *head){
  while(head != NULL){
    int s = 0;
    waitpid(-1, &s, 0);
    head = head->next; // list is used as counter here
  }
}

int main(int argc, char **argv){
  if (argc != 6) print_usage_and_exit();

  int8_t  mode;
  int32_t seconds;
  FILE *list_file;

  seconds = strtol(argv[2], NULL, 10);
  if(seconds <= 0){
    printf("Accepting only positive amount of seconds.\n");
    exit(1);
  }

  if(strcmp(argv[3], "0") == 0)      mode = 0;
  else if(strcmp(argv[3], "1") == 0) mode = 1;
  else {
    printf("Invalid mode. Available modes: 0/1\n");
    exit(1);
  }

  max_cpu_time = strtol(argv[4], NULL, 10);
  max_vm       = strtol(argv[5], NULL, 10) * 1000000;
  if(max_cpu_time <= 0){
    printf("Max CPU time limit must be positive!\n");
    exit(1);
  }
  if(max_vm <= 0){
    printf("Max virutal memory limit must be postitive!\n");
    exit(1);
  }

  list_file = fopen(argv[1], "r");
  if(!list_file){
    printf("Invalid path to list file\n.");
    exit(1);
  }

  watch_entry *entries = parse_list(list_file);
  fclose(list_file);

  // Make sure that archive directory exists
  mkdir(ARCHIVE, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

  start_processes(entries, seconds, mode);
  wait_for_processes(entries);
  dispose_entries(entries);

  return 0;
}
