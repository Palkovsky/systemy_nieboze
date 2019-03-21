#define _GNU_SOURCE

#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>

void print_usage_and_exit(){
  printf("Invalid parameters! USAGE:\n");
  printf("tester [file] [pmin] [pmax] [bytes]\n");
  exit(1);
}


int main(int argc, char **argv){
  if (argc != 5) print_usage_and_exit();

  FILE *file = fopen(argv[1], "a");
  if(!file){
    printf("Unable to open passed file!\n");
    return 2;
  }

  int32_t pmin, pmax, bytes;
  pmin  = strtol(argv[2], NULL, 10);
  pmax  = strtol(argv[3], NULL, 10);
  bytes = strtol(argv[4], NULL, 10);

  if(pmin <= 0 || pmax <= 0) {
    printf("pmin and pmax must be positive!\n");
    return 1;
  }
  if(pmin >= pmax){
    printf("pmax must be greater than pmin\n");
    return 1;
  }
  if(bytes <= 0){
    printf("bytes number must be positive!\n");
    return 1;
  }

  while(1){
    int32_t num = (rand() % (pmax - pmin + 1)) + pmin;
    printf("%d seconds to next write\n", num);
    sleep(num);

    char *time_buff = calloc(50, sizeof(char));
    time_t timestamp = time(NULL);
    strftime(time_buff, 50, "%Y-%m-%d_%H-%M-%S", localtime(&timestamp));

    int i;
    char *random_buff = calloc(bytes + 1, sizeof(char));
    for(i=0; i<bytes; i++){
      random_buff[i] = rand() % 256;
    }
    random_buff[i] = 0;

    char *buff = calloc(bytes + 200, sizeof(char));
    sprintf(buff, "\nPID: %d; Seconds: %d; Time: %s; Bytes: %s", getpid(), num, time_buff, random_buff);
    fwrite(buff, sizeof(char), strlen(buff), file);
    fflush(file);
    printf("%s\n", buff);

    free(buff);
    free(time_buff);
    free(random_buff);
  }

  fclose(file);
  return 0;
}
