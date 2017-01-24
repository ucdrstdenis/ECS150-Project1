#ifndef _COMMON_H
#define _COMMON_H

/* **************************************************** */
/*              Spec-Defined Assumptions                */
/* **************************************************** */
#define MAX_BUFFER    512
#define MAX_TOKENS     16
#define MAX_TOKEN_LEN  32
#define MAX_HIST_ITEMS 10

#define TRUE            1
#define FALSE           0

#define SI              STDIN_FILENO
#define SO              STDOUT_FILENO
#define SE              STDERR_FILENO

/* **************************************************** */
/*                  Keystroke Codes                     */
/* **************************************************** */
#define CTRL_D       0x04
#define TAB          0x09
#define RETURN       0x0A
#define BACKSPACE    0x7F
#define ESCAPE       0x1B
#define ARROW        0x5B
#define UP           0x41
#define DOWN         0x42
#define RIGHT        0x43
#define LEFT         0x44

/* **************************************************** */
/*                      Common                          */
/* **************************************************** */
void ErrorBell(void);                                   /* Sound Bell noise                                     */
void ClearCmdLine (char *cmdLine, int *cursorPos);      /* Clear the current cmdLine buffer and STDIN           */
void DisplayPrompt (int *cursorPos);                    /* Displace the main sshell$ prompt                     */
void PrintBackspace (void);				/* Prints Backspace character to STDOUT 		*/
void PrintNL (void);					/* Prints the newline character to STDOUT 		*/
void ThrowError (char *message);                        /* Print error message to STDERR                        */
void SayGoodbye (void);                                 /* Prints the exit message                              */
void CompleteCmd (char *cmd, int exitCode);             /* Prints + completed messages to STDOUT                */
char Check4Space(char key);                    		/* Checks if character is whitespace or not             */
char Check4Special(char key);                           /* Checks if special character of not                   */
char *RemoveWhitespace(char *string);                   /* Strips trailing and leading whitespace from a string */
char *InsertSpaces(char *cmd);                          /* Inserts ' ' before and after all <>& characters      */
/* **************************************************** */

#endif
