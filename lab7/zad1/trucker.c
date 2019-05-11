#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/time.h>

#include "data.h"

// Utils
void print_usage(char*);
void assert_positive(char*, long);
void exit_handler();
void print_timestamped(char*);
unsigned long get_timestamp(void);

// SHM
int shm_create(void);
void *shm_map(int);
prod_line *shm_shared_line(int, int, int);
void shm_unmap(void*);
void shm_remove(int);

// SEM
int sem_create(void);
void sem_remove(int);
void sem_lock(int);
void sem_free(int);

// Processes
void truck_logic(void);
void start_workers(int);

// Semaphore Operations
struct sembuf SEM_P = {0, -1, SEM_UNDO}; // semwait
struct sembuf SEM_V = {0,  1, SEM_UNDO}; // semsignal

// Shared memory and semaphore set identifiers
int shm_id;
int sem_id;

// Shared line object
prod_line *line;

// PIDs of workers
long truck_cap;
long line_cap;
long line_weight_cap;
long workers_cycles;
long workers_count;
pid_t *workers;

int main(int argc, char **argv)
{
  if(argc != 6)
  {
    printf("ERR: Invalid number of arguments.\n");
    print_usage(argv[0]);
    exit(1);
  }

  workers_count    = strtol(argv[1], NULL, 10);
  workers_cycles   = strtol(argv[2], NULL, 10);
  truck_cap   = strtol(argv[3], NULL, 10);
  line_cap    = strtol(argv[4], NULL, 10);
  line_weight_cap  = strtol(argv[5], NULL, 10);

  assert_positive("Workers count", workers_count);
  assert_positive("Workers cycles", workers_cycles);
  assert_positive("Truck capacity", truck_cap);
  assert_positive("Line capacity", line_cap);
  assert_positive("Line weight capacity", line_weight_cap);
  if(line_cap > MAX_LINE_CAPACITY || truck_cap > MAX_LINE_CAPACITY)
  {
    printf("Maximum allowed line/truck capacity is %d\n", MAX_LINE_CAPACITY);
    exit(1);
  }

  srand(time(NULL));
  signal(SIGINT, exit_handler);

  shm_id = shm_create();
  sem_id = sem_create();
  printf("SHM ID: %d\n", shm_id);
  printf("SEM ID: %d\n", sem_id);

  line = shm_shared_line(shm_id, line_cap, line_weight_cap);


  // Create truck process
  if(fork() == 0)
  {
    truck_logic();
    exit(0);
  }

  // Create loader processes
  start_workers(workers_count);

  // Wait for user to SIGINT
  for(;;)
  {
  }

  exit_handler();
  return 0;
}


/*
 * Function responsible for loading packages.
 * It won't let off semaphore if truck left.
 */
void truck_logic()
{
  char message[256];
  long loaded = 0;
  for(;;)
  {
    sem_lock(sem_id);
    {
      if(loaded == truck_cap)
      {
        sprintf(message, "TRUCK: Truck full! Sending new one...");
        print_timestamped(message);
	// colud also fork() here if we would really want another process for the truck
	// but it would require extra flag to inform that semaphore is taken
        loaded = 0;
        sprintf(message, "TRUCK: New truck ready. Capacity: %ld/%ld.", loaded, truck_cap);
        print_timestamped(message);
      }

      if(line->size > 0)
      {
         prod_node *package      = line_oldest(line);
         unsigned long time_diff = get_timestamp() - package->timestamp;
         loaded++;
         sprintf(message, "TRUCK: Loading package %d/%ld. Weight: %ld. Truck: %ld/%ld. Time diff: %ld", package->producer, package->ordnum, package->weight, loaded, truck_cap, time_diff);
         print_timestamped(message);
      }
    }
    sem_free(sem_id);
  }
}

/*
 * Starts worker processes and stores their PIDs in 'workers' array.
 */
void start_workers(int count)
{
  workers = calloc(count, sizeof(pid_t));
  for(int i=0; i<count; i++)
  {

    // Need to randomize N here, because srand() is in this process.
    int N = rand() % (line_weight_cap-1) + 1;
    int C = workers_cycles;

    pid_t pid = fork();
    if(pid == 0) // Child process
    {

      char N_str[10];
      char C_str[10];
      sprintf(N_str, "%d", N);
      sprintf(C_str, "%d", C);

      // For workes in infite loops, replace with:
      // execl(LOADER_EXECUTABLE, LOADER_EXECUTABLE,  N_str, NULL)
      if(execl(LOADER_EXECUTABLE, LOADER_EXECUTABLE,  N_str, C_str, NULL) == -1)
      {
        printf("Unable to exec %s. PID %d\n", LOADER_EXECUTABLE, getpid());
        exit(1);
      }
    }

    workers[i] = pid;
  }
}

/*
 * Shared memory access functions.
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
 * Semaphores
 */
int sem_create()
{
  key_t sem_key = ftok(FTOK_SEM_PATH, FTOK_SEM_SEED);
  int sem_id = semget(sem_key, 1, SEM_PERM | IPC_CREAT | IPC_EXCL);
  if(sem_id == -1)
  {
    printf("Unable to create semaphore: %s\n", strerror(errno));
    exit(1);
  }

  union semun
  {
    int val;
    struct semid_ds *buf;
    ushort array [1];
  } sem_attr;

  sem_attr.val = 1;
  if(semctl(sem_id, 0, SETVAL, sem_attr) == -1)
  {
    printf("Unable to set initial value of semaphore: %s\n", strerror(errno));
    exit(1);
  }

  return sem_id;
}

void sem_remove(int sem_id)
{
  if(semctl(sem_id, 0, IPC_RMID) == -1)
  {
    printf("Unable to remove semaphore: %s\n", strerror(errno));
    exit(errno);
  }
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
 * Utils
 */
void exit_handler()
{
  for(int i=0; i<workers_count; i++)
  {
    kill(workers[i], SIGINT);
  }

  shm_unmap(line);
  shm_remove(shm_id);
  sem_remove(sem_id);
  exit(0);
}

void print_usage(char *progname)
{
  printf("Usage: %s <W> <WC> <X> <K> <M>\n \
         \rW  - workers count\n \
         \rWC - worker cycles \n \
         \rX  - truck capacity\n \
         \rK  - production line capcity\n \
         \rM  - production line maximum weight\n", progname);
}

unsigned long get_timestamp()
{
  struct timeval current_time;
	gettimeofday(&current_time, NULL);
  return current_time.tv_sec * (int) 1e6 + current_time.tv_usec;
}

void print_timestamped(char *str)
{
  unsigned long timestamp = get_timestamp();
  printf("[%ld]: %s\n", timestamp, str);
}

void assert_positive(char *varname, long value)
{
  if(value <= 0)
  {
    printf("ERR: %s must be positive!\n", varname);
    exit(2);
  }
}
