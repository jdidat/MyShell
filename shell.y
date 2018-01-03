

/*
 * CS-252
 * shell.y: parser for shell
 *
 * This parser compiles the following grammar:
 *
 *	cmd [arg]* [> filename]
 *
 * you must extend it to understand the complete shell grammar
 *
 */

%code requires 
{
  #include <stdio.h>
  #include <string.h>
  #include <stdlib.h>
  #include <regex.h>
  #include <sys/types.h>
  #include <dirent.h>
  #include <unistd.h>
  #include <signal.h>
  #include <sys/stat.h>
  #include "command.hh"
  void yyerror(const char * s);
  int yylex();
  void expandWildCards(char * prefix, char * suffix);
  void addFilesToTable();
  int compareFiles(const void * f1, const void * f2);
  bool is_dir(const char * path);

#if __cplusplus > 199711L
#define register      // Deprecated in C++11 so remove the keyword
#endif
}

%union
{
  char        *string_val;
  // Example of using a c++ type in yacc
  //std::string *cpp_string;
}

%token <string_val> WORD
%token NOTOKEN GREAT NEWLINE LESS GREATGREAT GREATAMPERSAND PIPE AMPERSAND GREATGREATAMPERSAND TWOGREAT

%{
//#define yylex yylex
#include <cstdio>
#include "command.hh"

void yyerror(const char * s);
int yylex();

%}

%%

goal:
  commands
  ;

commands:
  command
  | commands command
  ;

command: simple_command
  ;

simple_command:	
  pipe_list iomodifier_list background_opt NEWLINE {
    Command::_currentCommand.execute();
  }
  | NEWLINE {
    Command::_currentCommand.clear();
    Command::_currentCommand.prompt();
  }
  | error NEWLINE { yyerrok; }
  ;

pipe_list:
  pipe_list PIPE command_and_args
  | command_and_args
  ;

command_and_args:
  command_word argument_list {
    Command::_currentCommand.
    insertSimpleCommand( Command::_currentSimpleCommand );
  }
  ;

argument_list:
  argument_list argument
  | /* can be empty */
  ;

argument:
  WORD {
    char * wildCheck = strdup($1);
    if (strchr(wildCheck, '*') || strchr(wildCheck, '?')) {
      expandWildCards(NULL, wildCheck);
      addFilesToTable();
      //free(fileArray);
    }
    else {
      Command::_currentSimpleCommand->insertArgument($1);
    }
  }
  ;

command_word:
  WORD {
    Command::_currentSimpleCommand = new SimpleCommand();
    Command::_currentSimpleCommand->insertArgument( $1 );
  }
  ;

iomodifier_opt:
  GREAT WORD {
    Command::_currentCommand._outFile = $2;
    Command::_currentCommand._outCount++;
  } 
  | LESS WORD {
    Command::_currentCommand._inFile = $2;
    Command::_currentCommand._inCount++;
  }
  | GREATGREAT WORD {
    Command::_currentCommand._outFile = $2;
    Command::_currentCommand._append = 1;
    Command::_currentCommand._outCount++;
  }
  | GREATAMPERSAND WORD {
    Command::_currentCommand._outFile = $2;
    Command::_currentCommand._errFile = strdup($2);
    Command::_currentCommand._outCount++;
  }
  | GREATGREATAMPERSAND WORD {
    Command::_currentCommand._outFile = $2;
    Command::_currentCommand._errFile = strdup($2);
    Command::_currentCommand._outCount++;
    Command::_currentCommand._append = 1;
  }
  | TWOGREAT WORD {
    Command::_currentCommand._errFile = $2;
    Command::_currentCommand._outCount++;
  }
  ;

iomodifier_list:
  iomodifier_list iomodifier_opt
  | iomodifier_opt
  | /* can be empty */
  ;

background_opt:
  AMPERSAND {
    Command::_currentCommand._background = 1;
  }
  | /* empty */
  ;
  

%%

int compareFiles (const void * f1, const void * f2) {
  const char * str1 = *(const char **) f1;
  const char * str2 = *(const char **) f2;
  return strcmp(str1, str2);
}

#define MAXFILENAME 2024
int nEntries = 0;
int maxEntries = 20;
char ** fileArray;
char * suffixCpy;

void expandWildCards(char * prefix, char * suffix) {
  //If first time calling func, malloc array and save suffix for later use
  if (prefix == NULL) {
    fileArray = (char **)malloc(sizeof(char *) * maxEntries);
    suffixCpy = strdup(suffix);
  }
  //If suffix empty, put prefix in array
  if (suffix[0] == '\0') {
    //if the array size has been reached, realloc
    if (nEntries == maxEntries) {
      maxEntries *= 2;
      fileArray = (char **)realloc(fileArray, maxEntries * sizeof(char *));
    }
    prefix[strlen(prefix)] = '\0';
    fileArray[nEntries] = strdup(prefix);
    nEntries++;
    return;
  }
  //use str to obtain next component in suffix
  char * s = strchr(suffix, '/');
  char component[MAXFILENAME];
  //copy up to first '/' to component from suffix
  if (s != NULL) {
    strncpy(component, suffix, s - suffix);
    suffix = s + 1;
  }
  //if else reached, then the last path has been reached
  //copy whole thing
  else {
    strcpy(component, suffix);
    suffix = suffix + strlen(suffix);
  }
  //expand the component
  char newPrefix[MAXFILENAME];
  //if component does not have wildcards, copy component
  //to newPrefix and recursively call expandWildCards
  if (!strchr(component, '*') && !strchr(component, '?')) {
    if (prefix == NULL) {
      sprintf(newPrefix, "%s", component);
    }
    else {
      sprintf(newPrefix, "%s/%s", prefix, component);
    }
    expandWildCards(newPrefix, suffix);
    return;
  }
  //If made it past if, the component has wildcards
  //so, convert component to regular expression
  char * reg = (char *)malloc(2*strlen(component) + 10);
  char * a = component;
  char * r = reg;
  *r = '^';
  r++;
  while (*a) {
    if (*a == '*') {
      *r = '.';
      r++;
      *r = '*';
      r++;
    }
    else if (*a == '?') {
      *r = '.';
      r++;
    }
    else if (*a == '.') {
      *r = '\\';
      r++;
      *r= '.';
      r++;
    }
    else {
      *r = *a;
      r++;
    }
    a++;
  }
  *r='$';
  r++;
  *r = '\0';
  regex_t re;
  //compile regular expression w/ regcomp
  int expbuf = regcomp(&re, reg, 0);
  if (expbuf > 0) {
    return;
  }
  char * dir;
  //if prefix is NULL or empty
  //the current directory is used
  if (prefix == NULL) {
    char * tempDir = ".";
    dir = strdup(tempDir);
  }
  else if (strcmp(prefix, "") == 0) {
    char * tempDir = "/";
    dir = strdup(tempDir);
  }
  else {
    dir = strdup(prefix);
  }
  dir[strlen(dir)] = '\0';
  DIR * d = opendir(dir);
  if (d == NULL) {
    return;
  }
  struct dirent * ent;
  //check what entries match regex
  //in the given directory
  while ((ent = readdir(d)) != NULL) {
    //check if name matches w/ regexec
    if (regexec(&re, ent->d_name, 0, NULL, 0) == 0) {
      //ifs are used to check if
      //hidden files are wanted or not
      if (ent->d_name[0] == '.') {
	if (component[0] == '.') {
	  //Entry matches, so name of entry
	  //is added to prefix and
	  //expandWildCards is called recursively
	  if (prefix == NULL) {
	    sprintf(newPrefix, "%s", ent->d_name);
	  }
	  else {
	    sprintf(newPrefix, "%s/%s", prefix, ent->d_name);
	  }
	  expandWildCards(newPrefix, suffix);
	}
      }
      else {
	if (component[0] != '.') {
	  if (prefix == NULL) {
	    sprintf(newPrefix, "%s", ent->d_name);
	  }
	  else {
	    sprintf(newPrefix, "%s/%s", prefix, ent->d_name);
	  }
	  expandWildCards(newPrefix, suffix);
	}
      }
    }
  }
  closedir(d);
}  
  
void addFilesToTable() {
  //Add arguemnts
  if (nEntries == 0) {
    Command::_currentSimpleCommand->insertArgument(strdup(suffixCpy));
  }
  else {
    //sort array w/ quicksort
    qsort(fileArray, nEntries, sizeof(char *), compareFiles);
    for (int i = 0; i < nEntries; i++) {
      Command::_currentSimpleCommand->insertArgument(strdup(fileArray[i]));
    }
  }
  nEntries = 0;
}

void
yyerror(const char * s)
{
  fprintf(stderr,"%s", s);
}

#if 0
main()
{
  yyparse();
}
#endif
