#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "data.h"

// Helpers
void exit_handler();
void print_usage(char*);
void print_timestamped(char*);

// SHM
int shm_get(void);
prod_line *shm_map(int);
void shm_unmap(prod_line*);

// SEM
sem_t *sem_acquire(void);
void sem_lock(sem_t*);
void sem_free(sem_t*);

// Globals
sem_t *sem_id;
prod_line *line;

int main(int argc, char **argv)
{
  if(argc != 2 && argc != 3)
  {
    printf("ERR: Invalid number of arguments.\n");
    print_usage(argv[0]);
    exit(1);
  }

  long N = strtol(argv[1], NULL, 10);
  long C = -1;
  if(N <= 0)
  {
    printf("N must be positive.\n");
    exit(1);
  }

  if(argc == 3)
  {
    C = strtol(argv[2], NULL, 10);
    if(C <= 0)
    {
      printf("C must be positive!\n");
      exit(1);
    }
  }

  signal(SIGINT, exit_handler);

  int shm_id = shm_get();
  sem_id = sem_acquire();
  line = shm_map(shm_id);

  long ordnum = 0;
  char message[256];
  while(C != 0)
  {
    sem_lock(sem_id);
    {
      if(line_put(line, N, ordnum) == 0)
      {
        sprintf(message,
                "PID %d: Put his %ld package. Weight: %ld. Line cap: %ld/%ld. Line weight: %ld/%ld",
                getpid(),
                ordnum,
                N,
                line->size,
                line->capacity,
                line_weight(line),
                line->weight_capacity);
        print_timestamped(message);
        ordnum++;
        if(C > 0)
        {
          C--;
        }
      }
      else
      {
        // This spams too hard. Uncomment if needed.
        //sprintf(message, "PID %d: Not enough space on line.", getpid());
        //print_timestamped(message);
      }
    }
    sem_free(sem_id);
  }

  exit_handler();
  return 0;
}

/*
 * Shared memory access functions.
 */
int shm_get()
{
  int shm_id = shm_open(SHM_NAME, O_RDWR, SHM_PERM);
  if(shm_id == -1)
  {
    printf("Unable to access shared memory segment: %s\n", strerror(errno));
    exit(1);
  }
  return shm_id;
}

prod_line *shm_map(int shm_id)
{
  void *ptr = mmap(NULL, sizeof(prod_line), PROT_READ | PROT_WRITE, MAP_SHARED, shm_id, 0);
  if(ptr == ((void*) -1))
  {
      printf("Error while mapping to shared memory segment: %s\n", strerror(errno));
      exit(errno);
  }
  return (prod_line*) ptr;
}

void shm_unmap(prod_line *ptr)
{
  if(munmap(ptr, sizeof(prod_line)) == -1)
  {
    printf("Error while detaching shared memory segment: %s\n", strerror(errno));
    exit(errno);
  }
}

/*
 * Semaphore functions
 */
sem_t *sem_acquire()
{
  sem_t *sem_id = sem_open(SEM_NAME, O_RDWR);
  if(sem_id == NULL)
  {
    printf("Unable to acquire semaphore: %s\n", strerror(errno));
    exit(1);
  }
  return sem_id;
}

void sem_lock(sem_t *sem_id)
{
  if(sem_wait(sem_id) < 0)
  {
    printf("Negative value when waiting for semaphore!\n");
    exit(1);
  }
}

void sem_free(sem_t *sem_id)
{
  if(sem_post(sem_id) < 0)
  {
    printf("Negative value when freeing semaphore!\n");
    exit(1);
  }
}

/*
 * Helpers
 */
void exit_handler()
{
  sem_close(sem_id);
  shm_unmap(line);
  exit(0);
}

void print_usage(char *progname)
{
  printf("Usage: %s <N> {C}\n \
         \rN - size of packages. Must be positive.\n \
         \rC - number of cycles, optional.\n", progname);
}

void print_timestamped(char *str)
{
  struct timeval current_time;
	gettimeofday(&current_time, NULL);
  unsigned long timestamp = current_time.tv_sec * (int) 1e6 + current_time.tv_usec;

  printf("[%ld]: %s\n", timestamp, str);
}
