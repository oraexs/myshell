#include "history.h"
#include <stdio.h>
#include <string.h>

/****
Global variables
****/
char shell_history[MAX_HISTORY][COMMAND_LENGHT];
int shell_historycount = 0;
int shell_curridx = 0;

void shell_inithistory() {
    shell_historycount = 0;
    shell_curridx = 0;
}

/***
add_to_history
Desc : Copies the cmd string into the history
***/
void add_to_history(const char* cmd)
{
    if (shell_historycount < MAX_HISTORY)
    {
        strncpy(shell_history[shell_historycount], cmd, COMMAND_LENGHT - 1);
        shell_history[shell_historycount][COMMAND_LENGHT - 1] = '\0'; /* Null termination */
        shell_historycount++;
    }

    shell_curridx = shell_historycount; /* Reset navigation index */
}

/***
get_history_up
Desc : Return history prior to current command
***/
const char* get_history_up()
{
    if (shell_curridx > 0)
    {
        return shell_history[--shell_curridx];
    }

    return NULL;
}

/***
get_history_down
Desc : Return history next to current command
***/
const char* get_history_down()
{
    if (shell_curridx < shell_historycount - 1)
    {
        return shell_history[++shell_curridx];
    }

    return NULL;
}

/***
print_history
Desc : Print all the commands logged into the buffer
***/
void print_history() {
    for (int i = 0; i < shell_historycount; i++)
    {
        printf("Command %d: %s\n", i + 1, shell_history[i]);
    }
}

/***
reset_history_index
Desc : Reset index after clearing input line
***/
void reset_history_index() {
    shell_curridx = shell_historycount;
}
