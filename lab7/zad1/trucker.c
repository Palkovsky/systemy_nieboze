#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/shm.h>
#include <sys/ipc.h>

#include "data.h"

// Helpers
void print_usage(char*);
void assert_positive(char*, long);

// SHM
int shm_create(void);
void *shm_map(int);
prod_line *shm_shared_line(int, int, int);
void shm_unmap(void*);
void shm_remove(int);

int main(int argc, char **argv)
{
  if(argc != 4)
  {
    printf("ERR: Invalid number of arguments.\n");
    print_usage(argv[0]);
    exit(1);
  }

  long truck_cap        = strtol(argv[1], NULL, 10);
  long line_cap         = strtol(argv[2], NULL, 10);
  long line_weight_cap  = strtol(argv[3], NULL, 10);
  assert_positive("Truck capacity", truck_cap);
  assert_positive("Line capacity", line_cap);
  assert_positive("Line weight capacity", line_weight_cap);
  if(line_cap > MAX_LINE_CAPACITY || truck_cap > MAX_LINE_CAPACITY)
  {
    printf("Maximum allowed line/truck capacity is %d\n", MAX_LINE_CAPACITY);
    exit(1);
  }

  int shm_id = shm_create();
  printf("SHM ID: %d\n", shm_id);

  prod_line *line = shm_shared_line(shm_id, line_cap, line_weight_cap);
  line_put(line, 1);
  line_put(line, 2);
  line_put(line, 3);

  char buff[5];
  fgets(buff, 5, stdin);
  line_put(line, 1);
  fgets(buff, 5, stdin);

  shm_unmap(line);
  shm_remove(shm_id);

  return 0;
}

/*
 * Shared memory manipulation functions.
 */
prod_line *shm_shared_line(int shm_id, int cap, int weight)
{
  prod_line *line = line_new(cap, weight);
  void *ptr = shm_map(shm_id);
  memcpy(ptr, line, sizeof(prod_line));
  line_dispose(line);
  return (prod_line*) ptr;
}

int shm_create()
{
  key_t shm_key = ftok(FTOK_SHM_PATH, FTOK_SHM_SEED);
  int shm_id = shmget(shm_key, sizeof(prod_line), SHM_PERM | IPC_CREAT | IPC_EXCL);
  if(shm_id == -1)
  {
    printf("Unable to create shared memory segment: %s\n", strerror(errno));
    exit(1);
  }
  return shm_id;
}

void shm_remove(int shm_id)
{
  if(shmctl(shm_id, IPC_RMID, NULL) == -1)
  {
    printf("Unable to remove shared memory segment: %s\n", strerror(errno));
    exit(errno);
  }
}


void *shm_map(int shm_id)
{
  void *ptr = shmat(shm_id, NULL, 0);
  if(ptr == ((void*) -1))
  {
    printf("Error while mapping to shared memory segment: %s\n", strerror(errno));
    exit(errno);
  }
  return ptr;
}

void shm_unmap(void *ptr)
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
  printf("Usage: %s <X> <K> <M>\n \
         \rX - truck capacity\n \
         \rK - production line capcity\n \
         \rM - production line maximum weight\n", progname);
}

void assert_positive(char *varname, long value)
{
  if(value <= 0)
  {
    printf("ERR: %s must be positive!\n", varname);
    exit(2);
  }
}
