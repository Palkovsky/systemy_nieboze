#define _GNU_SOURCE

#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include "chat.h"

void print_req(ReqMsg *msg);
void handle_exit();

key_t key;
int q_id;

int main(int argc, char **argv){
  printf("========== Chat server ===========\n");

  // Acquire queue
  key = ftok(SERVER_KEY_PATH, SERVER_KEY_SEED);
  printf("Server key: %d\n", key);
  q_id = msgget(key, IPC_CREAT | QUEUE_PERMISSIONS);
  if(q_id < 0){
    printf("%s\n", strerror(errno));
    exit(q_id);
  }

  // Register signal handler for CTRL+C
  signal(SIGINT, handle_exit);

  ReqMsg msg_buff;

  // Wait for messages
  printf("Queue key: %d | Listening...\n", q_id);
  while(1){

    if(msgrcv (q_id, &msg_buff, sizeof(ReqMsg) - sizeof(long), 0, 0) == -1) {
      printf("SERVER: Error while receiving message: %s.\n", strerror(errno));
      exit(1);
    }

    print_req(&msg_buff);
  }

  handle_exit();
  return 0;
}

void print_req(ReqMsg *msg){
  if(msg == NULL) return;
  printf("MSG TYPE: %d\n", msg->req_type);
  printf("ARG1: %s\n", msg->arg1);
  printf("ARG2: %s\n", msg->arg2);
  printf("\n");
}

void handle_exit(){
  msgctl(q_id, IPC_RMID, NULL);
  exit(0);
}
