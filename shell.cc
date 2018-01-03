#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>

#include "command.hh"
int yyparse(void);

extern "C" void disp(int sig) {
  printf("\n");
  Command::_currentCommand.prompt();
}

extern "C" void disp1(int sig) {
  int pid = wait3(0, 0, NULL);
  while (waitpid(-1, NULL, WNOHANG) > 0);
}

int main() {
  struct sigaction sa;
  sa.sa_handler = disp;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  if(sigaction(SIGINT, &sa, NULL)){
    perror("sigaction");
    exit(-1);
  }
  struct sigaction sa1;
  sa1.sa_handler = disp1;
  sigemptyset(&sa1.sa_mask);
  sa1.sa_flags = SA_RESTART;
  if (sigaction(SIGCHLD, &sa1, NULL)) {
    perror("sigaction");
    exit(-1);
  }
  Command::_currentCommand.prompt();
  yyparse();
}
