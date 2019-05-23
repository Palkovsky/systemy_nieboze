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
#define MUTEX_COUNT 5
#define MUTEX_ENTER 0
#define MUTEX_BUTTON 1
#define MUTEX_LEAVE 2
#define MUTEX_RIDE 3

typedef struct T_Arg {
    int identifier;
    int rides_target;
    int passenger_cap;
    Cart **cart;
    Cart *cart_head;
    Passenger **passenger;
    Passenger *passenger_head;
    pthread_mutex_t *mutex[MUTEX_COUNT];
    pthread_cond_t *cond[MUTEX_COUNT];
} T_Arg;


/*
 * Thread handlers.
 */
void *cart_routine(void*);
void *passenger_routine(void*);
void cart_test_stop_cond(T_Arg*);
void passenger_test_stop_cond(T_Arg*);

void spawn_threads(pthread_t*, int, void* (*) (void*), T_Arg*);
void join_threads(pthread_t*, int);

/*
 * Helper functions.
 */
unsigned long get_timestamp(void);
void print_timestamped(char*);
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

  if(passenger_thread_count < cart_capacity * cart_thread_count)
  {
      printf("'passenger threads' must be greater or equal to 'cart_capacity' * 'cart_thread_count'\n");
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
  targ->rides_target = rides_count;
  targ->passenger_cap = cart_capacity;
  targ->cart = &cart;
  targ->cart_head = cart_head;
  targ->passenger = &passenger;
  targ->passenger_head = passenger_head;

  for(int i=0; i<MUTEX_COUNT; i++)
  {
      targ->mutex[i] = malloc(sizeof(pthread_mutex_t));
      targ->cond[i] = malloc(sizeof(pthread_cond_t));
      pthread_mutex_init(targ->mutex[i], NULL);
      pthread_cond_init(targ->cond[i], NULL);
  }

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
    T_Arg *targ;
    int this_id;
    Cart *cart_head, *this_cart;
    char message[256];

    targ = (T_Arg*) arg;
    cart_head = targ->cart_head;
    this_id = targ->identifier;
    this_cart = cart_head + this_id;

    /* While last cart haven't done specific number of rides*/
    while(cart_head->prev->rides_done < targ->rides_target) 
    {
         /* While not in place for accepting passegners*/
        while(*targ->cart != this_cart){}

        cart_test_stop_cond(targ);

        sprintf(message, "C%d: opening doors\n", this_id);
        print_timestamped(message);
        
        this_cart->cart_state = STATE_ACCEPTING;
        
         /* While not full */
        pthread_mutex_lock(targ->mutex[MUTEX_ENTER]);
        while(this_cart->passengers_inside < targ->passenger_cap){
            pthread_cond_wait(targ->cond[MUTEX_ENTER], targ->mutex[MUTEX_ENTER]);
            pthread_mutex_unlock(targ->mutex[MUTEX_ENTER]);
        }

        this_cart->cart_state = STATE_WAITING_FOR_START;
        
        /* Wait until start pressed */
        while(this_cart->cart_state != STATE_STARTED) {}

        sprintf(message, "C%d: closing doors\n", this_id);
        print_timestamped(message);
        sprintf(message, "C%d: starting ride\n", this_id);
        print_timestamped(message);
        
        this_cart->cart_state = STATE_RIDING;
        *targ->cart = (*targ->cart)->next;
                
        sleep(1);

        /* Make sure cart is positioned at the front */
        while(*targ->cart != this_cart)
            {cart_test_stop_cond(targ);}

        this_cart->cart_state = STATE_EXITING;
        
        sprintf(message, "C%d: ride done. waiting for passengers to exit\n", this_id);
        print_timestamped(message);
        sprintf(message, "C%d: opening doors\n", this_id);
        print_timestamped(message);

        /* Wait for passengers to exit */
        pthread_mutex_lock(targ->mutex[MUTEX_LEAVE]);
        while(this_cart->passengers_inside > 0)
        {
            pthread_cond_wait(targ->cond[MUTEX_LEAVE], targ->mutex[MUTEX_LEAVE]);
            pthread_mutex_unlock(targ->mutex[MUTEX_LEAVE]);
        }

        sprintf(message, "C%d: closing doors\n", this_id);
        print_timestamped(message);

        this_cart->cart_state = STATE_NOT_ACCEPTING;
        this_cart->rides_done++;
    }

    sprintf(message, "C%d: ending execution\n", this_id);
    print_timestamped(message);
    
    free(arg);
    return NULL;
}

void *passenger_routine(void* arg)
{
    T_Arg *targ;
    int this_id;
    Passenger *passenger_head, *this_passenger;
    Cart *this_cart;
    char message[256];

    targ = (T_Arg*) arg;
    passenger_head = targ->passenger_head;
    this_id = targ->identifier;
    this_passenger = passenger_head + this_id;

    /* While last cart haven't done specific number of rides */
    while(targ->cart_head->prev->rides_done < targ->rides_target) 
    {

        /* Check if this passenger is in front of the line */
        while(*targ->passenger != this_passenger)
            {passenger_test_stop_cond(targ);}

        /* While cart not accepting */
        while((*targ->cart)->cart_state != STATE_ACCEPTING || (*targ->cart)->passengers_inside >= targ->passenger_cap)
            { passenger_test_stop_cond(targ);}

        pthread_mutex_lock(targ->mutex[MUTEX_ENTER]);
        {
            this_cart = *targ->cart;
            this_cart->passengers_inside++;
            *targ->passenger = (*targ->passenger)->next;
            sprintf(message, "P%d: entered C%d. State: %d\\%d\n",
                    this_id, this_cart->cart_id, this_cart->passengers_inside, targ->passenger_cap);
            print_timestamped(message);
        }
        pthread_cond_broadcast(targ->cond[MUTEX_ENTER]);
        pthread_mutex_unlock(targ->mutex[MUTEX_ENTER]);
        
        /* Wait until cart gets full */
        while(this_cart->cart_state != STATE_WAITING_FOR_START &&
              this_cart->cart_state != STATE_STARTED &&
              this_cart->cart_state != STATE_RIDING) {}

        /* If nobody done that, press 'start' */
        pthread_mutex_lock(targ->mutex[MUTEX_BUTTON]);
        if(this_cart->cart_state == STATE_WAITING_FOR_START)
        {
            sprintf(message, "P%d: pressed 'start' button in C%d\n", this_id, this_cart->cart_id);
            print_timestamped(message);
            this_cart->cart_state = STATE_STARTED;
        }
        pthread_mutex_unlock(targ->mutex[MUTEX_BUTTON]);

        /* Wait until ride finish */
        while(this_cart->cart_state != STATE_EXITING)
            {passenger_test_stop_cond(targ);}
        
        pthread_mutex_lock(targ->mutex[MUTEX_LEAVE]);
        {
            this_cart->passengers_inside--;
            sprintf(message, "P%d: left C%d. State: %d\\%d\n",
                    this_id, this_cart->cart_id, this_cart->passengers_inside, targ->passenger_cap);
            print_timestamped(message);
        }
        pthread_cond_broadcast(targ->cond[MUTEX_LEAVE]);
        pthread_mutex_unlock(targ->mutex[MUTEX_LEAVE]);
    }

    sprintf(message, "P%d: ending execution\n", this_id);
    print_timestamped(message);
    free(arg);
    return NULL;
}

void cart_test_stop_cond(T_Arg *targ)
{
    if(targ->cart_head->prev->rides_done < targ->rides_target)
        {return;}

    char message[256];
    sprintf(message, "C%d: ending execution\n", targ->identifier);
    print_timestamped(message);
    free(targ);
    pthread_exit(NULL);
}
void passenger_test_stop_cond(T_Arg *targ)
{
    if(targ->cart_head->prev->rides_done < targ->rides_target)
        {return;}

    char message[256];
    sprintf(message, "P%d: ending execution\n", targ->identifier);
    print_timestamped(message);
    free(targ);
    pthread_exit(NULL);
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

void print_timestamped(char *str)
{
  unsigned long timestamp = get_timestamp();
  printf("[%ld]: %s", timestamp, str);
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
