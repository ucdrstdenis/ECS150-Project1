#ifndef _SSHELL_H
#define _SSHELL_H

#define xStat(status) WEXITSTATUS(status)

/* **************************************************** */
/*                       SShell                         */
/* **************************************************** */
void InitShell (History *history, int *cursorPos);      /* Initialize the shell and relevant objects            */
char *SearchPath(char *prog);	                        /* Returns a pointer to the full path specified binary  */
char ChangeDir(char *args[]);                           /* Handles 'cd' commands                                */
char PrintWDir(char *args[]);                           /* Handles 'pwd' commands                               */
char RunCommand (char *cmdLine);                    	/* Wrapper to execute whatever is on the command line   */
char CheckCommand(char *cmd, char *isBackground);   	/* Check for invalid placement of special characters    */
int  ExecProgram(char **cmds[], int N, int FD, char BG);/* Execute program commands, recursive if piped       	*/
char **Cmd2Array (char *cmd);                       	/* Breaks up  a command into an array of arguments      */
char ***Pipes2Array (char *cmd);                    	/* Breaks up command into arrays of piped arguments     */
/* **************************************************** */

#endif
