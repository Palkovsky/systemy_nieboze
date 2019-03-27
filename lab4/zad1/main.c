#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>

pid_t child_pid = 0;

void sigint_handler();
void sigstp_handler();
void run_date_printer();
void kill_date_printer();

void print_current_time(){
  char *time_buff = calloc(50, sizeof(char));
  time_t timestamp = time(NULL);
  strftime(time_buff, 50, "%Y-%m-%d %H:%M:%S", localtime(&timestamp));
  printf("%s\n", time_buff);
  free(time_buff);
}

void sigstp_handler(){
  if(child_pid == 0){
    run_date_printer();
  }else{
    printf("Oczekuje na CTRL+Z - kontynuacja; CTRL+C - zakonczenie\n");
    kill(child_pid, SIGKILL);
    child_pid = 0;
  }
}

void sigint_handler(){
  printf("Odebrano sygnal SIGINT.\n");
  kill(child_pid, SIGKILL);
  exit(0);
}

void run_date_printer(){
  if(child_pid != 0) return;

  child_pid = fork();
  if(child_pid == 0){
    execl("./spam.sh", "spam", NULL);
  }
  pause();
}

int main(int argc, char **argv){

  struct sigaction action;
  action.sa_handler = sigint_handler;
  sigemptyset(&action.sa_mask);
  action.sa_flags = 0;
  sigaction(SIGINT, &action, NULL);

  signal(SIGTSTP, sigstp_handler);

  run_date_printer();
  while(1) {}

  return 0;
}
