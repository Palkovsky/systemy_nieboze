#define _GNU_SOURCE

#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include "chat.h"

#define MAX_ROOM_SIZE 32

void print_req();
void handle_exit();
void timestamp_message(int, char*);

// Message responses
void respond_init();
void respond_stop();
void respond_echo();
void respond_to_all();
void respond_to_friends();
void respond_to_one();
void respond_list();
void respond_friends();

ReqMsg req;                 // Buffer for incoming requests.
int q_id;                   // Qid of server message queue.
int clients[MAX_ROOM_SIZE]; // Array containing connected clients qids.
int friends[MAX_ROOM_SIZE]; // Friend list

int main(int argc, char **argv){
  memset(clients, 0, sizeof(clients));
  for(int i=0; i<MAX_ROOM_SIZE; i++) friends[i] = -1;

  // Acquire queue key and id
  key_t key = ftok(SERVER_KEY_PATH, SERVER_KEY_SEED);
  q_id = msgget(key, IPC_CREAT | QUEUE_PERMISSIONS);
  if(q_id < 0){
    printf("%s\n", strerror(errno));
    exit(q_id);
  }

  // Register signal handler for CTRL+C
  signal(SIGINT, handle_exit);

  printf("========== Chat server ===========\n");

  // Wait for messages
  printf("Queue ID: %d | Listening...\n", q_id);
  while(1){

    if(msgrcv (q_id, &req, sizeof(ReqMsg) - sizeof(long), BY_PRIORITY, 0) == -1) {
      printf("SERVER: Error while receiving message: %s.\n", strerror(errno));
      exit(1);
    }

    print_req();

    switch(req.req_type){
    case INIT_REQ:
      respond_init();
      break;
    case STOP_REQ:
      respond_stop();
      break;
    case ECHO_REQ:
      respond_echo();
      break;
    case TOALL_REQ:
      respond_to_all();
      break;
    case TOFRIENDS_REQ:
      respond_to_friends();
      break;
    case TOONE_REQ:
      respond_to_one();
      break;
    case LIST_REQ:
      respond_list();
      break;
    case FRIENDS_REQ:
      respond_friends();
      break;
    default:
      printf("Bad request.\n");
      break;
    }
  }

  return 0;
}

/*
 * Finds place in chat room and responds with client_id
 */
void respond_init(){
  int i = 0;
  while(i < MAX_ROOM_SIZE && clients[i] != 0) i++;

  // Case when chatroom is full
  if(i == MAX_ROOM_SIZE) return;

  // Place client in chatroom and put his new id in response
  int client_qid = req.num1;
  clients[i] = client_qid;
  req.num1 = i;

  if(msgsnd(client_qid, &req, sizeof(ReqMsg) - sizeof(long), 0) == -1){
    printf("Error sending init response to %d. %s.\n", client_qid, strerror(errno));
  }
}

/*
 * Disconects client from server.
 */
void respond_stop(){
  int client_qid = clients[req.num1];
  clients[req.num1] = 0;
  req.num1 = -1;

  if(msgsnd(client_qid, &req, sizeof(ReqMsg) - sizeof(long), 0) == -1){
    printf("Error sending stop response to %d. %s.\n", client_qid, strerror(errno));
  }
}

/*
 * Sends unmodified response to client.
 */
void respond_echo(){
  int client_qid = clients[req.num1];
  req.type = req.req_type = ECHO_REQ;
  timestamp_message(req.num1, req.arg1);

  if(msgsnd(client_qid, &req, sizeof(ReqMsg) - sizeof(long), 0) == -1){
    printf("Error sending echo response to %d. %s.\n", client_qid, strerror(errno));
  }
}

/*
 * Sends message to everyone in chatroom.
 */
void respond_to_all(){
  req.type = req.req_type = ECHO_REQ;
  timestamp_message(req.num1, req.arg1);

  for(int i = 0; i < MAX_ROOM_SIZE; i++){
    int qid = clients[i];
    if(qid == 0) continue;

    if(msgsnd(qid, &req, sizeof(ReqMsg) - sizeof(long), 0) == -1){
      printf("Error sending echo response to %d. %s.\n", qid, strerror(errno));
    }
  }
}

/*
 * Sends message only to ids from friends list.
 */
void respond_to_friends(){
  req.type = req.req_type = ECHO_REQ;
  timestamp_message(req.num1, req.arg1);

  for(int i=0; i < MAX_ROOM_SIZE && friends[i] != -1; i++){
    int qid = clients[friends[i]];
    if(qid == 0) continue;

    if(msgsnd(qid, &req, sizeof(ReqMsg) - sizeof(long), 0) == -1){
      printf("Error sending echo response to %d. %s.\n", qid, strerror(errno));
    }
  }
}

/*
 * Sends message to specific id specified in num2.
 */
void respond_to_one(){
  req.type = req.req_type = ECHO_REQ;
  timestamp_message(req.num1, req.arg1);

  int qid = clients[req.num2];
  if(qid == 0) return;

  if(msgsnd(qid, &req, sizeof(ReqMsg) - sizeof(long), 0) == -1){
    printf("Error sending echo response to %d. %s.\n", qid, strerror(errno));
  }
}

/*
 * Sends ids of connected clients.
 */
void respond_list(){
  int client_qid = clients[req.num1];

  memset(req.arg1, 0, sizeof(req.arg1));
  strcat(req.arg1, "Active Users:\n");

  for(int i=0; i < MAX_ROOM_SIZE; i++){
    if(clients[i] != 0){
      char str[32];
      sprintf(str, "%d", i);
      strcat(req.arg1, str);
    }

    if(i == req.num1) strcat(req.arg1, "(You)");
    if(clients[i] != 0) strcat(req.arg1, "\n");
  }

  memset(req.arg2, 0, sizeof(req.arg2));
  strcat(req.arg2, "Current friends:\n");

  for(int i=0; i < MAX_ROOM_SIZE && friends[i] != -1; i++){
    char str[32];
    sprintf(str, "%d", friends[i]);
    strcat(req.arg2, str);
    strcat(req.arg2, "\n");
  }

  if(msgsnd(client_qid, &req, sizeof(ReqMsg) - sizeof(long), 0) == -1){
    printf("Error sending list response to %d. %s.\n", client_qid, strerror(errno));
  }
}

/*
 * Allows setting friends array.
 */
void respond_friends(){
  int client_qid = clients[req.num1];
  for(int i=0; i<MAX_ROOM_SIZE; i++) friends[i] = -1;

  int i = 0;
  char *token = strtok(req.arg1, " ");
  while(token != NULL && i < MAX_ROOM_SIZE){
    friends[i++] = atoi(token);
    token = strtok(NULL, " ");
  }

  if(msgsnd(client_qid, &req, sizeof(ReqMsg) - sizeof(long), 0) == -1){
    printf("Error sending friends response to %d. %s.\n", client_qid, strerror(errno));
  }
}

/*
 * Prints request struct.
 */
void print_req(){
  printf("MSG TYPE: %d\n", req.req_type);
  printf("NUM1: %d\n", req.num1);
  printf("NUM2: %d\n", req.num2);
  printf("ARG1: %s\n", req.arg1);
  printf("ARG2: %s\n", req.arg2);
  printf("==============\n");
}

/*
 * Adds timestamp to given string with sender id.
 */
void timestamp_message(int uid, char *text){
  char temp[MAX_MSG_SIZE];
  strcpy(temp, text);

  time_t raw_time;
  time(&raw_time);

  char timestamp[50];
  strftime(timestamp, 50, "%Y %m %d %H:%M:%S", localtime(&raw_time));

  sprintf(text, "[%s] | %d: %s", timestamp, uid, temp);
}

/*
 * Handles SIGINT
 */
void handle_exit(){
  // Sends shoutdown request to all active clients.
  req.type = req.req_type = STOP_REQ;

  for(int i=0; i < MAX_ROOM_SIZE; i++){
    if(clients[i] == 0) continue;
    if(msgsnd(clients[i], &req, sizeof(ReqMsg) - sizeof(long), 0) == -1){
      printf("Error sending stop request to %d. %s.\n", clients[i], strerror(errno));
    }
  }

  msgctl(q_id, IPC_RMID, NULL);
  exit(0);
}
