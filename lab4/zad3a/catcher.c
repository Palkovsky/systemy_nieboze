#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>

int sigusr_count = 0;

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
  sigusr_count += 1;
  printf("Received %s from %d. Total: %d. Method: %d\n", strsignal(sig), info->si_pid, sigusr_count, info->si_code);
}

void handle_sigusr2(int sig, siginfo_t *info, void *ucontext){
  pid_t sender_pid  = info->si_pid;
  int send_method = info->si_code;
  int response_sig = (sig == SIGRTMIN + 2) ? SIGRTMIN + 1 : SIGUSR1;
  int received_count = sigusr_count;

  printf("End of transmission! Sending back %d of %s to %d.\n", sigusr_count, strsignal(response_sig), sender_pid);
  while(sigusr_count--) {
    send_sig(sender_pid, response_sig, send_method);
  }
  sigusr_count = 0;
  send_sig(sender_pid, sig, send_method); // Send EOT sig SIGUSR2 in case of SIGUSR1 or SIGRTMIN+2 in case of SIGRTMIN+1
  printf("%d of %s sent back!\n", received_count, strsignal(response_sig));
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
