#ifndef _SSHELL_H
#define _SSHELL_H

#include "process.h"                                    /* Structures and methods for tracking processes        */
/* **************************************************** */
/*                     Convenience                      */
/* **************************************************** */
#define xStat(status) WEXITSTATUS(status)               /* Rename WEXITSTATUS                                   */
#define WMODE (O_CREAT | O_TRUNC | O_WRONLY)            /* Create if doesn't exist, clear file, write only      */
#define RMODE (O_RDONLY)				/* Read only mode 					*/

/* **************************************************** */
/*                       SShell                         */
/* **************************************************** */
void InitShell (History *history, int *cursorPos);      /* Initialize the shell and relevant objects            */
char ChangeDir(char *args[]);                           /* Handles 'cd' commands                                */
char PrintWDir(char *args[]);                           /* Handles 'pwd' commands                               */
char RunCommand (char *cmdLine);                    	/* Wrapper to execute whatever is on the command line   */
char ExecProgram(char **cmds[], Process *P);            /* Execute program commands, inner-looped when piped    */
void ForkMe(char *cmds[], Process *Me);                 /* Forks a process. Child executes, parent waits.       */
void RunMe(char *cmds[], Process *Me);                  /* Execute a single execvp call post fork()             */
void Wait4Me(Process *Me);                              /* Executes blocking or non-blocking wait               */
int OpenMe(const char *Me, const int Mode);		/* Calls fopen(), checks for errors 			*/
char Redirect(char *args[], int *fd);                   /* Sets up input/output file descriptors                */
char CheckRedirect(char **cmds[], Process *P, int N);   /* Sets up redirects and checks if piped                */
char **Cmd2Array (char *cmd);                       	/* Breaks up  a command into an array of arguments      */
char ***Pipes2Arrays (char *cmd, char *numPipes);       /* Breaks up command into arrays of piped arguments     */
/* **************************************************** */

#endif
