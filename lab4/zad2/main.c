#define _GNU_SOURCE

#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>

#define ARCHIVE "archiwum"

typedef struct watch_entry {
  char* path;
  char* name;
  FILE* fp;
  uint32_t interval;
  uint32_t copies_made;
  uint8_t paused;
  pid_t pid;
  struct watch_entry* next;
} watch_entry;

watch_entry *monitored_entry = NULL;

void print_usage_and_exit(){
  printf("Invalid parameters! USAGE: main.out [list_file]\n");
  exit(1);
}

uint64_t filesize(FILE *f){
  fseek(f, 0, SEEK_END);
  uint64_t sz = ftell(f);
  fseek(f, 0, SEEK_SET);
  return sz;
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

void watch_for_modification(watch_entry *entry){
  uint32_t copies = 0;
  uint64_t last_update;
  struct stat stat;

  // Load whole file into memory
  uint64_t filesz = filesize(entry->fp);
  uint8_t  *buff = calloc(filesz, sizeof(uint8_t));
  fread(buff, sizeof(uint8_t), filesz, entry->fp);

  // Read last modification date
  lstat(entry->path, &stat);
  last_update = stat.st_mtime;

  while(1){

    while(monitored_entry->paused == 1) pause();

    lstat(entry->path, &stat);
    if(stat.st_mtime > last_update){
      // Update date
      last_update = stat.st_mtime;
      copies++;
      monitored_entry->copies_made = copies;

      char *archived_path = filepath_with_timestamp(entry->name, last_update);
      //printf("%s modified ==> %s, PID: %d\n", entry->path, archived_path, getpid());

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
      fread(buff, sizeof(uint8_t), filesz, entry->fp);
    }

    sleep(entry->interval);
  }
}

void handle_stop(){
  monitored_entry->paused = 1;
}

void handle_start(){
  monitored_entry->paused = 0;
}

void handle_raport(){
  printf("Process %d made %d copies of %s.\n", getpid(), monitored_entry->copies_made, monitored_entry->path);
}

void start_processes(watch_entry *head, int counter){
  if(head == NULL){
    printf("%d monitors started!\n", counter);
    return;
  }

  pid_t pid = fork();
  if(pid == 0){ // child
    monitored_entry = head;
    signal(SIGUSR1, handle_stop);
    signal(SIGUSR2, handle_start);
    signal(SIGTSTP, handle_raport);
    watch_for_modification(head);
  }else {
    head->pid = pid;
    start_processes(head->next, counter + 1);
  }
}

void end_processes(watch_entry *head){
  if(head == NULL) return;
  kill(head->pid, SIGTSTP);
  usleep(10e3);
  kill(head->pid, SIGKILL);
  end_processes(head->next);
}

void list_process(watch_entry *proc){
  printf("PID: %d; Monitoring: %s; Paused: %d\n", proc->pid, proc->path, proc->paused);
}

void list_processes(watch_entry *head){
  if(head == NULL)  return;
  list_process(head);
  list_processes(head->next);
}

void stop_process(watch_entry *proc){
  if(kill(proc->pid, SIGUSR1) != 0)
    printf("Unable to stop process with pid %d.\n", proc->pid);
  else proc->paused = 1;
}

void stop_processes(watch_entry *head){
  if(head == NULL) return;
  stop_process(head);
  stop_processes(head->next);
}

void resume_process(watch_entry *proc){
  if(kill(proc->pid, SIGUSR2) != 0)
    printf("Unable to resume process with pid %d.\n", proc->pid);
  else proc->paused = 0;
}

void resume_processes(watch_entry *head){
  if(head == NULL) return;
  resume_process(head);
  resume_processes(head->next);
}

watch_entry *find_process_by_pid(watch_entry *head, pid_t pid){
  if(head == NULL) return NULL;
  if(head->pid == pid) return head;
  return find_process_by_pid(head->next, pid);
}

void wait_for_commands(watch_entry *head){
  while(1){
    char input[255];
    printf("Command: ");
    scanf ("%[^\n]%*c", input);

    if(strcmp(input, "help") == 0){
      printf("list        - lists all monitor processes\n");
      printf("stop [pid]  - stops process with given pid\n");
      printf("stop all    - stops all processes\n");
      printf("start [pid] - resumes process with given pid\n");
      printf("start all   - resumes all processes\n");
      printf("end         - finish monitoring\n");
    } else if(strcmp(input, "list") == 0){
      list_processes(head);
    }else if(strcmp(input, "stop all") == 0){
      stop_processes(head);
    }else if(strcmp(input, "start all") == 0){
      resume_processes(head);
    }else if(strncmp("start", input, 5) == 0){ // check if prefixed with 'start'
     strtok(input, " ");
     char *token = strtok(NULL, " ");
     unsigned long pid = strtoul(token, NULL, 10);
     watch_entry *proc = find_process_by_pid(head, pid);

     if(proc == NULL){
       printf("Invalid PID\n");
       continue;
     }
     resume_process(proc);
     list_process(proc);

    }else if(strncmp("stop", input, 4) == 0){ // check if prefixed with 'stop'
      strtok(input, " ");
      char *token = strtok(NULL, " ");
      unsigned long pid = strtoul(token, NULL, 10);
      watch_entry *proc = find_process_by_pid(head, pid);

      if(proc == NULL){
        printf("Invalid PID\n");
        continue;
      }
      stop_process(proc);
      list_process(proc);

    }else if(strcmp(input, "end") == 0){
      printf("================ SUMMARY ====================\n");
      end_processes(head);
      return;
    } else{
      printf("Invalid command. Use 'help'\n");
    }
  }
}

int main(int argc, char **argv){
  if (argc != 2) print_usage_and_exit();

  FILE *list_file;

  list_file = fopen(argv[1], "r");
  if(!list_file){
    printf("Invalid path to list file\n.");
    exit(1);
  }

  watch_entry *entries = parse_list(list_file);
  fclose(list_file);

  // Make sure that archive directory exists
  mkdir(ARCHIVE, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

  start_processes(entries, 0);
  list_processes(entries);
  wait_for_commands(entries);
  dispose_entries(entries);

  return 0;
}
