#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <signal.h>


#include "utils.h"

/*
 * DEFINES
 */
#define UNIX_PATH_MAX    108
#define MAX_CLIENTS      16
#define MAX_EPOLL_EVENTS 128

/*
 * MAIN LOGIC
 */
void *listener_thread(void *);
void *heartbeat_thread(void *);
void *input_thread(void *);
void handle_event(epoll_event*);

/*
 * HELPERS
 */
void exit_handler(int);
int add_epoll(int, int);
int add_client_fd(int);
void print_usage(const char*);

/*
 * FLAGS
 */
static volatile int RUNNING = 1;

/*
 * GLOBALS
 */
unsigned short af_inet_port;
char           af_unix_path[UNIX_PATH_MAX];
int            client_fds[MAX_CLIENTS] = {0}; // Holds FDs of connected sockets.


int main(int argc, char **argv)
{
  if(argc != 3)
    { print_usage(argv[0]); exit(1); }

  af_inet_port = strtoul(argv[1], NULL, 10);
  strcpy(af_unix_path, argv[2]);

  signal(SIGINT, exit_handler);

  pthread_t *listener_tid, *input_tid, *heartbeat_tid;

  listener_tid = spawn_thread(listener_thread,
                              "Unable to spawn socket listener thread.");
  input_tid = spawn_thread(input_thread,
                           "Unable to spawn input thread.");
  heartbeat_tid = spawn_thread(heartbeat_thread,
                               "Unable to spawn heartbeat thread.");

  join_thread(listener_tid, "Unable to join listener thread.");
  join_thread(input_tid, "Unable to join input thread.");
  join_thread(heartbeat_tid, "Unable to join heartbeat thread.");

  return 0;
}


/*
 * Reads requests from sockets and responds.
 */
void handle_event(epoll_event *event)
{
  size_t bytes_read;
  char buff[1 << 8];

  if((event->events & EPOLLRDHUP) == EPOLLRDHUP)
  {
    printf("FD %d disconected.\n", event->data.fd);

    return;
  }

  if((event->events & EPOLLOUT) == EPOLLOUT)
  {
    bytes_read = read(event->data.fd, buff, sizeof(buff));
    if(bytes_read > 0)
    {
      printf("Reading file descriptor '%d' -- ", event->data.fd);
      printf("%zd bytes read.\n", bytes_read);
      printf("Read '%s'\n", buff);
    }
  }
}

/*
 * Connection listener.
 * this thing got too bing
 */
void *listener_thread(void *arg)
{
  int inet_fd, unix_fd, cli_fd;
  sockaddr_in inet_addr, inet_cli;
  sockaddr_un unix_addr, unix_cli;
  socklen_t addr_len;
  int flags;

  int epoll_fd, event_count;
  epoll_event events[MAX_EPOLL_EVENTS];

  // AF_INET TCP socket creation
  inet_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (inet_fd == -1)
  {
    printf("AF_INET: Socket creation failed: %s\n", strerror(errno));
    exit(2);
  }
  else
    { printf("AF_INET: Socket successfully created!\n"); }

  // AF_INET - make socket non-blocking
  flags = fcntl(inet_fd, F_GETFL);
  fcntl(inet_fd, F_SETFL, flags | O_NONBLOCK);

  memset(&inet_addr, 0, sizeof(inet_addr));
  inet_addr.sin_family = AF_INET;
  inet_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  inet_addr.sin_port = htons(af_inet_port);

  // AF_INET binding
  if ((bind(inet_fd, (sockaddr*) &inet_addr, sizeof(inet_addr))) != 0)
  {
    printf("AF_INET: Socket bind failed: %s\n", strerror(errno));
    exit(2);
  }
  else
    { printf("AF_INET: Socket successfully binded!\n"); }

  // AF_INET listen
  if (listen(inet_fd, MAX_CLIENTS) != 0)
  {
    printf("AF_INET: Listening failed: %s\n", strerror(errno));
    exit(1);
  }
  else
    { printf("AF_INET: Listening on port :%d...\n", af_inet_port); }

  //AF_UNIX socket creation
  unix_fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (unix_fd == -1)
  {
      printf("AF_UNIX: Socket creation failed: %s\n", strerror(errno));
      exit(2);
  }
  else
    { printf("AF_UNIX: Socket successfully created!\n"); }

  // AF_UNIX - make socket non-blocking
  flags = fcntl(unix_fd, F_GETFL);
  fcntl(unix_fd, F_SETFL, flags | O_NONBLOCK);

  memset(&unix_addr, 0, sizeof(unix_addr));
  unix_addr.sun_family = AF_UNIX;
  strcpy(unix_addr.sun_path, af_unix_path);

  // AF_UNIX binding
  if ((bind(unix_fd, (sockaddr*) &unix_addr, sizeof(unix_addr))) != 0)
  {
    printf("AF_UNIX: Socket bind failed: %s\n", strerror(errno));
    exit(2);
  }
  else
    { printf("AF_UNIX: Socket successfully binded!\n"); }

  // AF_UNIX listen
  if (listen(unix_fd, MAX_CLIENTS) != 0)
  {
    printf("AF_UNIX: Listening failed: %s\n", strerror(errno));
    exit(1);
  }
  else
    { printf("AF_UNIX: Listening on %s...\n", af_unix_path); }

  // Create EPOLL FD
  if((epoll_fd = epoll_create1(0)) == -1)
  {
    printf("Failed to create epoll file descriptor.\n");
    exit(2);
  }

  /*
   * MAIN SERVER LOOP
   */
  while(RUNNING)
  {
    memset(&inet_cli, 0, sizeof(sockaddr_in));
    memset(&unix_cli, 0, sizeof(sockaddr_un));

    /*
     * Check for any incoming connections on both sockets.
     */
    cli_fd = accept(inet_fd, (sockaddr*) &inet_cli, &addr_len);
    if(cli_fd >= 0)
    {
      printf("INET FD: %d\n", cli_fd);
      add_epoll(epoll_fd, cli_fd);
      add_client_fd(cli_fd);
    }

    cli_fd = accept(unix_fd, (sockaddr*) &unix_cli, &addr_len);
    if(cli_fd >= 0)
    {
      printf("UNIX FD: %d\n", cli_fd);
      add_epoll(epoll_fd, cli_fd);
      add_client_fd(cli_fd);
    }

    /*
     * Monitor epool events.
     */
    event_count = epoll_wait(epoll_fd, events, MAX_EPOLL_EVENTS, 0);
    for(int i = 0; i < event_count; i++)
      { handle_event(&events[i]); }
  }

  // Close socket before finishing
  close(inet_fd);
  close(unix_fd);
  close(epoll_fd);
  unlink(af_unix_path);

  printf("Listener stopping...\n");
  return NULL;
}

void *heartbeat_thread(void *arg)
{
  char *buff = "buff";

  printf("HEARTBEAT: Thread starting...\n");
  while(RUNNING)
  {
    for(int i=0; i<MAX_CLIENTS; i++)
    {
      if(client_fds[i] == 0)
        { continue; }

      if(send(client_fds[i], buff, strlen(buff) + 1, MSG_NOSIGNAL) < 0)
      {
        printf("HEARTBEAT: Socket %d not responding.\n", client_fds[i]);
        client_fds[i] = 0;
        continue;
      }
    }
    sleep(5);
  }
  printf("HEARTBEAT: Thread stopping...\n");
  return NULL;
}

/*
 * INPUT thread
 */
void *input_thread(void *arg)
{
  while(RUNNING)
    {}
  printf("Input thread stopping...\n");
  return NULL;
}

/*
 * Helpers
 */
void exit_handler(int sig)
{
  RUNNING = 0;
}

int add_epoll(int epoll_fd, int fd)
{
  epoll_event event;
  event.events = EPOLLIN | EPOLLOUT;
  event.data.fd = fd;

  if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event) < 0)
  {
    printf("Failed to add file descriptor to epoll: %s\n", strerror(errno));
    return -1;
  }
  return 0;
}

int add_client_fd(int fd)
{
  int i = 0;
  while(i < MAX_CLIENTS)
  {
    if(client_fds[i] == 0)
    {
      client_fds[i] = fd;
      return i;
    }
    i++;
  }
  return -1;
}

void print_usage(const char *progname)
{
  printf("Usage: %s <TCP port> <UNIX path> \n \
         \r TCP port - TCP port to run server on. Ports 0-1023 require superuser privleges.  \n \
         \r UNIX Path - AF_UNIX pathname\n", progname);
}
