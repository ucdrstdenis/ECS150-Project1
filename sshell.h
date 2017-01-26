#ifndef _SSHELL_H
#define _SSHELL_H

#include "process.h"                                    /* Functions for tracking background processes          */
#define xStat(status) WEXITSTATUS(status)               /* Rename WEXITSTATUS                                   */
#define WMODE O_CREAT | O_TRUNC | O_WRONLY              /* Create if doesn't exist, clear file, write only      */

/* **************************************************** */
/*                       SShell                         */
/* **************************************************** */
void InitShell (History *history, int *cursorPos);      /* Initialize the shell and relevant objects            */
char ChangeDir(char *args[]);                           /* Handles 'cd' commands                                */
char PrintWDir(char *args[]);                           /* Handles 'pwd' commands                               */
char RunCommand (char *cmdLine);                    	/* Wrapper to execute whatever is on the command line   */
char ExecProgram(char **cmds[], Process *P);            /* Execute program commands, looped if piped            */
void Wait4Me(Process *Me);                              /* Executes blocking or non-blocking wait               */
void ForkMe(char *cmds[], Process *Me);                 /* Forks a process. Child executes, parent waits        */
char Redirect(char *args[], int *fd);                   /* Sets up input/output file descriptors                */
char CheckRedirect(char **cmds[], Process *P, int N);   /* Sets up redirects and checks if piped                */
char **Cmd2Array (char *cmd);                       	/* Breaks up  a command into an array of arguments      */
char ***Pipes2Array (char *cmd, char *numPipes);        /* Breaks up command into arrays of piped arguments     */
/* **************************************************** */

#endif
