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
int connect_unix(const char*, const char*);
int connect_inet(const char*, int);

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
    sock_fd = connect_inet(hostname, port);
  }
  else
    { sock_fd = connect_unix(hostname, address); }

  char buff[1 << 8];
  scanf("%s", buff);

  printf("%zd\n", write(sock_fd, buff, sizeof(buff)));

  close(sock_fd);

  return 0;
}

int connect_unix(const char *hostname, const char *sockpath)
{
  return -1;
}

int connect_inet(const char *hostname, int port)
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

void print_usage(const char *progname)
{
  printf("Usage: %s <hostname> <socket_type> <address> \n \
         \r hostname    - Uniqe string identifier \n \
         \r socket type - NET for AF_INET, NIX for AF_UNIX \n \
         \r address     - TCP port for AF_INET. Path for AF_UNIX\n",
         progname);
}

