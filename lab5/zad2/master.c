#define _GNU_SOURCE

#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>

#define BUFF_SIZE 128

void print_usage_and_exit(){
  printf("Invalid parameters! USAGE: master.out [fifo_path]\n");
  exit(1);
}

int main(int argc, char **argv){
  if (argc != 2) print_usage_and_exit();

  // Will try to open fifo
  // If path is wrong it'll try to create one and open it.
  char *fifo_path = argv[1];
  int fifo = open(fifo_path, O_RDWR);
  if(fifo < 0){
    printf("Given FIFO stream doesn't exist. Creating one...\n");
    if(mkfifo(fifo_path, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH) == -1){
      printf("Error: %s, while creating '%s'.\n", strerror(errno), fifo_path);
      exit(1);
    }
    printf("Created.\n");
    fifo = open(fifo_path, O_RDWR);
  }

  // Infinite loop - reading BUFF_SIZE bytes from FIFO.
  char buff[BUFF_SIZE];
  while(1){
   int bytes = read(fifo, buff, BUFF_SIZE);
   if(bytes > 0){
     printf("%s", buff);
     memset(buff, 0, BUFF_SIZE);
   }
  }

  return 0;
}
