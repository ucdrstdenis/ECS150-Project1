#ifndef _SSHELL_H
#define _SSHELL_H

/* **************************************************** */
/*               Shell Print Characters                 */
/* **************************************************** */
const char *SHELL_PROMPT = "sshell$ ";
const char *BELL = "\a";
const char *NEWLINE = "\r\n";
const char *EXITLINE = "Bye...\n";
const char *BACKSPACE_CHAR = "\b \b";
/* **************************************************** */

/* **************************************************** */
/*               Shell Structures                       */
/* **************************************************** */
typedef struct Process {                                /* Process Node                             */
    pid_t PID;	                                        /* PID of command that was run              */
    char *cmd;                                          /* command that was executed                */
    char running;                                       /* 1 if running, 0 if complete              */
    char status;                                        /* Completion Status when process completed */
    struct Process *next;                               /* points to next process in list           */
    struct Process *prev;                               /* points to  previous process in the list  */
} Process;

typedef struct BackgroundProcessList {                  /* Maintains list of background processes   */
    unsigned int count;                                 /* Number of outstanding processes          */
    Process *top;                                       /* Process List                             */
} BackgroundProcessList;
/* **************************************************** */

/* **************************************************** */
/*                Global Structures                     */
/* **************************************************** */
BackgroundProcessList *processList; 

/* **************************************************** */
/*                       SShell                         */
/* **************************************************** */
void InitShell (History *history, int *cursorPos);      /* Initialize the shell and relevant objects            */
char ChangeDir(char *args[]);                           /* Handlse 'cd' commands                                */
char PrintWDir(char *args[]);                           /* Print Working Direcory (handles pwd)                 */
char *SearchPath(char *prog);	                        /* Returns a pointer to the full path specified binary  */
int  ExecProgram(char **cmds[], int N, int FD, char BG);/* Execute program commands, recursive if piped       	*/
void CompleteCmd (char *cmd, char exitCode);            /* Prints + completed messages to STDOUT                */
char CheckCommand(char *cmd, char *isBackground);   	/* Check for invalid placement of special characters    */
char RunCommand (char *cmdLine);                    	/* Wrapper to execute whatever is on the command line   */
char *RemoveWhitespace (char *string);              	/* Stripes trailing or leading whitespace from a string */
char **Cmd2Array (char *cmd);                       	/* Breaks up  a command into an array of arguments      */
char ***Pipes2Array (char *cmd);                    	/* Breaks up command into arrays of piped arguments     */     

void MarkProcessDone(BackgroundProcessList *pList, pid_t PID, int status);   /* Mark process with matching PID as completed          */
void CheckCompletedProcesses(BackgroundProcessList *pList);                  /* Check if any processes have completed                */
void AddProcess(BackgroundProcessList *pList, pid_t PID, char *cmd);         /* Add a process to the list of background processes    */
/* **************************************************** */

#endif
