#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

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
