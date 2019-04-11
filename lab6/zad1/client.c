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

void prompt_user(char *buff);
void handle_exit();
void handle_server_messages();
void handle_server_shutdown();
void send_request();

// Requests
void send_init();
void send_stop();
void send_echo(const char *message);
void send_list();
void send_friends(const char *friends);
void send_to_all(const char *message);
void send_to_friends(const char *message);
void send_to_one(int target_id, const char *message);

int server_qid; // Server queue identifier
int client_qid; // Client queue identifier
int client_id;  // Given by server. Required for communication.

ReqMsg msg;
char input_buff[MAX_MSG_SIZE];

int main(int argc, char **argv){
  signal(SIGINT, handle_exit);

  // Try connect to server queue for client -> server communication.
  key_t server_key = ftok(SERVER_KEY_PATH, SERVER_KEY_SEED);
  if((server_qid = msgget(server_key, QUEUE_PERMISSIONS)) == -1){
    printf("Error while conecting to server: %s\n", strerror(errno));
    exit(1);
  }
  printf("Server Queue ID: %d\n", server_qid);


  // Create private message queue for server -> client communication.
  if((client_qid =  msgget(IPC_PRIVATE, QUEUE_PERMISSIONS)) == -1){
    printf("Error while creating client queue: %s\n", strerror(errno));
    exit(1);
  }

  send_init();
  send_list();
  send_to_all("no elo cipeczko co masz na sobe");

  printf("========== Chat client ===========\n");
  while(1){

    // Handling server messages
    handle_server_messages();

    //prompt_user(input_buff);
    //printf("%s\n", input_buff);
  }

  return 0;
}

void handle_server_messages(){
  if(msgrcv(client_qid, &msg, sizeof(ReqMsg) - sizeof(long), 0, 0) == -1) {
    return;
  }

  switch(msg.req_type){
  case ECHO_REQ:
    printf("%s\n", msg.arg1);
    break;

  case STOP_REQ:
    handle_server_shutdown();
    break;

  default:
    break;
  }

  msg.req_type = -1;
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
  send_request(WAIT);
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
  send_request(WAIT);
  printf("%s\n", msg.arg1);
  printf("%s\n", msg.arg2);
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

  if(waitflag == 0){
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
 * Prompts user for command
 */
void prompt_user(char *buff){
  memset(buff, 0, MAX_MSG_SIZE);
  printf("Command: ");
  scanf("%[^\n]%*c", buff);
}

/*
 * CTRL+C handler
 */
void handle_exit(){
  send_stop();
  msgctl(client_qid, IPC_RMID, NULL);
  exit(1);
}

void handle_server_shutdown(){
  msgctl(client_qid, IPC_RMID, NULL);
  exit(1); 
}
