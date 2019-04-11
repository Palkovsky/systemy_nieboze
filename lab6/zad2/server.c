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
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>

#include "chat.h"

#define MAX_ROOM_SIZE 32

void print_req();
void handle_exit();
void timestamp_message(int uid, char *string);

ReqMsg req;                 // Buffer for incoming requests.
mqd_t qd;                   // Queue descriptor
int clients[MAX_ROOM_SIZE]; // Array containing connected clients qids.
int friends[MAX_ROOM_SIZE]; // Friend list

int main(int argc, char **argv){
  memset(clients, 0, sizeof(clients));
  for(int i=0; i<MAX_ROOM_SIZE; i++) friends[i] = -1;

  // Create server queue
  qd = mq_open(SERVER_NAME, O_RDONLY | O_CREAT, Q_PERM, NULL);
  if(qd < 0){
    printf("Error while opening server: %s\n", strerror(errno));
    exit(1);
  }

  // Register signal handler for CTRL+C
  signal(SIGINT, handle_exit);

  printf("========== Chat server ===========\n");

  // Wait for messages
  printf("Queue ID: %d | Listening...\n", qd);
  while(1){

  }

  return 0;
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

  // should happen hir

  mq_close(qd);
  mq_unlink(SERVER_NAME);
  exit(0);
}
