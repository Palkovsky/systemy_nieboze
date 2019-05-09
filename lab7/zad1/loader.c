#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/time.h>

#include "data.h"

#define MAX_N 100

// Semaphore Operations
struct sembuf SEM_P = {0, -1, SEM_UNDO}; // semwait
struct sembuf SEM_V = {0,  1, SEM_UNDO}; // semsignal

// Helpers
void exit_handler();
void print_usage(char*);
void print_timestamped(char*);

// SHM
int shm_get(void);
prod_line *shm_map(int);
void shm_unmap(prod_line*);

// SEM
int sem_acquire(void);
void sem_lock(int);
void sem_free(int);

// Globals
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
  if(N <= 0 || N > MAX_N)
  {
    printf("N must be in range from 1 to %d.\n", MAX_N);
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
  int sem_id = sem_acquire();
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
 * Semaphore functions
 */
int sem_acquire()
{
  key_t sem_key = ftok(FTOK_SEM_PATH, FTOK_SEM_SEED);
  int sem_id = semget(sem_key, 0, 0);
  if(sem_id == -1)
  {
    printf("Unable to acquire semaphore: %s\n", strerror(errno));
    exit(1);
  }

  return sem_id;
}

void sem_lock(int sem_id)
{
  if(semop(sem_id, &SEM_P, 1) < 0)
    {
      printf("Negative value when waiting for semaphore!\n");
      exit(1);
    }
}

void sem_free(int sem_id)
{
  if(semop(sem_id, &SEM_V, 1) < 0)
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
  shm_unmap(line);
  exit(0);
}

void print_usage(char *progname)
{
  printf("Usage: %s <N> {C}\n \
         \rN - size of packages. From 1 to %d.\n \
         \rC - number of cycles, optional.\n", progname, MAX_N);
}

void print_timestamped(char *str)
{
  struct timeval current_time;
	gettimeofday(&current_time, NULL);
  unsigned long timestamp = current_time.tv_sec * (int) 1e6 + current_time.tv_usec;

  printf("[%ld]: %s\n", timestamp, str);
}
