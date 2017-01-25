#ifndef _SSHELL_H
#define _SSHELL_H

#include "process.h"                                    /* Functions for tracking background processes          */
#define xStat(status) WEXITSTATUS(status)               /* Rename WEXITSTATUS                                   */

/* **************************************************** */
/*                       SShell                         */
/* **************************************************** */
void InitShell (History *history, int *cursorPos);      /* Initialize the shell and relevant objects            */
char *SearchPath(char *prog);	                        /* Returns a pointer to the full path specified binary  */
char ChangeDir(char *args[]);                           /* Handles 'cd' commands                                */
char PrintWDir(char *args[]);                           /* Handles 'pwd' commands                               */
char RunCommand (char *cmdLine);                    	/* Wrapper to execute whatever is on the command line   */
char CheckCommand(char *cmd, char *isBackground);   	/* Check for invalid placement of special characters    */
void ExecProgram(char **cmds[], int N, Process *P);     /* Execute program commands, recursive if piped       	*/
void Wait4Me(Process *P);                               /* Executes blocking or non-blocking wait               */
char **Cmd2Array (char *cmd);                       	/* Breaks up  a command into an array of arguments      */
char ***Pipes2Array (char *cmd);                    	/* Breaks up command into arrays of piped arguments     */
/* **************************************************** */

#endif
