#define _GNU_SOURCE

#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_ARGS_NO 5

typedef struct call_chain {
  char *command;
  char **argv;
  int argc;
  int endline;
  pid_t pid;
  struct call_chain *next;
} call_chain;

void print_usage_and_exit(){
  printf("Invalid parameters! USAGE: main.out [list_file]\n");
  exit(1);
}


// String manpilatuion in C. Yaaaay!
call_chain* parse_list(FILE *file){
  char*      line = NULL;
  size_t     bufsize = 0;
  call_chain *head = NULL;
  call_chain *it = NULL;

  // Read lines of file
  while (getline(&line, &bufsize, file) != -1) {
    char *token = strtok(line, "\n|");
    while(token != NULL){
      call_chain* node = calloc(1, sizeof(call_chain));
      node->next = NULL;

      node->command = calloc(strlen(token)+1, sizeof(char));
      strcpy(node->command, token);

      if(head == NULL) head = it = node;
      else{
        it->next = node;
        it = node;
      }
      token = strtok(NULL, "|");
    }

    it->endline = 1;
  }

  // Iterate over list and parse args
  it = head;
  while(it != NULL){
    char *token = strtok(it->command, " ");
    it->argv = calloc(MAX_ARGS_NO + 2, sizeof(char*));
    it->argc = 0;

    while(token != NULL && it->argc < MAX_ARGS_NO){
      // Trims new line from end
      token[strcspn(token, "\n")] = 0;
      it->argv[it->argc] = calloc(strlen(token) + 1, sizeof(char));
      strcpy(it->argv[it->argc], token);
      token = strtok(NULL, " ");
      it->argc++;
    }
    it->argv[it->argc] = NULL;

    it = it->next;
  }


  return head;
}

void print_list(call_chain *node){
  if(node == NULL) return;
  for(int i=0; i<node->argc+1; i++){
    printf("%s, ", node->argv[i]);
  }
  if(node->endline) {
    printf("\n");
    return;
  } else printf("| ");
  print_list(node->next);
}

void run_all(call_chain *head){
  if(head == NULL) return;

  call_chain* it = head;
  size_t cmd_count = 0;

  // Count commands in one line
  while(1){
    cmd_count++;
    if(it->endline == 1) break;
    it = it->next;
  }

  // Allocate space for pipe descriptors
  int fds[4] = {0};
  pipe(fds + 2);

  // 0 - previous read end
  // 1 - previous write end
  // 2 - current read end
  // 3 - current write end

  it = head;
  for(int i=0; i<cmd_count; i++){
    pid_t pid = fork();

    if(pid == 0){
      // Don't overwrite STDIN for first program
      if(i > 0){
        dup2(fds[0], STDIN_FILENO);
        close(fds[1]); // Close writing end.
      }

      // Dont overwirte STDOUT for last program
      if(i < cmd_count - 1){
        dup2(fds[3], STDOUT_FILENO);
        close(fds[2]); // Close reading end.
      }

      // After plumbing everything corectly - run program
      execvp(it->argv[0], it->argv);
    }

    // Writing descriptor in parent has to closed, otherwise grep/wc will hang waiting for more input.
    close(fds[3]);

    it->pid = pid;
    it = it->next;

    fds[0] = fds[2];
    fds[1] = fds[3];
    pipe(fds + 2);
  }


  int s;
  it = head;
  while(1){
    // Waits for every program in chain
    waitpid(it->pid, &s, 0);

    // If it's last one - print what was executed
    if(it->endline == 1){
      printf("PROGRAM: ");
      print_list(head);
      printf("===============\n");
      break;
    }

    it = it->next;
  }

  // Execute next line
  run_all(it->next);
}

int main(int argc, char **argv){
  if (argc != 2) print_usage_and_exit();

  FILE *list_file = fopen(argv[1], "r");
  if(!list_file){
    printf("Invalid path to list file\n.");
    exit(1);
  }

  call_chain* head = parse_list(list_file);
  fclose(list_file);
  run_all(head);

  return 0;
}
