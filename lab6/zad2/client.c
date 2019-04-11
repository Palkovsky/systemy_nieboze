#define _GNU_SOURCE

#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <mqueue.h>

#include "chat.h"

void read_instructions(FILE *f);
void exec_instruction(char *instruction);
void handle_exit();
void send_request();

mqd_t server_qd; // Server queue descriptor
mqd_t client_qd; // Client queue descriptor
int client_id; // Given by server. Required for communication.
ReqMsg msg;

int main(int argc, char **argv){
  signal(SIGINT, handle_exit);

  printf("Server Queue Descriptor: %d\n", server_qd);
  printf("Private Queue Descriptor: %d\n", client_qd);

  read_instructions(stdin);

  printf("========== Chat client ===========\n");
  while(1){
  }

  return 0;
}

/*
 * Reads instructions from specified fd.
 */
void read_instructions(FILE *f){
  char *buff = calloc(MAX_MSG_SIZE, sizeof(char));
  size_t size;

  while(1){
    if(getline(&buff, &size, f) == -1) return;
    exec_instruction(buff);
  }

  free(buff);
}

void exec_instruction(char *instruction){

  printf("Executing: %s", instruction);

  if(strncmp(instruction, "ECHO ", 5) == 0){
    instruction += 5;

  }else if(strncmp(instruction, "TOALL ", 6) == 0){
    instruction += 6;

  }else if(strncmp(instruction, "TOFRIENDS ", 10) == 0){
    instruction += 10;

  }else if(strncmp(instruction, "TOONE ", 6) == 0){
    instruction += 6;
    char *token = strtok(instruction, " ");
    int target = atoi(token);
    token = strtok(NULL, "");

  }else if(strncmp(instruction, "LIST", 4) == 0){

  }else if(strncmp(instruction, "STOP", 4) == 0){

  }else if(strncmp(instruction, "FRIENDS ", 8) == 0){
    instruction += 8;


  }else if(strncmp(instruction, "READ ", 5) == 0){
    instruction += 5;

    char *path = calloc(256, sizeof(char));
    strcpy(path, instruction);
    path[strcspn(path, "\n")] = 0;


    FILE *f = fopen(path, "r");
    if(f == NULL){
      printf("Unable to open %s. %s\n", path, strerror(errno));
      free(path);
      return;
    }

    read_instructions(f);
    fclose(f);
    free(path);

  } else {
    printf("Unrecognizale command: %s", instruction);
  }
}

/*
 * CTRL+C handler
 */
void handle_exit(){
  exit(1);
}
