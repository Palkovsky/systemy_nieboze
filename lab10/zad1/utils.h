#ifndef UTILS_H
#define UTILS_H

#include <netinet/in.h>
#include <sys/un.h>
#include <sys/epoll.h>
#include <pthread.h>

typedef struct sockaddr sockaddr;
typedef struct sockaddr_in sockaddr_in;
typedef struct sockaddr_un sockaddr_un;
typedef struct epoll_event epoll_event;

typedef enum {
   /* When client registers in server */
   MSG_REGISTER,
   /* When server asks client for doing job */
   MSG_REQUEST,
   /* When client ends computation and sends response to server.*/
   MSG_RESPONSE,
   /* When server checks if client still responding */
   MSG_PING
} msg_type;

typedef struct message {
  msg_type type;
  /* Fields used by some certain types to pass extra data. */
  char buff[4096];
  long num;
  long num_sec;
} message;

pthread_t *spawn_thread(void* (*)(void*), const char*);
void join_thread(pthread_t*, const char*);
void print_msg(message);

#endif /* UTILS_H */
