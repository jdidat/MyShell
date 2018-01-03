#ifndef command_h
#define command_h

#include "simpleCommand.hh"

// Command Data Structure

struct Command {
  int _numOfAvailableSimpleCommands;
  int _numOfSimpleCommands;
  SimpleCommand ** _simpleCommands;
  char * _outFile;
  char * _inFile;
  char * _errFile;
  int _background;
  int _append;
  int _inCount;
  int _outCount;

  void prompt();
  void print();
  void execute();
  void setenvExe(int i);
  void unsetenvExe(int i);
  void cdExe(int i);
  void sourceExe(int i);
  void printenvExe();
  void clear();

  Command();
  void insertSimpleCommand( SimpleCommand * simpleCommand );

  static Command _currentCommand;
  static SimpleCommand *_currentSimpleCommand;
};

#endif
