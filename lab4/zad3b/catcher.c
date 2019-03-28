#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>

int sigusr_count;

void send_sig(pid_t target, int sig, int method){
  union sigval val;
  switch(method){
  case SI_USER: //kill
    kill(target, sig);
    break;
  case SI_QUEUE: // sigqueue
    sigqueue(target, sig, val);
    break;
  }
}

void handle_sigusr1(int sig, siginfo_t *info, void *ucontext){
  pid_t sender_pid = info->si_pid;
  int send_method  = info->si_code;
  sigusr_count += 1;
  printf("Received %s from %d. Total: %d. Method: %d\n", strsignal(sig), sender_pid, sigusr_count, send_method);
  send_sig(sender_pid, sig, send_method); // Send confirmation back
}

void handle_sigusr2(int sig, siginfo_t *info, void *ucontext){
  pid_t sender_pid  = info->si_pid;
  int send_method = info->si_code;
  printf("Received %d, sending EOT.\n", sigusr_count);
  sigusr_count = 0;
  send_sig(sender_pid, sig, send_method); // Send EOT sig
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

int main(int argc, char **argv){
  printf("========== CATCHER || PID %d  ==========\n", getpid());
  printf("Waiting for signals...\n");

  init_signal_handlers();

  while(1) pause();

  return 0;
}
