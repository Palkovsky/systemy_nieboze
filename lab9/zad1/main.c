#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <pthread.h>

#include "data.h"

/*
 * Helper struct to prevent global variables.
 */
typedef struct T_Arg {
    int identifier;
    Cart **cart;
    Cart *cart_head;
    Passenger **passenger;
    Passenger *passenger_head;
} T_Arg;

/*
 * Thread handlers.
 */
void *cart_routine(void*);
void *passenger_routine(void*);

void spawn_threads(pthread_t*, int, void* (*) (void*), T_Arg*);
void join_threads(pthread_t*, int);

/*
 * Helper functions.
 */
unsigned long get_timestamp(void);
void assert_positive(long, const char*);
void print_usage(const char*);

int main(int argc, char **argv)
{
  if(argc != 5)
  {
    printf("Invalid number of arguments.\n");
    print_usage(argv[0]);
    exit(1);
  }

  long passenger_thread_count, cart_thread_count;
  long cart_capacity, rides_count;

  passenger_thread_count = strtol(argv[1], NULL, 10);
  cart_thread_count = strtol(argv[2], NULL, 10);
  cart_capacity = strtol(argv[3], NULL, 10);
  rides_count = strtol(argv[4], NULL, 10);

  assert_positive(passenger_thread_count, "passenger threads");
  assert_positive(cart_thread_count, "cart threads");
  assert_positive(cart_capacity, "cart capacity");
  assert_positive(rides_count, "rides count");

  if(passenger_thread_count < cart_capacity)
  {
      printf("'passenger threads' must be greater or equal to 'cart_capacity'\n");
      exit(1);
  }

  T_Arg *targ;
  Cart *cart, *cart_head;
  Passenger *passenger, *passenger_head;
  pthread_t *passenger_tids;
  pthread_t *cart_tids;

  cart_head = cart = init_roller_coaster(cart_thread_count);
  passenger_head = passenger = init_passenger_queue(passenger_thread_count);

  targ = malloc(sizeof(T_Arg));
  targ->cart = &cart;
  targ->cart_head = cart_head;
  targ->passenger = &passenger;
  targ->passenger_head = passenger_head;

  cart_tids = malloc(sizeof(pthread_t) * cart_thread_count);
  passenger_tids = malloc(sizeof(pthread_t) * passenger_thread_count);

  spawn_threads(cart_tids, cart_thread_count, cart_routine, targ);
  spawn_threads(passenger_tids, passenger_thread_count, passenger_routine, targ);
  
  join_threads(cart_tids, cart_thread_count);
  join_threads(passenger_tids, passenger_thread_count);
  
  free(passenger_tids);
  free(cart_tids);
  free(cart_head);
  free(passenger_head);
  free(targ);
  
  return 0;
}


/*
 * Threads logic
 */
void *cart_routine(void* arg)
{
    T_Arg *targ = (T_Arg*) arg;
    Cart *this_cart = targ->cart_head + targ->identifier;
    printf("Started %d cart.\n", this_cart->cart_id);
    free(arg);
    return NULL;
}

void *passenger_routine(void* arg)
{
    T_Arg *targ = (T_Arg*) arg;
    Passenger *this_passenger = targ->passenger_head + targ->identifier;
    printf("Started %d passenger.\n", this_passenger->passenger_id);
    free(arg);
    return NULL;
}

/*
 * API calls
 */
void spawn_threads(pthread_t *tids, int size, void *(*routine) (void*), T_Arg *arg)
{
    for(int i=0; i < size; i++)
    {

      T_Arg *targ = malloc(sizeof(T_Arg));
      memcpy(targ, arg, sizeof(T_Arg));
      targ->identifier = i;
      
      if(pthread_create(&tids[i], NULL, routine, targ) < 0)
      {
          printf("Unable to spawn thread: %s\n", strerror(errno));
          exit(1);
      }
  }
}

void join_threads(pthread_t *tids, int size)
{
  for(int i=0; i < size; i++)
  {
      void *ret;
      if(pthread_join(tids[i], &ret) < 0)
      {
          printf("Unable to join thread: %s\n", strerror(errno));
          exit(1);
      }
  }
}

/*
 * Helpers
 */
unsigned long get_timestamp()
{
  struct timeval current_time;
	gettimeofday(&current_time, NULL);
  return current_time.tv_sec * (int) 1e6 + current_time.tv_usec;
}

void assert_positive(long value, const char* varname)
{
    if(value > 0)
        {return;}
    printf("'%s' must be positive\n", varname);
    exit(1);
}

void print_usage(const char* progname)
{
  printf("Usage: %s <passenger_thread_count> <cart_thread_count> <cart_capacity> <rides_count>\n",
         progname);
}
