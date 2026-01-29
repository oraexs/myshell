#ifndef HISTORY_H
#define HISTORY_H

/***
DEFINE'S
***/
#define MAX_HISTORY    100
#define COMMAND_LENGHT 1024

/***
FUNCTION DECLARATIONS
****/
void shell_inithistory();
void add_to_history(const char *cmd);
void print_history();
const char* get_history_up();
const char* get_history_down();
void reset_history_index();

#endif