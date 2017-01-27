#ifndef _COMMON_H
#define _COMMON_H

/* **************************************************** */
/*                Spec-Defined Assumptions              */
/* **************************************************** */
#define MAX_BUFFER    512
#define MAX_TOKENS     16
#define MAX_TOKEN_LEN  32
#define MAX_HIST_ITEMS 10

/* **************************************************** */
/*                    Keystroke Codes                   */
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
/*                     Convenience                      */
/* **************************************************** */
#define TRUE         1
#define FALSE        0

#define SI           STDIN_FILENO
#define SO           STDOUT_FILENO
#define SE           STDERR_FILENO

/* **************************************************** */
/*                   Common functions                   */
/* **************************************************** */
void SayHello (void);                                   /* Prints the hello message. Removed for auto-testing   */
void SayGoodbye (void);                                 /* Prints the exit message                              */
void ErrorBell(void);                                   /* Sound Bell noise                                     */
void PrintNL (void);                                    /* Prints the newline character to STDOUT               */
void PrintBackspace (void);                             /* Prints Backspace character to STDOUT                 */
void ClearCmdLine (char *cmdLine, int *cursorPos);      /* Clear the current cmdLine buffer and STDIN           */
void DisplayPrompt (int *cursorPos);                    /* Displace the main sshell$ prompt                     */
void CompleteCmd (char *cmd, int exitCode);             /* Prints + completed messages to STDOUT                */
void CompleteChain (char *cmd, int *xArray);            /* Prints '+ completed' messages for chains to STDERR   */
void Dup2AndClose(int old, int bnew);                   /* Runs dup2() and close(), performs error checking     */
/* **************************************************** */
/*                    Error functions                   */
/* **************************************************** */
void ThrowError (char *message);                        /* Print error message to STDERR                        */
void NoInputFile (void);                                /* Prints Error: no input file to STDERR                */
void NoOutputFile (void);                               /* Prints Error: no output file to STDERR               */
void InvalidCommand (void);                             /* Prints Error: invalid command line to STDERR         */
void BadInputRedirect (void);                           /* Prints Error: mislocated input redirection           */
void BadOutputRedirect (void);                          /* Prints Error: mislocated output redirection          */
/* **************************************************** */
/*                  Parsing functions                   */
/* **************************************************** */
char CheckCommand(char *cmd, char *isBackground);   	/* Check for invalid placement of special characters    */
char Check4Space(char key);                             /* Checks if character is whitespace or not             */
char Check4Special(char key);                           /* Checks if special character of not                   */
char *RemoveWhitespace(char *string);                   /* Strips trailing and leading whitespace from a string */
char *InsertSpaces(char *cmd);                          /* Ensures ' ' before and after all <>& characters      */
/* ******************************************************/
/*                  Unused functions                    */
/* ******************************************************/
//char *SearchPath(char *prog);	                        /* Returns a pointer to the full path specified binary  */
//ExecProgram(**Cmds[], N, Process *P);                 /* Function to execute a command. Recursive if piped    */
/* **************************************************** */

#endif
