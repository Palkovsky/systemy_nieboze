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
  printf("Invalid parameters! USAGE: slave.out [fifo_path] [samples]\n");
  exit(1);
}

int file_exist (char *filename){
  struct stat buffer;
  return (stat (filename, &buffer) == 0);
}

int main(int argc, char **argv){
  if (argc != 3) print_usage_and_exit();

  char *fifo_path = argv[1];
  long samples_no = strtol(argv[2], NULL, 10);

  if(samples_no <= 0){
    printf("Samples number must be greater than 0.\n");
    exit(1);
  }

  int fifo;
  if(!file_exist(fifo_path)){
    printf("Unable to open fifo. If you want to create one use master first.\n");
    exit(1);
  }else
    fifo = open(fifo_path, O_RDWR);

  printf("========== SLAVE | PID %d ==========\n", getpid());
  srand(time(NULL));

  char date_buff[64] = {0};
  char out_buff[BUFF_SIZE] = {0};

  while(samples_no--){
    FILE *date_pipe = popen("date", "r");
    fread(date_buff, sizeof(char), 64, date_pipe);
    pclose(date_pipe);

    sprintf(out_buff, "PID: %d | DATE: %s", getpid(), date_buff);

    printf("%s", out_buff);
    write(fifo, out_buff, BUFF_SIZE);

    int sleep_amount  = (rand() % 4) + 2;
    printf("Sleeping for %d seconds...\n", sleep_amount);
    sleep(sleep_amount);
  }

  close(fifo);

  return 0;
}
