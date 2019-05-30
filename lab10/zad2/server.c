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
int register_client(int, const char*);
void print_usage(const char*);

/* * INPUT thread
 * FLAGS
 */
static volatile int RUNNING = 1;

/*
 * GLOBALS
 */
static unsigned short af_inet_port;
static char           af_unix_path[UNIX_PATH_MAX];
static int            inet_fd;
static int            unix_fd;

static int       client_fds[MAX_CLIENTS] = {0};  // Holds FDs of connected sockets.
static int       client_busy[MAX_CLIENTS] = {0}; // Tells if FD is busy.
static void*     client_addr[MAX_CLIENTS];
static char      client_hostname[MAX_CLIENTS][1 << 6]; // Hostname of FD.

static int requests_sent = 0;

int main(int argc, char **argv)
{
  pthread_t *listener_tid, *input_tid;

  if(argc != 3)
    { print_usage(argv[0]); exit(1); }

  af_inet_port = strtoul(argv[1], NULL, 10);
  strcpy(af_unix_path, argv[2]);
  signal(SIGINT, exit_handler);

  listener_tid = spawn_thread(listener_thread,
                              "Unable to spawn socket listener thread.");
  input_tid = spawn_thread(input_thread,
                           "Unable to spawn input thread.");

  join_thread(listener_tid, "Unable to join listener thread.");
  join_thread(input_tid, "Unable to join input thread.");

  return 0;
}

/*
 * Reads messages from sockets.
 */
void handle_event(epoll_event *event)
{
  message msg;
  size_t bytes_read;
  int fd;
  sockaddr *addr;
  socklen_t len;
  int i;

  fd = event->data.fd;
  addr = malloc(sizeof(sockaddr));
  len = sizeof(sockaddr);
  bytes_read = recvfrom(fd, &msg, sizeof(msg), 0, addr, &len);

  if(bytes_read == sizeof(message))
  {
    switch(msg.type)
    {
    case MSG_REGISTER:
      i = register_client(fd, msg.buff);
      client_addr[i] = malloc(sizeof(sockaddr));
      memcpy(client_addr[i], addr, sizeof(sockaddr));

      printf("======= WELCOME %s =======\n", msg.buff);
      printf("------------------------\n");
      break;
    case MSG_QUIT:
      for(int i=0; i<MAX_CLIENTS; i++)
      {
        if(strcmp(client_hostname[i], msg.buff) == 0)
        {
          printf("======= RIP %s =======\n", client_hostname[i]);
          printf("------------------------\n");
          client_fds[i] = client_busy[i] = 0;
          free(client_addr[i]);
          client_addr[i] = NULL;
          memset(client_hostname[i], 0, sizeof(client_hostname[i]));
          break;
        }
      }
      break;
    case MSG_RESPONSE:
      printf("======== %ld RESPONSE ========\n", msg.num);
      printf("WORD COUNT: %ld\n", msg.num_sec);
      printf("------------------------\n");

      for(int i=0; i < MAX_CLIENTS; i++)
      {
        if(strcmp(client_hostname[i], msg.buff) == 0)
          { client_busy[i]--; break; }
      }
      break;
    default:
      break;
    }
  }

  free(addr);
}

/*
 * Connection listener.
 * this thing got too big
 */
void *listener_thread(void *arg)
{
  sockaddr_in inet_addr;
  sockaddr_un unix_addr;
  int flags;
  int epoll_fd, event_count;
  epoll_event events[MAX_EPOLL_EVENTS];

  // AF_INET UDP socket creation
  inet_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
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

  //AF_UNIX socket creation
  unix_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
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

  // Create EPOLL FD
  if((epoll_fd = epoll_create1(0)) == -1)
  {
    printf("Failed to create epoll file descriptor.\n");
    exit(2);
  }

  add_epoll(epoll_fd, inet_fd);
  add_epoll(epoll_fd, unix_fd);

  /*
   * MAIN SERVER LOOP
   */
  while(RUNNING)
  {
    // Monitor epoll events.
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

/*
 * INPUT thread
 */
void *input_thread(void *arg)
{
  message msg;
  char path[1 << 8];
  int fp;

  memset(&msg, 0, sizeof(message));
  msg.type = MSG_REQUEST;

  while(RUNNING)
  {
    scanf("%s", path);

    fp = open(path, O_RDONLY);
    if(fp < 0)
      { printf("Unable to open: %s\n", path); continue; }

    if(read(fp, msg.buff, sizeof(msg.buff)) < 0)
      { printf("Error while reading %s\n", path); continue; }

    msg.num = requests_sent++;

    int cli_idx, cli_fd;
    cli_idx = -1;

    for(int i=0; i<MAX_CLIENTS; i++)
    {
      if(client_fds[i] == 0)
        { continue; }

      cli_idx = i;
      if(!client_busy[i])
        { break; }
    }

    cli_fd = client_fds[cli_idx];
    if(cli_idx == -1 || cli_fd == 0)
      { printf("No availablie clients\n"); continue; }

    if(sendto(cli_fd, &msg, sizeof(msg), 0, (sockaddr*) client_addr[cli_idx], sizeof(sockaddr)) < 0)
      { printf("Error sending request to FD %s: %s\n", client_hostname[cli_idx], strerror(errno)); }
    else
    {
      client_busy[cli_idx]++;
      printf("======= REQUEST %ld =========\n", msg.num);
      printf("WORKER: %d\n", cli_fd);
      printf("-----------------------------\n");
    }
  }

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
  event.events = EPOLLIN;
  event.data.fd = fd;
  if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event) < 0)
  {
    printf("Failed to add file descriptor to epoll: %s\n", strerror(errno));
    return -1;
  }
  return 0;
}

int register_client(int fd,  const char *hostname)
{
  int i = 0;
  while(i < MAX_CLIENTS)
  {
    if(client_fds[i] == 0)
    {
      client_fds[i] = fd;
      client_busy[i] = 0;
      strcpy(client_hostname[i], hostname);
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
