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
#include <sys/ipc.h>
#include <sys/msg.h>

#include "chat.h"

#define WAIT 0
#define NOWAIT 1

void read_instructions(FILE*);
void exec_instruction(char*);
void handle_exit();
void handle_server_messages();
void handle_server_shutdown();
void send_request();

// Requests
void send_init();
void send_stop();
void send_echo(const char*);
void send_list();
void send_friends(const char*);
void send_to_all(const char*);
void send_to_friends(const char*);
void send_to_one(int, const char*);

int server_qid; // Server queue identifier
int client_qid; // Client queue identifier
int client_id;  // Given by server. Required for communication.

ReqMsg msg;

int main(int argc, char **argv){
  signal(SIGINT, handle_exit);

  // Try connect to server queue for client -> server communication.
  key_t server_key = ftok(SERVER_KEY_PATH, SERVER_KEY_SEED);
  if((server_qid = msgget(server_key, QUEUE_PERMISSIONS)) == -1){
    printf("Error while conecting to server: %s\n", strerror(errno));
    exit(1);
  }

  // Create private message queue for server -> client communication.
  if((client_qid =  msgget(IPC_PRIVATE, QUEUE_PERMISSIONS)) == -1){
    printf("Error while creating client queue: %s\n", strerror(errno));
    exit(1);
  }

  printf("Server Queue ID: %d\n", server_qid);
  printf("Private Queue ID: %d\n", client_qid);

  send_init();
  read_instructions(stdin);

  printf("========== Chat client ===========\n");
  while(1){
    handle_server_messages(); // Handling server messages
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

/*
 * Handles messages from server.
 * Server can only ask for echoing certain message or stopping.
 */
void handle_server_messages(){
  if(msgrcv(client_qid, &msg, sizeof(ReqMsg) - sizeof(long), BY_PRIORITY, 0) == -1) {
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
  msg.type = msg.req_type = INIT_REQ;
  msg.num1 = client_qid;
  send_request(WAIT);
  printf("Client ID in chatroom: %d\n", msg.num1);
  client_id = msg.num1;
}

void send_stop(){
  msg.type = msg.req_type = STOP_REQ;
  msg.num1 = client_id;
  send_request(NOWAIT);
}

void send_echo(const char *text){
  msg.type = msg.req_type = ECHO_REQ;
  msg.num1 = client_id;
  strcpy(msg.arg1, text);
  send_request(NOWAIT);
}

void send_list(){
  msg.type = msg.req_type = LIST_REQ;
  msg.num1 = client_id;
  send_request(NOWAIT);
}

void send_friends(const char *text){
  msg.type = msg.req_type = FRIENDS_REQ;
  msg.num1 = client_id;
  strcpy(msg.arg1, text);
  send_request(WAIT);
  printf("Request sent! Use LIST to view current friend list.\n");
}

void send_to_all(const char *text){
  msg.type = msg.req_type = TOALL_REQ;
  msg.num1 = client_id;
  strcpy(msg.arg1, text);
  send_request(NOWAIT);
}

void send_to_friends(const char *text){
  msg.type = msg.req_type = TOFRIENDS_REQ;
  msg.num1 = client_id;
  strcpy(msg.arg1, text);
  send_request(NOWAIT);
}

void send_to_one(int target_id, const char *text){
  msg.type = msg.req_type = TOONE_REQ;
  msg.num1 = client_id;
  msg.num2 = target_id;
  strcpy(msg.arg1, text);
  send_request(NOWAIT);
}


/*
 *  Sends request and waits for response.
 */
void send_request(int waitflag){
  if(msgsnd(server_qid, &msg, sizeof(ReqMsg) - sizeof(long), 0) == -1){
    printf("Error sending request. %s.\n", strerror(errno));
    return;
  }

  if(waitflag == WAIT){
    ReqMsg res;
    if(msgrcv (client_qid, &res, sizeof(ReqMsg) - sizeof(long), 0, 0) == -1) {
      printf("Error while receiving message: %s.\n", strerror(errno));
      return;
    }

    // Collect returned information
    msg.num1 = res.num1;
    msg.num2 = res.num2;
    strcpy(msg.arg1, res.arg1);
    strcpy(msg.arg2, res.arg2);
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
  msgctl(client_qid, IPC_RMID, NULL);
  exit(1);
}
