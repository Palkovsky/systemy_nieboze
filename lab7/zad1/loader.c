#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/shm.h>
#include <sys/ipc.h>

#include "data.h"

#define MAX_N 100

// Helpers
void print_usage(char*);

// SHM
int shm_get(void);
prod_line *shm_map(int);
void shm_unmap(prod_line*);

int main(int argc, char **argv)
{
  if(argc != 2)
  {
    printf("ERR: Invalid number of arguments.\n");
    print_usage(argv[0]);
    exit(1);
  }

  long N = strtol(argv[1], NULL, 10);
  if(N <= 0 || N > MAX_N)
  {
    printf("N must be in range from 1 to %d.\n", MAX_N);
    exit(1);
  }

  int shm_id = shm_get();
  prod_line *line = shm_map(shm_id);

  for(;;)
  {
    printf("WEIGHT: %ld\n", line_weight(line));
    sleep(2);
  }

  shm_unmap(line);

  return 0;
}


/*
 * Shared memory manipulation functions.
 */
int shm_get()
{
  key_t shm_key = ftok(FTOK_SHM_PATH, FTOK_SHM_SEED);
  int shm_id = shmget(shm_key, sizeof(prod_line), 0);
  if(shm_id == -1)
  {
    printf("Unable to access shared memory segment: %s\n", strerror(errno));
    exit(1);
  }
  return shm_id;
}


prod_line *shm_map(int shm_id)
{
  void *ptr = shmat(shm_id, NULL, 0);
  if(ptr == ((void*) -1))
  {
      printf("Error while mapping to shared memory segment: %s\n", strerror(errno));
      exit(errno);
  }
  return (prod_line*) ptr;
}

void shm_unmap(prod_line *ptr)
{
  if(shmdt(ptr) == -1)
    {
      printf("Error while detaching shared memory segment: %s\n", strerror(errno));
      exit(errno);
    }
}

/*
 * Helpers
 */
void print_usage(char *progname)
{
  printf("Usage: %s <N>\n \
         \rN - number of packages. From 1 to %d.\n", progname, MAX_N);
}
