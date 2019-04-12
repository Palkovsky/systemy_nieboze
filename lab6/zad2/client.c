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

#define WAIT 0
#define NOWAIT 1

void read_instructions(FILE *f);
void exec_instruction(char *instruction);
void handle_exit();
void handle_server_messages();
void handle_server_shutdown();
void send_request();
char* gen_queue_name();

// Requests
void send_init();
void send_stop();
void send_echo(const char *message);
void send_list();
void send_friends(const char *friends);
void send_to_all(const char *message);
void send_to_friends(const char *message);
void send_to_one(int target_id, const char *message);

char *q_name;
mqd_t server_qd; // Server queue descriptor
mqd_t client_qd; // Client queue descriptor
int client_id; // Given by server. Required for communication.
ReqMsg msg;

int main(int argc, char **argv){
  signal(SIGINT, handle_exit);

  // Connects to server queue
  server_qd = mq_open(SERVER_NAME, O_WRONLY);
  if(server_qd < 0){
    printf("Error while conecting to server: %s\n", strerror(errno));
    exit(1);
  }

  struct mq_attr attr;
  attr.mq_flags = 0;
  attr.mq_maxmsg = 10;
  attr.mq_msgsize = sizeof(ReqMsg);
  attr.mq_curmsgs = 0;

  // Creates client queue
  q_name = gen_queue_name();
  mq_unlink(q_name);
  client_qd = mq_open(q_name, O_RDONLY | O_CREAT, Q_PERM, &attr);
  if(client_qd < 0){
    printf("Error while opening server: %s\n", strerror(errno));
    exit(1);
  }

  printf("Server Queue Descriptor: %d\n", server_qd);
  printf("Private Queue Descriptor: %d\n", client_qd);

  send_init();
  read_instructions(stdin);

  printf("========== Chat client ===========\n");
  while(1){
    handle_server_messages();
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
    send_echo(instruction);

  }else if(strncmp(instruction, "TOALL ", 6) == 0){
    instruction += 6;
    send_to_all(instruction);

  }else if(strncmp(instruction, "TOFRIENDS ", 10) == 0){
    instruction += 10;
    send_to_friends(instruction);

  }else if(strncmp(instruction, "TOONE ", 6) == 0){
    instruction += 6;
    char *token = strtok(instruction, " ");
    int target = atoi(token);
    token = strtok(NULL, "");
    send_to_one(target, token);

  }else if(strncmp(instruction, "LIST", 4) == 0){
    send_list();

  }else if(strncmp(instruction, "STOP", 4) == 0){
    send_stop();

  }else if(strncmp(instruction, "FRIENDS ", 8) == 0){
    instruction += 8;
    send_friends(instruction);

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

void handle_server_messages(){
  if(mq_receive(client_qd, (char*) &msg, sizeof(msg), NULL) < 0) {
    return;
  }

  switch(msg.req_type){
  case ECHO_REQ:
    printf("%s\n", msg.arg1);
    break;

  case LIST_REQ:
    printf("%s\n", msg.arg1);
    printf("%s\n", msg.arg2);
    break;

  case STOP_REQ:
    handle_server_shutdown();
    break;

  default:
    break;
  }
}


void send_init(){
  msg.req_type = INIT_REQ;
  strcpy(msg.arg1, q_name);
  send_request(WAIT);
  printf("Client ID in chatroom: %d\n", msg.num1);
  client_id = msg.num1;
}

void send_stop(){
  msg.req_type = STOP_REQ;
  send_request(NOWAIT);
}

void send_echo(const char *text){
  msg.req_type = ECHO_REQ;
  msg.num1 = client_id;
  strcpy(msg.arg1, text);
  send_request(NOWAIT);
}

void send_list(){
  msg.req_type = LIST_REQ;
  msg.num1 = client_id;
  send_request(NOWAIT);
}

void send_friends(const char *text){
  msg.req_type = FRIENDS_REQ;
  msg.num1 = client_id;
  strcpy(msg.arg1, text);
  send_request(NOWAIT);
}

void send_to_all(const char *text){
  msg.req_type = TOALL_REQ;
  msg.num1 = client_id;
  strcpy(msg.arg1, text);
  send_request(NOWAIT);
}

void send_to_friends(const char *text){
  msg.req_type = TOFRIENDS_REQ;
  msg.num1 = client_id;
  strcpy(msg.arg1, text);
  send_request(NOWAIT);
}

void send_to_one(int target_id, const char *text){
  msg.req_type = TOONE_REQ;
  msg.num1 = client_id;
  msg.num2 = target_id;
  strcpy(msg.arg1, text);
  send_request(NOWAIT);
}


/*
 *  Sends request and waits for response.
 */
void send_request(int waitflag){
  if(mq_send(server_qd, (char*) &msg, sizeof(msg), msg.req_type) < 0){
    printf("Error sending request. %s.\n", strerror(errno));
    return;
  }

  if(waitflag == WAIT){
    if(mq_receive(client_qd, (char*) &msg, sizeof(msg), NULL) < 0) {
      printf("Error while receiving message: %s.\n", strerror(errno));
      return;
    }
  }
}


/*
 * CTRL+C handler
 */
void handle_exit(){
  send_stop();
}

/*
 * Executes when server notifies abut stopping.
 */
void handle_server_shutdown(){
  mq_close(server_qd);
  mq_close(client_qd);
  mq_unlink(q_name);
  exit(1);
}


/*
 * Generates process-uniqe queue name.
 */
char* gen_queue_name(){
  char *name = calloc(32, sizeof(char));
  strcat(name, "/");
  sprintf(name+1, "%d", getpid());
  return name;
}

