#ifndef _SSHELL_H
#define _SSHELL_H

#include "process.h"                                    /* Functions for tracking background processes          */
#define xStat(status) WEXITSTATUS(status)               /* Rename WEXITSTATUS                                   */

/* **************************************************** */
/*                       SShell                         */
/* **************************************************** */
void InitShell (History *history, int *cursorPos);      /* Initialize the shell and relevant objects            */
char ChangeDir(char *args[]);                           /* Handles 'cd' commands                                */
char PrintWDir(char *args[]);                           /* Handles 'pwd' commands                               */
char RunCommand (char *cmdLine);                    	/* Wrapper to execute whatever is on the command line   */
void ExecProgram(char **cmds[], int N, Process *P);     /* Execute program commands, recursive if piped       	*/
void Wait4Me(Process *Me);                              /* Executes blocking or non-blocking wait               */
char **Cmd2Array (char *cmd);                       	/* Breaks up  a command into an array of arguments      */
char ***Pipes2Array (char *cmd, char *numPipes);        /* Breaks up command into arrays of piped arguments     */
/* **************************************************** */

#endif
