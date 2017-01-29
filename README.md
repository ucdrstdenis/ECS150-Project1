# SShell Synopsys #
A simple shell written in c.

# SShell Rundown #
A basic overview of how this program works. 

`main()` located at the bottom of `sshell.c`, does 3 things:
- Initialize the shell with `InitShell()`.
- Process the keystroke.
- Handle exiting the application.

`InitShell()` does 4 things:
- Alloc/init the local history structure `*History`.
- Alloc/init the global process structure `*ProcessList`.
- Define the `ChildSignalHandler()` for the `SIGCHLD` signal.
- Set the terminal to non-cannonical mode using Joël Porquet's noncanmode.c.

Keystroke processing is very straight forward:
- When a user presses a key, the keystroke is written to `STDOUT` and copied to a local buffer. 
- UP/DOWN arrows call `DisplayNextEntry()` and `DisplayLastEntry()` from the history API.
- TAB, LEFT, and RIGHT arrow keys call the `ErrorBell()` function to sound an audible bell.

When a user presses the `RETURN` key, 3 things happen:
- The contents of the command line are added to the shell's history.
- The command is processed with the `RunCommand()` wrapper routine.
- The process list is checked for any processes that may have completed, and if they have, it prints thier `+ completed ` message to `STDERR` and removes them from the list.

`RunCommand()` routine does 3 things:
- Performs initial layer of command checking.
- Parses the command into a   ***char array, based on the pipe `|` characters.  For example, the command `ls -la|grep common> outfile` would be transformed into `{ {"ls", "-la", NULL}, {"grep", "common", ">", "outfile", NULL}, NULL}`. This is done within `Pipes2Arrays()` and `Cmd2Array()` routines.
- The command is checked for built-in calls which are `exit` `cd` and `pwd`, and calls their subroutines. If the command is not built in, it calls `ExecProgram()`.

`ExecProgram()` does several things:
- If the commands are piped, `ExecProgram()` uses a while loop to chain the commands together. 
- It also checks the command arrays for I/O redirects with a call to `CheckRedirects()`, which calls `SetupRedirects()` to get the I/O file descriptors, and performs second and third level error checking. This includes checking the output file descriptor against any pipes the output may need to be sent to.

The `*Process` structure is the main object that gets passed around from function to function.
- Although the process structures' main objective is to handle background routines, it evolved into a convenient mechanism for handling program execution, file redirecting, and command pipelining. 
- This is because the I/O file descriptors, background flags, command contents, and more can all be stored in the `*Process` object, whose main constructor is `AddProcess()`.
- When processes are chained together, `AddProcessAsChild()` is also used. This doesn't imply the structure is a child in the true sense, it's just a convenient way to iterate through the process list and string together the exit codes from piped commands.
- When a process is run, it calls `ForkMe()`, which forks the command into a child process that calls `RunMe()` for `execvp()`, while the parent waits with `Wait4Me()`. 
- If the process is marked for background execution `Wait4Me()` uses a nonblocking `waitpid()` with `WNOHANG`. The `ChildSignalHandler()` routine is entered when the background process completes, and calls `MarkProcessDone()` to mark the process in the list as completed.

Finally, we are back to the last step from when the RETURN key was pressed. 

The `*Process List` is checked for completed commands, + completed messages are printed, and the whole thing repeats.

If the 'exit' command or CTRL+D is pressed, the main routine checks the process list to make sure there are no outstanding processes. 

# Header Files (API) #
``` c
/* **************************************************** */
/*                        sshell.h                      */
/* **************************************************** */
void InitShell (History *history, int *cursorPos);      /* Initialize the shell and relevant objects            */
char ChangeDir(char *args[]);                           /* Handles 'cd' commands                                */
char PrintWDir(char *args[]);                           /* Handles 'pwd' commands                               */
char RunCommand (char *cmdLine);                    	/* Wrapper to execute whatever is on the command line   */
char ExecProgram(char **cmds[], Process *P);            /* Execute program commands, inner-looped when piped    */
void ForkMe(char *cmds[], Process *Me);                 /* Forks a process. Child executes, parent waits.       */
void RunMe(char *cmds[], Process *Me);                  /* Execute a single execvp call post fork()             */
void Wait4Me(Process *Me);                              /* Executes blocking or non-blocking wait               */
char Redirect(char *args[], int *fd);                   /* Sets up input/output file descriptors                */
char CheckRedirect(char **cmds[], Process *P, int N);   /* Sets up redirects and checks if piped                */
char **Cmd2Array (char *cmd);                       	/* Breaks up  a command into an array of arguments      */
char ***Pipes2Array (char *cmd, char *numPipes);        /* Breaks up command into arrays of piped arguments     */
/* **************************************************** */

/* **************************************************** */
/*                      history.h                       */
/* **************************************************** */
/*      See file for History and Entry structures       */
/* **************************************************** */
void AddHistory(History *history, char *cmdLine, int cmdLen); 	        /* Adds a new entry to the history list                */
void DisplayNextEntry(History *history, char *cmdLine, int *cursorPos); /* Displays next history entry in the command line     */
void DisplayPrevEntry(History *history, char *cmdLine, int *cursorPos); /* Displays previous history entry in the command line */
void RemoveLastEntry(History *history);                                 /* Removes the entry at the bottom of the history list */
/* **************************************************** */

/* **************************************************** */
/*                       process.h                      */
/* **************************************************** */
/*    See file for Process and ProcessList structs      */
/* **************************************************** */
void CompleteChain (Process *P, int *xArray);                                                     /* Prints '+ completed' messages for chains       */
int *GetChainStatus(Process *P);                                                                  /* Get the exit status codes from piped commands  */
Process *CopyDelete(Process *To, Process *From);                                                  /* Copy a process to another process, then delete */
void CheckCompletedProcesses(ProcessList *pList);                                                 /* Check if any processes have completed          */
char MarkProcessDone(ProcessList *pList, pid_t PID, int status);                                  /* Mark process with matching PID as completed    */
Process *AddProcessAsChild(ProcessList *pList, Process *P, pid_t cPID, char *cmd);                /* Create a new process marked as child of parent */
Process *AddProcess(ProcessList *pList, pid_t PID, char *cmd, char nPipes, char isBG, int *fd);   /* Adds a process struct to the list of processes */ 
/* **************************************************** */

/* **************************************************** */
/*                       common.h                       */
/* **************************************************** */
void SayHello (void);                                   /* Prints the hello message. Removed for auto-testing   */
void SayGoodbye (void);                                 /* Prints the exit message                              */
void ErrorBell(void);                                   /* Sound Bell noise                                     */
void PrintNL (void);                                    /* Prints the newline character to STDOUT               */
void PrintBackspace (void);                             /* Prints Backspace character to STDOUT                 */
void ClearCmdLine (char *cmdLine, int *cursorPos);      /* Clear the current cmdLine buffer and STDIN           */
void DisplayPrompt (int *cursorPos);                    /* Displace the main sshell$ prompt                     */
void CompleteCmd (char *cmd, int exitCode);             /* Prints + completed messages to STDOUT                */
void Dup2AndClose(int old, int bnew);                   /* Runs dup2() and close(), performs error checking     */
/* **************************************************** */
/*              common.h - Error Functions              */
/* **************************************************** */
void ThrowError (char *message);                        /* Print error message to STDERR                        */
void NoInputFile (void);                                /* Prints Error: no input file to STDERR                */
void NoOutputFile (void);                               /* Prints Error: no output file to STDERR               */
void InvalidCommand (void);                             /* Prints Error: invalid command line to STDERR         */
void BadInputRedirect (void);                           /* Prints Error: mislocated input redirection           */
void BadOutputRedirect (void);                          /* Prints Error: mislocated output redirection          */
/* **************************************************** */
/*             common.h - Parsing Functions             */
/* **************************************************** */
char CheckCommand(char *cmd, char *isBackground);   	/* Check for invalid placement of special characters    */
char Check4Space(char key);                             /* Checks if character is whitespace or not             */
char Check4Special(char key);                           /* Checks if special character <>& or not               */
char *RemoveWhitespace(char *string);                   /* Strips trailing and leading whitespace from a string */
char *InsertSpaces(char *cmd);                          /* Ensures ' ' before and after all <>& characters      */
/* ******************************************************/
/*             common.h - Unused functions              */
/* ******************************************************/
//char *SearchPath(char *prog);	                        /* Returns a pointer to the full path specified binary  */
//ExecProgram(**Cmds[], N, Process *P);                 /* Function to execute a command. Recursive if piped    */
/* **************************************************** */

/* **************************************************** */
/* noncanmode.h - based on Joël Porquet's noncanmode.c  */
/* **************************************************** */
``````

# Build / Run #
This program was compiled using the linux gcc 6.3.1 compiler.

To build, cd to the folder in a terminal.

Type `make` and the `Makefile` does the rest.

After building, the shell can be run by typing `./sshell`

# Testing #
Testing was performed with the `sshell_test.sh` script provided by John Chan. 

# Contributors #
Robert St. Denis

Brian Labar

Riley Mikkelsen

## References ##
1. [Basic linked list history](http://stackoverflow.com/questions/20588556/linked-list-implementation-to-store-command-history-in-my-shell)

2. [Makefile Reference 1](http://stackoverflow.com/questions/1484817/how-do-i-make-a-simple-makefile-for-gcc-on-linux)

3. [Makefile Reference 2](http://mrbook.org/blog/tutorials/make/)

4. [GitHub Markdown Cheatsheet](https://github.com/adam-p/markdown-here/wiki/Markdown-Cheatsheet)

5. [Readme.md Example] (https://gist.github.com/jxson/1784669)

## Unused References ##
1. [Example of recursive piping](https://gist.github.com/zed/7835043) - See unused `ExecProgram()` at the bottom of  `common.c`.
