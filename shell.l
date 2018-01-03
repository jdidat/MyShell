
/*
 *
 * CS-252 Fall 2017
 * shell.l: lexical analyzer for shell
 * You have to extend it.
 *
 */

%{

#include <cstring>
#include <string.h>
#include "y.tab.hh"
#include <stdio.h>
#include <stdlib.h>

extern "C" char * read_line();

int mygetc(FILE * f) {
  static char *p;
  char ch;
  if (!isatty(0)) {
  // stdin is not a tty. Call real getc
    return getc(f);
  }
  // stdin is a tty. Call our read_line.
  if (p==NULL || *p == 0) {
    char * s = read_line();
    p = s;
  }
  ch = *p;
  p++;
  return ch;
}

#undef getc
#define getc(f) mygetc(f)

static  void yyunput (int c,char *buf_ptr  );

void myunputc(int c) {
  unput(c);
}

%}

%%

\n {
  return NEWLINE;
}

[ \t] {
  /* Discard spaces and tabs */
}

">" {
  return GREAT;
}

"<" {
  return LESS;
}

">>" {
  return GREATGREAT;
}

">&" {
  return GREATAMPERSAND;
}

">>&" {
  return GREATGREATAMPERSAND;
}

"|" {
  return PIPE;
}

"&" {
  return AMPERSAND;
}

"2>" {
  return TWOGREAT;
}

["].*["] { 
    yylval.string_val = strdup(yytext+1);
    yylval.string_val[strlen(yylval.string_val) - 1] = '\0';
    return WORD;
}

[^ \t\n|><&]*\\[^ \t\n]* {
    char * rmEsc = (char *)malloc(200);
    int count = 0;
    char * tmp = yytext;
    while (*tmp) {
       if (*tmp == '\\') {
           if(*(tmp + 1) == '\\')  {
               tmp += 2;
               rmEsc[count++] = '\\';
           }
           else { 
               rmEsc[count++] = *(++tmp);
           }
       }
       else {
           rmEsc[count++] = *tmp; 
       }
       tmp++;
   }
  rmEsc[count] = '\0';
  yylval.string_val = strdup(rmEsc);
  return WORD;
}

[`].*[`] {
   //remove ` from command
   char * c = strdup(yytext) + 1;
   c[strlen(yytext) - 2] = '\0';
   
   //save original in/out to be restored for later
   int original_in = dup(0);
   int original_out = dup(1);
   
   //create both pipes and call pipe
   int fd_pipe_in[2];
   int fd_pipe_out[2];
   pipe(fd_pipe_in);
   pipe(fd_pipe_out);
   
   //write to the output of pipe
   write(fd_pipe_in[1], c, strlen(c));   
   write(fd_pipe_in[1], "\nexit\n", 6);
   close(fd_pipe_in[1]); 
   
   //close pipe
   dup2(fd_pipe_in[0], 0);
   close(fd_pipe_in[0]);
   dup2(fd_pipe_out[1], 1);
   close(fd_pipe_out[1]);   
   
   //create a new process and execute
   int ret = fork();
   if (ret == 0) {
      execvp("/proc/self/exe", NULL);
      exit(1);
   }
   
   //exit if creation of child process failed
   else if (ret < 0) {
      perror("fork");
      exit(1);
   }
   
   //restore original in/out
   dup2(original_in, 0);
   dup2(original_out, 1);
   close(original_in);
   close(original_out);   
   
   //write the output of the child process into a buffer
   //then, unputc the characters of the buffer back into the scanner's buffer
   //to be used by the "upper command"
   char d;
   char * b = (char *)malloc(sizeof(char) * 100);
   int count = 0; 
   while (read(fd_pipe_out[0], &d, 1) != 0) {
      if (d != '\n') { 
         b[count] = d;
	 count++;
      }
      else {
         b[count] = ' ';
	 count++;
      }
   }
   count--;
   b[count] = '\0';
   count = strlen(b);
   while (count >= 0) {
       myunputc(b[count]);
       count--;
   }
   close(fd_pipe_out[0]);
}

[$].* {
  char * word = strdup(yytext);
  char * dollar = strchr(word, '$');
  //char * dollar = strdup(yytext) + 1;
  char * finalWord = (char *)malloc(sizeof(char)*50);
  //Use loop that repeatedly checks string for another $
  while (dollar) {
    if (*(dollar + 1) == '{' && *(dollar + 2) != '}') {
      char * inDollar = dollar + 2;
      if (*dollar == '$') {
        //int pid = getpid();
        //char * pidString = (char *)malloc(sizeof(char) * 20);
        //itoa(pid, pidString, 10);
        //yylval.string_val = strdup(pidString);
        //return WORD;
      }
      if (*dollar == '?') {

      }
      if (*dollar == '!') {

      }
      if (*dollar == '_') {

      }
      char * envDollar = (char *)malloc(sizeof(char)*20);
      char * envInDollar = envDollar;
      //Build up string w/ name of env variable in $ and brackets
      while (*inDollar != '}') {
        *envInDollar = *inDollar;
        envInDollar++; 
        inDollar++;
      }
      *envInDollar = '\0';
      char * getEnv = getenv(envDollar);
      //get the env variable and cat it to replace      
      //finalWord[0] = '\0';
      strcat(finalWord, getEnv);
      //reposition original argument sent in
      while (*(word-1) != '}') {
        word++;
      }
      char * inBetween = (char *)malloc(sizeof(char)*20);
      char * inBetweenDollar = inBetween;
      //build up string of what's in between $
      while (*word != '$' && *word) {
        *inBetweenDollar = *word;
        inBetweenDollar++; 
        word++;
      }
      *inBetweenDollar = '\0';
      //cat whats in between to word
      strcat(finalWord, inBetween);
      finalWord[strlen(finalWord)] = '\0';
    }
    dollar++;
    dollar = strchr(dollar, '$');
  }
  yylval.string_val = strdup(finalWord);
  return WORD;
} 

[~][^ >|<&`]* {
  if (strlen(yytext) == 1 || yytext[1] == '/') {
    char * c = strdup(getenv("HOME"));
    yylval.string_val = strdup(c);
    return WORD;
  }
  else {
    char * c = strdup(yytext + 1);
    char * home = (char *)malloc(sizeof(char) * 50);
    strcat(home, "/homes/");
    strcat(home, c);
    yylval.string_val = strdup(home);
    return WORD;
  }
}

[^ \t\n|><][^ \t\n|><]*  {
  /* Assume that file names have only alpha chars */
  yylval.string_val = strdup(yytext);
  return WORD;
}

{
  /* Invalid character in input */
  return NOTOKEN;
}

%%
