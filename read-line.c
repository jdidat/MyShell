/*
 * CS354: Operating Systems. 
 * Purdue University
 * Example that shows how to read one line with simple editing
 * using raw terminal.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>

#define MAX_BUFFER_LINE 2048

//extern void tty_raw_mode(void);

// Buffer where line is stored
int line_length;
char line_buffer[MAX_BUFFER_LINE];
int cursor_location;
// Simple history array
// This history does not change. 
// Yours have to be updated.
int history_index = 0;
char ** history;
int history_length = 0;

void read_line_print_usage()
{
  char * usage = "\n"
    " ctrl-?                    Print usage\n"
    " Backspace key (ctrl-H)    Deletes last character\n"
    " up arrow                  See last command in the history\n"
    " down arrow                See next command in the history\n"
    " right arrow               move cursor to the right\n"
    " left arrow                move cursor to the left\n"
    " Home key (ctrl-A)         cursor moves to beginning of line\n"
    " End key (ctrl-E)          cursor moves to end of line\n"
    " Delete key (ctrl-D)       removes character at cursor\n";
  
  write(1, usage, strlen(usage));
}

/* 
 * Input a line with some basic editing.
 */
char * read_line() {
  // Set terminal in raw mode
  struct termios restore;
  tcgetattr(0, &restore);
  tty_raw_mode();
  line_length = 0;
  cursor_location = 0;
  // Read one line until enter is typed
  while (1) {
    // Read one character in raw mode.
    char ch;
    read(0, &ch, 1);
    if (ch >= 32 && ch <= 126) {
      // It is a printable character. 
      // Do echo
      write(1,&ch,1);
      // If max number of character reached return.
      if (line_length == MAX_BUFFER_LINE-2) {
	break; 
      }
      //characters of buffer are moved after the cursor position
      int i;
      for (i = line_length + 1; i > cursor_location; i--) {
	line_buffer[i] = line_buffer[i - 1];
      }
      line_buffer[cursor_location] = ch;
      line_length++;
      cursor_location++;
      //buffer is reprinted because it has been manipulated
      int j;
      for (j = cursor_location; j < line_length; j++) {
	ch = line_buffer[j];
	write(1, &ch, 1);
      }
      //erases the leftover character after printing
      ch = ' ';
      write(1,&ch,1);
      ch = 8;
      write(1,&ch,1);
      int k;
      for (k = cursor_location; k < line_length; k++) {
	ch = 27;
	write(1,&ch,1);
	ch = 91;
	write(1,&ch,1);
	ch = 68;
	write(1,&ch,1);
      }
    }
    else if (ch == 31) {
      // ctrl-?, prints out usability
      read_line_print_usage();
      line_buffer[0]=0;
      break;
    }
    else if (ch == 8 || ch == 127) {
      // <backspace> was typed. Remove previous character read.
      // checks to see if at the front of buffer
      // Mac keyboards use delete key for backspace
      if (line_length == 0 || cursor_location == 0) {
	continue;
      }
      //move the characters in line over after the cursor 
      int i;
      for (i = cursor_location; i < line_length; i++) {
	  line_buffer[i - 1] = line_buffer[i];
      }
      line_length--;
      cursor_location--;

      // Write a space to erase the last character read
      ch = ' ';
      write(1,&ch,1);

      // Go back one character
      ch = 8;
      write(1,&ch,1);

      // Go back one character
      ch = 8;
      write(1,&ch,1);

      //buffer is reprinted because it has been manipulated  
      int j;
      for (j = cursor_location; j < line_length; j++) {
	  ch = line_buffer[j];
	  write(1, &ch, 1);
      }
      ch = ' ';
      write(1,&ch,1);
      ch = 8;
      write(1,&ch,1);
      int k;
      for (k = cursor_location; k < line_length; k++) {
	  ch = 27;
	  write(1,&ch,1);
	  ch = 91;
	  write(1,&ch,1);
	  ch = 68;
	  write(1,&ch,1);
      }
      // Remove one character from buffer
      //line_length--;
    }
    else if (ch == 1) {
      //home key
      if (line_length == 0 || cursor_location == 0) {
        continue;
      }
      // Go back to front
      int i = 0;
      for (i =0; i < cursor_location; i++) {
        ch = 27;
	write(1,&ch,1);
	ch = 91;
	write(1,&ch,1);
        ch = 68;
	write(1,&ch,1);
      }
      cursor_location = 0;
    }
    else if (ch == 4) {
      //delete key
      if (cursor_location == line_length) {
	continue;
      }
      //erase last char w/ space and go back
      ch = ' ';
      write(1,&ch,1);
      ch = 8;
      write(1,&ch,1);
      // move characters from buffer  after cursor + 1
      int i;
      for (i = cursor_location; i < line_length - 1; i++) {
	  line_buffer[i] = line_buffer[i + 1];
      }
      line_length--;
      //display is reprinted since it was manipulated
      int j;
      for (j = cursor_location; j < line_length; j++) {
	ch = line_buffer[j];
	write(1,&ch,1);
      }
      ch = ' ';
      write(1,&ch,1);
      ch = 8;
      write(1,&ch,1);
      int k;
      for (k = cursor_location; k < line_length; k++) {
	ch = 27;
	write(1,&ch,1);
	ch = 91;
	write(1,&ch,1);
	ch = 68;
	write(1,&ch,1);
      }
    }
    else if (ch == 5) {
      //end key 
      if (line_length == 0 || cursor_location == line_length) {
	continue;
      }
      //move to the end of the line
      int i = 0;
      for (i = cursor_location; i < line_length; i++) {
        ch = 27;
	write(1,&ch,1);
	ch = 91;
	write(1,&ch,1);
	ch = 67;
	write(1,&ch,1);
      }
      cursor_location = line_length;
    }
    else if (ch==10) {
      //If enter is pressed, line must be saved in history
      if (history_length == 0) {
	history = (char **) malloc(sizeof(char*) * 100);
      }
      history[history_length] = (char *)malloc(sizeof(char) * MAX_BUFFER_LINE);
      strncpy(history[history_length], line_buffer, line_length);
      history_length++;
      // Print newline
      write(1,&ch,1);
      break;
    }
    else if (ch==27) {
      // Escape sequence. Read two chars more
      //
      // HINT: Use the program "keyboard-example" to
      // see the ascii code for the different chars typed.
      //
      char ch1; 
      char ch2;
      read(0, &ch1, 1);
      read(0, &ch2, 1);
      if (ch1==91 && ch2==65) {
	// Up arrow. Print next line in history.
        // First check to see if any history exists
	if (history_length == 0) {
	  continue;
        }
	//get rid of oldline w/ backspaces
	int i = 0;
	for (i =0; i < line_length; i++) {
	  ch = 8;
	  write(1,&ch,1);
	}
	// Print spaces on top
	for (i =0; i < line_length; i++) {
	  ch = ' ';
	  write(1,&ch,1);
	}
	// Print backspaces
	for (i =0; i < line_length; i++) {
	  ch = 8;
	  write(1,&ch,1);
	}	
	//get line of history and add
	//it to the buffer
        if (history_index < history_length) {
	  history_index++;
	}
	strcpy(line_buffer, history[history_length - history_index]);
	line_length = strlen(line_buffer);
	//history_index=(history_index+1)%history_length;
	write(1, line_buffer, line_length);
	cursor_location = line_length;
      }
      else if (ch1==91 && ch2==66) {
	// Down arrow. Print prev line in history.
	//get rid of old line w/ backspaces
	int i;
	for (i = 0; i < line_length; i++) {
	  ch = 8;
	  write(1,&ch,1);
	}
	// Print spaces on top
	int j;
	for (j = 0; j < line_length; j++) {
	  ch = ' ';
	  write(1,&ch,1);
	}
	// Print backspaces
	int k;
	for (k = 0; k < line_length; k++) {
	  ch = 8;
	  write(1,&ch,1);
	  }
	// Copy line from history
	if (history_index - 1 > 0) {
	  history_index--;
	  strcpy(line_buffer, history[history_length - history_index]);
	  }
	else {
	  strcpy(line_buffer, "");
	}
	line_length = strlen(line_buffer);
	write(1, line_buffer, line_length);
	cursor_location = line_length;
      }
      else if (ch1==91 && ch2==67) {
	// right arrow
	if (cursor_location == line_length) {
	  continue;
	}
	ch = 27;
	write(1,&ch,1);
	ch = 91;
	write(1,&ch,1);
	ch = 67;
	write(1,&ch,1);
	cursor_location++;
      }
      else if (ch1==91 && ch2==68) {
	// left arrow
	if (cursor_location == 0) {
	  continue;
	}
	ch = 27;
	write(1,&ch,1);
	ch = 91;
	write(1,&ch,1);
	ch = 68;
	write(1,&ch,1);
	cursor_location--;
      }
      else if (ch1==91 && ch2==51) {
	char ch3;
	read(0, &ch3, 1);
	if (ch3 == 126) {
	  // <delete> was typed. remove next char
	  if (cursor_location == line_length) {
	    continue;
	  }
	  ch = ' ';
	  write(1,&ch,1);
	  ch = 8;
	  write(1,&ch,1);
	  int i;
	  for (i = cursor_location; i < line_length - 1; i++) {
	    line_buffer[i] = line_buffer[i + 1];
	  }
	  line_length--;
	  int j;
	  for (j = cursor_location; i < line_length; j++) {
	    ch = line_buffer[j];
	    write(1,&ch,1);
	  }
	  ch = ' ';
	  write(1,&ch,1);
	  ch = 8;
	  write(1,&ch,1);
	  int k;
	  for (k = cursor_location; k < line_length; k++) {
	    ch = 27;
	    write(1,&ch,1);
	    ch = 91;
	    write(1,&ch,1);
	    ch = 68;
	    write(1,&ch,1);
	  }
	}
      }
      else if (ch1==91 && ch2==49) {
	char ch3;
	read(0, &ch3, 1);
	if (ch3 == 126) {
	  // <home> was typed.
	  if (line_length == 0 || cursor_location == 0) {
	    continue;
	  }
	  int i = 0;
	  for (i = 0; i < cursor_location; i++) {
	    ch = 27;
	    write(1,&ch,1);
	    ch = 91;
	    write(1,&ch,1);
	    ch = 68;
	    write(1,&ch,1);
	  }
	  cursor_location = 0;
	}
      }
      else if (ch1==91 && ch2==52) {
	char ch3;
	read(0, &ch3, 1);
	if (ch3 == 126) {
	  // <end> was typed.
	  if (line_length == 0 || cursor_location == line_length) {
	    continue;
	  }
	  int i = 0;
	  for (i = cursor_location; i < line_length; i++) {
	    ch = 27;
	    write(1,&ch,1);
	    ch = 91;
	    write(1,&ch,1);
	    ch = 67;
	    write(1,&ch,1);
	  }
	  cursor_location = line_length;
	}
      }
    }
  }

  // Add eol and null char at the end of string
  line_buffer[line_length]=10;
  line_length++;
  line_buffer[line_length]=0;
  tcsetattr(0, TCSANOW, &restore);
  return line_buffer;
}

