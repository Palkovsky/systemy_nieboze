#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "utils.h"

#define NIX_SOCK 1
#define NET_SOCK 2

/*
 * Return FD of specific socket.
 */
int connect_unix(const char*);
int connect_inet(int);
int client_loop(int, const char*);

/*
 * Helpers
 */
int count_words(char*);
void print_usage(const char*);

int main(int argc, char **argv)
{
  char *hostname, *address;
  int sock_type;
  int sock_fd;

  { // Arg parsing
    if(argc != 4)
      { print_usage(argv[0]); exit(1); }

    hostname = argv[1];
    address  = argv[3];
    if(strcmp(argv[2], "NET") == 0)
      { sock_type = NET_SOCK; }
    else if(strcmp(argv[2], "NIX") == 0)
      { sock_type = NIX_SOCK; }
    else
      { printf("Socket type must be either NET or NIX!\n"); exit(1); }
  }

  if(sock_type & NET_SOCK)
  {
    int port = strtol(address, NULL, 10);
    if(port < 0 || port >= 1<<16)
      { printf("INET port must be in range %d-%d.", 0, 1<<16); exit(1); }
    sock_fd = connect_inet(port);
  }
  else
    { sock_fd = connect_unix(address); }

  return client_loop(sock_fd, hostname);
}

int client_loop(int fd, const char *hostname)
{
  message msg;
  int word_count;

  //Send init message
  msg.type = MSG_REGISTER;
  strcpy(msg.buff, hostname);
  send(fd, &msg, sizeof(msg), 0);

  for(;;)
  {
    if(recv(fd, &msg, sizeof(msg), MSG_DONTWAIT) < 0)
      { continue; }

    switch(msg.type)
    {
    case MSG_PING:
      printf("PINGED\n");
      break;
    case MSG_REQUEST:
      //print_msg(msg);

      printf("======= REQUEST %ld =======\n", msg.num);
      printf("---------------------------\n");

      word_count = count_words(msg.buff);
      msg.type = MSG_RESPONSE;
      msg.num_sec = word_count;

      if(send(fd, &msg, sizeof(msg), MSG_NOSIGNAL) < 0)
        { printf("Error sending request to FD %d\n", fd); }
      else
      {
        printf("======= RESPONSE %ld =======\n", msg.num);
        printf("FD: %d\n", fd);
        printf("---------------------------\n");
      }
      break;
    default:
      break;
    }

    memset(&msg, 0, sizeof(message));
  }

  close(fd);
  return 0;
}

int connect_unix(const char *sockpath)
{
  int sock_fd;
  sockaddr_un server_addr;

  sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (sock_fd == -1)
  {
    printf("UNIX: Socket creation failed: %s\n", strerror(errno));
    exit(2);
  }
  else
    { printf("UNIX: Socket successfully created!\n"); }

  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sun_family = AF_UNIX;
  strcpy(server_addr.sun_path, sockpath);

  if ((connect(sock_fd, (sockaddr*) &server_addr, sizeof(server_addr))) != 0)
  {
    printf("UNIX: Connection failed: %s\n", strerror(errno));
    exit(2);
  }
  else
    { printf("UNIX: Aquired connection!\n"); }

  return sock_fd;
}

int connect_inet(int port)
{
  int sock_fd;
  sockaddr_in server_addr;

  sock_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (sock_fd == -1)
    { printf("INET: socket creation failed.\n"); exit(1); }
  else
    { printf("INET: socket successfully created.\n"); }

  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  server_addr.sin_port = htons(port);

  if (connect(sock_fd, (sockaddr*) &server_addr, sizeof(server_addr)) != 0)
    { printf("INET: Connection with the server failed.\n"); exit(1); }
  else
    { printf("INET: Connected to the server.\n"); }

  return sock_fd;
}

/*
 * Helpers
 */
int count_words(char *text)
{
  int count;
  char *token;

  count = 0;
  token = strtok(text, " ");
  while(token != NULL)
  {
    count++;
    token = strtok(NULL, " ");
  }

  return count;
}

void print_usage(const char *progname)
{
  printf("Usage: %s <hostname> <socket_type> <address> \n \
         \r hostname    - Uniqe string identifier \n \
         \r socket type - NET for AF_INET, NIX for AF_UNIX \n \
         \r address     - TCP port for AF_INET. Path for AF_UNIX\n",
         progname);
}

