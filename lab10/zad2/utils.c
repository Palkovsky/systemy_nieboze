#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "utils.h"

pthread_t *spawn_thread(void *(*thread_main)(void*), const char *err)
{
  pthread_t *tid = malloc(sizeof(pthread_t));
  if(pthread_create(tid, NULL, thread_main, NULL) != 0)
  {
    printf("%s\n", err);
    exit(1);
  }
  return tid;
}

void join_thread(pthread_t *tid, const char *err)
{
  void *ret;
  if(pthread_join(*tid, &ret) != 0)
  {
    printf("%s\n", err);
    exit(1);
  }
  free(tid);
}

void print_msg(message msg)
{
  char typestr[1<<8];
  switch(msg.type)
  {
  case MSG_REGISTER:
    strcpy(typestr, "REGISTER");
    break;
  case MSG_REQUEST:
    strcpy(typestr, "REQUEST");
    break;
  case MSG_RESPONSE:
    strcpy(typestr, "RESPONSE");
    break;
  case MSG_QUIT:
    strcpy(typestr, "QUIT");
    break;
  }
  printf("MSG TYPE: %s\n", typestr);
  printf("MSG BUFF: %s\n", msg.buff);
  printf("MSG NUM: %ld\n", msg.num);
  printf("MSG NUM SECONDARY: %ld\n", msg.num_sec);
}
