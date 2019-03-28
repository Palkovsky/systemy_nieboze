#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>

int received_signals_count;
int total_signals_count;

void handle_sigusr1(int sig, siginfo_t *info, void *ucontext){
  received_signals_count += 1;
}

void handle_sigusr2(int sig, siginfo_t *info, void *ucontext){
  printf("Received %d/%d signals.\n", received_signals_count, total_signals_count);
  exit(0);
}

void init_signal_handlers(){
  struct sigaction act;

  act.sa_flags = SA_SIGINFO;

  // Setup SIGUSR1 handler and SIGRTMIN + 1 for RT signals
  act.sa_sigaction = handle_sigusr1;
  sigfillset(&act.sa_mask);
  sigdelset(&act.sa_mask, SIGUSR1);
  sigdelset(&act.sa_mask, SIGRTMIN + 1);
  sigaction(SIGUSR1, &act, NULL);
  sigaction(SIGRTMIN + 1, &act, NULL);

  // Setup SIGUSR2 handler and SIGRTMIN + 2 for RT signals
  act.sa_sigaction = handle_sigusr2;
  sigfillset(&act.sa_mask);
  sigdelset(&act.sa_mask, SIGUSR2);
  sigdelset(&act.sa_mask, SIGRTMIN + 2);
  sigaction(SIGUSR2, &act, NULL);
  sigaction(SIGRTMIN + 2, &act, NULL);
}


void send_mode_kill(pid_t pid, long count){
  while(count--){
    if(kill(pid, SIGUSR1) != 0) printf("Error sending SIGUSR1 to %d\n", pid);
  }
  if(kill(pid, SIGUSR2) != 0) printf("Error sending SIGUSR2 to %d\n", pid);
}

void send_mode_sigqueue(pid_t pid, long count){
  union sigval val;
  while(count--){
    if(sigqueue(pid, SIGUSR1, val) != 0) printf("Error sending SIGUSR1 to %d\n", pid);
  }
  if(sigqueue(pid, SIGUSR2, val) != 0) printf("Error sending SIGUSR2 to %d\n", pid);
}

void send_mode_rt(pid_t pid, long count){
  while(count--){
    if(kill(pid, SIGRTMIN + 1) != 0) printf("Error sending SIGRTMIN + 1 to %d\n", pid);
  }
  if(kill(pid, SIGRTMIN + 2) != 0) printf("Error sending SIGRTMIN + 2 to %d\n", pid);
}

int main(int argc, char **argv){
  if(argc != 4) {
    printf("Usage: %s [catcher-pid] [sigals-count] [mode - kill/sigqueue/rt]\n", argv[0]);
    exit(1);
  }

  unsigned long catcher_pid = strtoul(argv[1], NULL, 10);
  long signals_count = strtol(argv[2], NULL, 10);
  if(signals_count < 0){
    printf("Signals count cannot be negative.\n");
    exit(1);
  } else if(signals_count > 10000){
    printf("Maximum allowed signal count is 10000.\n");
    exit(1);
  }

  received_signals_count = 0;
  total_signals_count = signals_count;
  init_signal_handlers();

  if(strcmp(argv[3], "kill") == 0)           send_mode_kill(catcher_pid, signals_count);
  else if (strcmp(argv[3], "sigqueue") == 0) send_mode_sigqueue(catcher_pid, signals_count);
  else if (strcmp(argv[3], "rt") == 0)       send_mode_rt(catcher_pid, signals_count);
  else {
    printf("Invalid mode. Valid options: kill/sigqueue/rt\n");
    exit(1);
  }

  while(1) pause();

  return 0;
}
