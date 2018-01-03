/*
 * CS252: Shell project
 *
 * Template file.
 * You will need to add more code here to execute the command table.
 *
 * NOTE: You are responsible for fixing any bugs this code may have!
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <regex.h>
#include <pwd.h>
#include "command.hh"

extern char **environ;

Command::Command()
{
	// Create available space for one simple command
	_numOfAvailableSimpleCommands = 1;
	_simpleCommands = (SimpleCommand **)
		malloc( _numOfSimpleCommands * sizeof( SimpleCommand * ) );

	_numOfSimpleCommands = 0;
	_outFile = 0;
	_inFile = 0;
	_errFile = 0;
	_append = 0;
	_background = 0;
	_inCount = 0;
	_outCount = 0;
}

void Command::insertSimpleCommand( SimpleCommand * simpleCommand ) {
	if ( _numOfAvailableSimpleCommands == _numOfSimpleCommands ) {
		_numOfAvailableSimpleCommands *= 2;
		_simpleCommands = (SimpleCommand **) realloc( _simpleCommands,
			 _numOfAvailableSimpleCommands * sizeof( SimpleCommand * ) );
	}
	
	_simpleCommands[ _numOfSimpleCommands ] = simpleCommand;
	_numOfSimpleCommands++;
}

void Command:: clear() {
	for ( int i = 0; i < _numOfSimpleCommands; i++ ) {
		for ( int j = 0; j < _simpleCommands[ i ]->_numOfArguments; j ++ ) {
			free ( _simpleCommands[ i ]->_arguments[ j ] );
		}
		
		free ( _simpleCommands[ i ]->_arguments );
		free ( _simpleCommands[ i ] );
	}

	if ( _outFile ) {
		free( _outFile );
	}

	if ( _inFile ) {
		free( _inFile );
	}

	if ( _errFile ) {
		free( _errFile );
	}

	_numOfSimpleCommands = 0;
	_outFile = 0;
	_inFile = 0;
	_errFile = 0;
	_background = 0;
	_append = 0;
	_inCount = 0;
	_outCount = 0;
}

void Command::print() {
	printf("\n\n");
	printf("              COMMAND TABLE                \n");
	printf("\n");
	printf("  #   Simple Commands\n");
	printf("  --- ----------------------------------------------------------\n");
	
	for ( int i = 0; i < _numOfSimpleCommands; i++ ) {
		printf("  %-3d ", i );
		for ( int j = 0; j < _simpleCommands[i]->_numOfArguments; j++ ) {
			printf("\"%s\" \t", _simpleCommands[i]->_arguments[ j ] );
		}
	}

	printf( "\n\n" );
	printf( "  Output       Input        Error        Background\n" );
	printf( "  ------------ ------------ ------------ ------------\n" );
	printf( "  %-12s %-12s %-12s %-12s\n", _outFile?_outFile:"default",
		_inFile?_inFile:"default", _errFile?_errFile:"default",
		_background?"YES":"NO");
	printf( "\n\n" );
	
}

void Command::execute() {
	// Don't do anything if there are no simple commands
	if ( _numOfSimpleCommands == 0 ) {
		prompt();
		return;
	}
        //exits if first argument is "exit"
	if(strcmp(_simpleCommands[0]->_arguments[0], "exit") == 0) {
	  printf("Good bye!!\n");
	  _exit(1);
	}
        //save input, output, and error
	int tmp_in = dup(0);
	int tmp_out = dup(1);
	int tmp_err = dup(2);
        int fd_in;
        //Here the initial input is set
	if (_inFile) {
	  //if an input command is called more than once, an error occurred  
	  if (_inCount >= 2) {
	    perror("Ambiguous input redirect.");
	  }
	  fd_in = open(_inFile, O_RDONLY);
	}
	else {
	  //if input file is not used for command, use default input
	  fd_in = dup(tmp_in);
	}
	int ret;
	int fd_out;
	int fd_err;
        //Here the initial err is set
	if (_errFile) {
	  if (_append) {
	    //if append is set, append at end of err file
	    fd_err = open(_errFile, O_WRONLY | O_CREAT | O_APPEND, 0664);
	  }
	  else {
	    //else, truncate to err file
            fd_err = open(_errFile, O_WRONLY | O_CREAT | O_TRUNC, 0664);
	  }
	}
	else {
	  //if err file not used for command, use default err
          fd_err = dup(tmp_err);
	}
	dup2(fd_err, 2);
	close(fd_err);
	for(int i = 0; i < _numOfSimpleCommands; i++) {
	  //input is redirected
	  dup2(fd_in, 0);
	  close(fd_in);
          //check for environment variable 'setenv'
	  if(strcmp(_simpleCommands[i]->_arguments[0], "setenv") == 0) {
	    setenvExe(i);
	    return;
	  }
          //check for environment variable 'unsetenv'
	  if(strcmp(_simpleCommands[i]->_arguments[0], "unsetenv") == 0) {
	    unsetenvExe(i);
	    return;
	  }
          //check for built-in function 'cd'
	  if(strcmp(_simpleCommands[i]->_arguments[0], "cd") == 0) {
            cdExe(i);
	    return;
	  }
          if (i == _numOfSimpleCommands - 1) {
	    //executed on last simple command
	    if(_outFile) {
	      if (_outCount >= 2) {
		//if an output command is called more tban once, an error has occurred 
		perror("Ambiguous output redirect.");
	      }
	      if (_append) {
		//if append is set, append at end of out file
		fd_out = open(_outFile, O_WRONLY | O_CREAT | O_APPEND, 0664);
	      }
	      else {
		//else, truncate to err file
		fd_out = open(_outFile, O_WRONLY | O_CREAT | O_TRUNC, 0664);
	      }
	    }
	    else {
	      //if err file not used for command, use default err
	      fd_out = dup(tmp_out);
	    }
	  }
	  else {
	    //if its not the last simple command, create pipe
	    int fd_pipe[2];
	    pipe(fd_pipe);
	    fd_out=fd_pipe[1];
	    fd_in=fd_pipe[0];
	  }
          //output is redirected
	  dup2(fd_out, 1);
	  close(fd_out);
          //child process is created
	  ret = fork();
	  if (ret == 0) {
            //check for built-in function 'source'
	    if(strcmp(_simpleCommands[i]->_arguments[0], "source") == 0) {
	      sourceExe(i);
	      return;
	    }
	    //check for built-in function 'printenv'
	    if (strcmp(_simpleCommands[i]->_arguments[0], "printenv") == 0) {
	      printenvExe();
	    }
	    execvp(_simpleCommands[i]->_arguments[0], _simpleCommands[i]->_arguments);
	    perror("execvp");
	    _exit(1);
	  }
	}
	//input/output/err is restored 
	dup2(tmp_in, 0);
	dup2(tmp_out, 1);
	dup2(tmp_err, 2);
	close(tmp_in);
	close(tmp_out);
	close(tmp_err);
	if (!_background) {
	  //waits for the last command to be executed
	  waitpid(ret, NULL, 0);
	}
	// Clear to prepare for next command
	clear();
	// Print new prompt
	prompt();
}

void Command::setenvExe(int i) {
  int ret = setenv(_simpleCommands[i]->_arguments[1], _simpleCommands[i]->_arguments[2], 1);
  if (ret) {
    perror("setenv");
  }
  clear();
  prompt();
}

void Command::unsetenvExe(int i) {
  int ret = unsetenv(_simpleCommands[i]->_arguments[1]);
  if (ret) {
    perror("unsetenv");
  }
  clear();
  prompt();
}

void Command::cdExe(int i) {
  int ret = 0;
  //if no directory is give, nav to home dir
  if (_simpleCommands[i]->_numOfArguments == 1) {
    ret = chdir(getenv("HOME"));
  }
  else {
    ret = chdir(_simpleCommands[i]->_arguments[1]);
  }
  if (ret < 0) {
    perror("chdir");
  }
  clear();
  prompt();
}

void Command::sourceExe(int i) {
  FILE * f = fopen(_simpleCommands[i]->_arguments[1], "r");
  char c;
  char * buffer = (char *)malloc(200);
  int j = 0;
  int pipe_in[2];
  while ((c = getc(f)) != EOF) {
    if (c == '\n') {
      pipe(pipe_in);
      write(pipe_in[1], buffer, strlen(buffer));
      write(pipe_in[1], "\nexit\n", 6);
      close(pipe_in[1]);
      dup2(pipe_in[0], 0);
      close(pipe_in[0]);
      int ret = fork();
      if (ret == 0) {
	execvp("/proc/self/exe", NULL);
	exit(1);
      }
      else if (ret < 0) {
	perror("fork");
	exit(1);
      }
      memset(buffer, '\0', strlen(buffer));
      j = 0;
      waitpid(ret, NULL, 0);
    }
    else {
      buffer[j++] = c;
    }
  }
  fclose(f);
  clear();
  prompt();
}

void Command::printenvExe() {
  char ** envCpy = environ;
  while (*envCpy) {
    printf("%s\n", *envCpy);
    envCpy++;
  }
}

// Shell implementation

void Command::prompt() {
  if ( isatty(0) ) {
    printf("myshell>");
    fflush(stdout);
    //Print prompt
  }
}

Command Command::_currentCommand;
SimpleCommand * Command::_currentSimpleCommand;
