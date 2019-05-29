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

typedef struct message {

} messsage;

pthread_t *spawn_thread(void* (*)(void*), const char*);
void       join_thread(pthread_t*, const char*);

#endif /* UTILS_H */
