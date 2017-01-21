#ifndef _SSHELL_H
#define _SSHELL_H

/* ************************************ */
/*       Shell Print Characters         */
/* ************************************ */
const char *SHELL_PROMPT = "sshell$ ";
const char *BELL = "\a";
const char *NEWLINE = "\r\n";
const char *EXITLINE = "\r\nBye...\n";
const char *BACKSPACE_CHAR = "\b \b";
/* ************************************ */

/* ************************************ */
/*          Global Variables            */
/* ************************************ */
unsigned int backgroundCmdRunning = 0;

/* ************************************ */
/*               SShell                 */
/* ************************************ */
void InitShell (History *history, int *cursorPos);      /* Initialize the shell and relevant objects            */
void ThrowError (char *message);                        /* Print error message to STDERR                        */
char ChangeDir(char *args[]);                           /* Handlse 'cd' commands                                */
char PrintDir(char *args[]);                            /* Print Working Direcory 				                */
int ExecProgram(char **cmds[], int N, int FD, char BG); /* Execute program commands, recursive if piped       	*/
void CompleteCmd (char *cmd, char exitCode, char newLn);/* Prints + completed messages to STDOUT                */
char CheckCommand(char *cmd, char *isBackground);   	/* Check for invalid placement of special characters    */
char RunCommand (char *cmdLine);                    	/* Wrapper to execute whatever is on the command line   */
char *RemoveWhitespace (char *string);              	/* Stripes trailing or leading whitespace from a string */
char **Cmd2Array (char *cmd);                       	/* Breaks up  a command into an array of arguments      */
char ***Pipes2Array (char *cmd);                    	/* Breaks up command into arrays of piped arguments     */     
/* ************************************ */

#endif
