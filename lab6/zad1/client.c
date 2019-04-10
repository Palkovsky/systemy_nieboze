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


void prompt_user(char *buff);
void handle_exit();
void send_request(ReqMsg* buff);

void send_init(ReqMsg* buff, int client_id){
  char id_string[32];
  sprintf(id_string, "%d", client_id);

  buff->type = buff->req_type = INIT_REQ;
  strcpy(buff->arg1, id_string);

  send_request(buff);
}

key_t server_key;
int server_id;
int client_id;

int main(int argc, char **argv){
  signal(SIGINT, handle_exit);

  server_key = ftok(SERVER_KEY_PATH, SERVER_KEY_SEED);
  printf("Server key: %d\n", server_key);
  if((server_id = msgget(server_key, QUEUE_PERMISSIONS)) == -1){
    printf("Error while conecting to server: %s\n", strerror(errno));
    exit(1);
  }

  if((client_id =  msgget(IPC_PRIVATE, QUEUE_PERMISSIONS)) == -1){
    printf("Error while creating client queue: %s\n", strerror(errno));
    exit(1);
  }

  char input_buff[MAX_MSG_SIZE];
  ReqMsg msg_buff;
  send_init(&msg_buff, client_id);

  printf("========== Chat client ===========\n");
  while(1){
    prompt_user(input_buff);
    printf("%s\n", input_buff);
  }

  return 0;
}

void send_request(ReqMsg *buff){
  if(msgsnd(server_id, buff, sizeof(ReqMsg) - sizeof(long), 0) == -1){
    printf("Error sending request. %s.\n", strerror(errno));
    exit(1);
  }
}

void prompt_user(char *buff){
  memset(buff, 0, MAX_MSG_SIZE);
  printf("Command: ");
  scanf("%[^\n]%*c", buff);
}

void handle_exit(){
  msgctl(client_id, IPC_RMID, NULL);
  exit(0);
}
