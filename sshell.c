#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>

/* **************************************************** */
/*              User - defined .h files                 */
/* **************************************************** */
#include "common.h"                                     /* Keystrokes, parsing, & common functions        */
#include "history.h"                                    /* History structures and related functions       */
#include "noncanmode.h"                                 /* Modified version of the file provided by Joel  */
#include "sshell.h"                                     /* Function prototypes for sshell.c functions     */
/* **************************************************** */

/* **************************************************** */
/* Interrupt / Signal handler for SIGCHDL               */
/* **************************************************** */
void ChildSignalHandler(int signum)
{
    pid_t PID;
    int status = 0;
    //char debug[MAX_BUFFER];
    while ((PID = waitpid(-1, &status, WNOHANG)) > 0){    /* Allow multiple child processes to terminate if necessary */
    //    sprintf(debug, "DEBUG: PID returned=%d, status returned=%d",xStat(status));
    //    ThrowError(debug);
        MarkProcessDone(processList, PID, xStat(status));/* Mark the process as completed                             */
    }
    //sprintf(debug, "DEBUG: status returned=%d",xStat(status));
    //ThrowError(debug);
}
/* **************************************************** */

/* **************************************************** */
/* Executes blocking or nonblocking wait()              */
/* **************************************************** */
void Wait4Me(Process *Me)
{
    int status;
    if (Me->isBG) {                                     /* If it's to be run in background        */
        waitpid(Me->PID, &status, WNOHANG);             /* Use non-blocking call to waitpid       */
        Me->status = xStat(status);                     /* Set the temporary status               */
    } else {                                            /* Otherwise                              */
        waitpid(Me->PID, &status, 0);                   /* wait for child to exit                 */
        MarkProcessDone(processList, Me->PID, xStat(status));
    }
}
/* **************************************************** */

/* **************************************************** */
/* Change Directory Command  (handles cd)               */
/* **************************************************** */
char ChangeDir(char *args[])
{
    if ((args[0] == NULL) || (chdir(args[0]) == -1)) {  /* If no dir specified or chdir() fails */
        ThrowError("Error: no such directory");         /* Print message on STDERR              */
        return 1;                                       /* Return error code 1                  */
    }
    return 0;                                           /* Otherwise return error code 0        */
}
/* **************************************************** */

/* **************************************************** */
/* Print Working Directory  (handles pwd)               */
/* **************************************************** */
char PrintWDir(char *args[])
{
    char workingDir[MAX_BUFFER];
    int fd;                                             /* File descriptor */
    getcwd(workingDir, MAX_BUFFER);                     /* Write working directory into workingDir */
    sprintf(workingDir, "%s\n", workingDir);            /* Add a new line character                */
    
    // DEBUGGING
    char debug[MAX_BUFFER];
    int i = 0;
    while(args[i] != NULL) {
        sprintf(debug, "Args[%d] = %s", i, args[i]);
        ThrowError(debug);
        i++;
    }
    
    if (args[1]==NULL) {                                /* If the first argument == >, set out fd   */
        write(STDOUT_FILENO, workingDir, strlen(workingDir));
        return 0;
    }
    if (!strcmp(args[1], ">") && args[2] != NULL) {
        fd = open(args[2], O_CREAT | O_TRUNC | O_WRONLY, 0600);
        write(fd, workingDir, strlen(workingDir));
        return 0;
    } else {
        ThrowError("Error: invalid argument to pwd");
        return 1;
    }
}
/* **************************************************** */

/* **************************************************** */
/* Breaks up a command into a NULL terminated           */ 
/* dynamically allocated array.                         */
/*                                                      */
/*"ls -l -a" -> {"ls","-l","-a", NULL};                 */
/* **************************************************** */
char **Cmd2Array(char *cmd)
{
    char **args =  (char **) malloc(MAX_TOKENS * sizeof(char*));
    unsigned int i = 0;
    cmd = RemoveWhitespace(cmd);                        /* Remove leading/trailing whitespace                   */
    char *space = strchr(cmd, ' ');                     /* space points to the first occurance of ' ' in cmd    */
    
    while(space != NULL) {                              /* Repeat until no more ' ' found                       */
        *space = '\0';                                  /* Replace ' ' with '\0'                                */
        args[i] = (char *) malloc(strlen(cmd)+1);       /* Allocate space for the cmd string                    */
        args[i++] = cmd;                                /* Put null terminated string into the arguments array  */
        cmd = RemoveWhitespace(space + 1);              /* Remove leading/trailing whitespace in remaining cmd  */
        space = strchr(cmd, ' ');                       /* space points to the first place ' ' occurs in cmd    */
    }
    
    if (*cmd != '\0') {                                 /* If no more spaces, but still not end of cmd          */
        args[i] = (char *)malloc(strlen(cmd)+1);        /* Allocate space for the remaining portion of cmd      */
        args[i++] = cmd;
    }

    args[i] = (char *) malloc(sizeof(char*));           /* Allocate space for the NULL character (Not needed?)  */
    args[i] = NULL;
    return args;
}
/* **************************************************** */

/* **************************************************** */
/* Breaks up  a command into dynamically allocated      */
/* 2D array of command pointers                         */
/*                     Example  1                       */
/* "ls -la|grep filename" -> {args0, args1, NULL}       */
/* where args0 = {"ls", "-la", NULL}                    */
/* where args1 = {"grep", "filename", NUll}             */
/*                     Example  2                       */
/* "ls -la" -> {args0, NULL}                            */
/* where args0 = {"ls", "-la", NULL}                    */
/* **************************************************** */
char ***Pipes2Arrays(char *cmd)
{
    unsigned int i = 0;
    char ***pipes =  (char ***) malloc(MAX_TOKENS * sizeof(char**));
    cmd = RemoveWhitespace(cmd);                        /* Remove leading/trailing whitespace                   */
    char *bar = strchr(cmd, '|');                       /* bar points to the first occurance of '|' in cmd      */
    
    while(bar != NULL) {                                /* Repeat until no more '|' found or max tokens reached */
        *bar = '\0';                                    /* Replace '|' with '\0                                 */
        pipes[i++] = Cmd2Array(cmd);                    /* Put the cmd array into the pipes array               */
        cmd = RemoveWhitespace(bar+1);                  /* Remove leading/trailing whitespace for remaining part the command*/
        bar = strchr(cmd, '|');                         /* bar points to the first occurance of '|' in cmd      */
    }      
    
    if (*cmd != '\0')                                   /* If there are still characters in cmd                 */
        pipes[i++] = Cmd2Array(cmd);                    /* Add them to the array                                */
         
    pipes[i] = NULL;                                    /* Set the last entry to be NULL                        */
    return pipes;                                       /* Return the pointer                                   */
}
/* **************************************************** */

/* **************************************************** */
/* Function to execute program commands                 */
/* If function is piped, execProgram() is recursive     */
/* **************************************************** */
void ExecProgram(char **cmds[], int N, Process *P)
{
    if (cmds[N+1] == NULL) {                            /* If there's only 1 command in the array   */
        Dup2AndClose(P->fdIn, SI);                      /* Read from fdIn                           */
        execvp(cmds[N][0], cmds[N]);                    /* Execute command                          */
        perror("execvp");                               /* Coming back here is an error             */
        exit(EXIT_FAILURE);                             /* Exit failure                             */
        
    } else {
        int fdOut[2];                                   /* Create file descriptors                  */
        Process *cP;                                    /* Pointer to new child process             */
        pipe(fdOut);                                    /* Create pipe                              */
        
        cP = AddProcessAsChild(processList, P->PID, fork(), "\0", P->isBG, fdOut[0]);
        switch(cP->PID) {                               /* fork the process                         */
            case -1:                                    /* If fork fails                            */
                perror("fork");                         /* Report the error                         */
                exit(EXIT_FAILURE);                     /* Exit with failure                        */
                
            case 0:                                     /* Child Process, writes to the pipe        */
                close(fdOut[0]);                        /* Don't need to read from pipe             */
                Dup2AndClose(P->fdIn, SI);              /* Link Input file descriptor to the pipe   */
                Dup2AndClose(fdOut[1], SO);             /* Link output file descripter to STDOUT    */
                //cmds[N][0] = SearchPath(cmds[N][0]);  /* Replace with full PATH to binary name    */
                execvp(cmds[N][0], cmds[N]);            /* Execute the command                      */
                perror("execvp");                       /* Coming back here is an error             */
                exit(EXIT_FAILURE);                     /* Exit failure                             */
                
            default:                                    /* Parent Process, reads from the  pipe     */
                close(fdOut[1]);                        /* Don't need to write to pipe              */
                close(P->fdIn);                         /* Close existing input file descriptor     */
                Wait4Me(cP);                            /* Blocking or non-blocking wait            */
                ExecProgram(cmds, N+1, cP);             /* Execute the first command in the array   */
        }
    }
}
/* **************************************************** */

/* **************************************************** */
/* Wrapper to execute anything sent from command line   */
/* **************************************************** */
char RunCommand(char *cmdLine)
{
    char ***Cmds = (char ***) malloc(MAX_TOKENS * sizeof(char**));
    Process *P;                                         /* New Process Pointer                   */
    char isBg = 0;                                      /* Flag for background commands          */
    char *cmdCopy = (char *) malloc(strlen(cmdLine)+1); /* Holds copy of the command line        */
    
    strcpy(cmdCopy, cmdLine);                           /* Make the copy                         */
    cmdLine = InsertSpaces(cmdLine);                    /* Add spaces before and after <>&       */
    cmdLine = RemoveWhitespace(cmdLine);                /* Remove leading/trailing whitespace    */
    
    if (CheckCommand(cmdLine, &isBg)) return 0;         /* Check for invalid character placement */
    Cmds = Pipes2Arrays(cmdLine);                       /* Breakup command into  *array[][]      */
    
    if (Cmds[0] == NULL)              return 0;         /* Return if nothing in command line     */
    if (!strcmp(Cmds[0][0], "exit"))  return 1;         /* 'exit' forces main loop to break      */
    
    if (!strcmp(Cmds[0][0], "cd"))                      /* If first command = "cd"               */
        CompleteCmd(cmdCopy, ChangeDir(&Cmds[0][1]));   /* cd and print + completed message      */
    
    else if (!strcmp(Cmds[0][0], "pwd"))                /* If first command = "pwd"              */
        CompleteCmd(cmdCopy, PrintWDir(Cmds[0]));       /* pwd & print + completed message       */
    
    else {                                              /* Otherwise, try executing the program  */
        P = AddProcess(processList, fork(), cmdCopy, isBg, SI);
        switch(P->PID) {
            case -1:                                    /* fork() failed                          */
                perror("fork");                         /* Report the error                       */
                return EXIT_FAILURE;                    /* Force main loop to break               */
            case 0:                                     /* Child Process                          */
                ExecProgram((char ***)Cmds, 0, P);      /* Execute recursive piping               */
            default:                                    /* Parent Process (PID > 0)               */
                Wait4Me(P);                             /* Wait w/ blocking or non-blcoking       */
        }
    }
    return 0;                                           /* Continue main loop                     */
}
/* **************************************************** */

/* **************************************************** */
/* Shell Initialization function                        */
/* **************************************************** */
void InitShell(History *history, int *cursorPos)
{
    /* Initialize the global process list */
    processList->count = 0;                             /* Number of outstanding processes = 0              */
    processList->top = NULL;                            /* No outstanding processes yet                     */
    
    /* Initialize history structure */
    history->count = 0;                                 /* Number of history items = 0                      */
    history->traversed = 0;                             /* Traversed history items = 0                      */
    history->top = NULL;                                /* No history entries yet                           */
    history->current = NULL;                            /* Not currently viewing any entry                  */
    
    /* Setup SIGCHLD signal handler */
    struct sigaction act;                               /* Sigaction struct for SIGCHLD signal handlers     */
    act.sa_flags = SA_RESTART | SA_NOCLDSTOP;           /* Avoid EINTR | Only call when process terminates  */
    act.sa_handler = ChildSignalHandler;                /* Define signal handler                            */
    sigemptyset(&act.sa_mask);                          /* From noncanmode.c                                */

    if (sigaction(SIGCHLD, &act, NULL)) {               /* Call sigaction, check for error                  */
        perror("sigaction");                            /* If theres an error, throw it                     */
        exit(1);                                        /* Terminate the program                            */
    }
    
    SetNonCanMode();                                    /* Switch to non-canonical terminal mode            */
    DisplayPrompt(cursorPos);                           /* Print the prompt and clear the cursor position   */
}
/* **************************************************** */

/* **************************************************** */
/*                        MAIN                          */
/* **************************************************** */
int main(int argc, char *argv[], char *envp[])
{
    int cursorPos = 0;
    char keystroke, cmdLine[MAX_BUFFER];
    unsigned char tryExit = 0, keepRunning = 1;

    processList = malloc(sizeof(ProcessList)); 		     /* Global list of processes being tracked */
    History *history = (History*)malloc(sizeof(History));/* Local list of history entries */
    InitShell(history, &cursorPos);                      /* Initialize the shell */

mainLoop:                                                /* Shell main loop label */
    while (keepRunning) {                                /* Main Loop */
        keystroke = GetChar();
                                                         /* @TODO put switch statement into keystrokeHandler() */
        switch(keystroke) {                              /* Process the keytroke */
            case CTRL_D:                                 /* CTRL + D */
                PrintNL();
                keepRunning = 0;
                break;
           
            case TAB:                                    /* TAB KEY */
                ErrorBell();
                break;
            
            case BACKSPACE:                              /* BACKSPACE */
                if (cursorPos) {
                    PrintBackspace();
                    cursorPos -= 1;
                }
                else
                    ErrorBell();
                break;
            
            case ESCAPE:                                 /* ARROW KEYS */
                if (GetChar() == ARROW)
                    switch(GetChar()) {
                        case UP:                         /* UP */
                            DisplayNextEntry(history, cmdLine, &cursorPos);
                            break;
                        case DOWN:                       /* DOWN */
                            DisplayPrevEntry(history, cmdLine, &cursorPos);
                            break;
                        case LEFT:                       /* LEFT */
                            ErrorBell();
                            break;
                        case RIGHT:                      /* RIGHT */
                            ErrorBell();
                            break;
                    }
                break;

            case RETURN:                                 /* ENTER KEY */
                cmdLine[cursorPos] = '\0';
                PrintNL();
                AddHistory(history, cmdLine, cursorPos);
                if((tryExit = RunCommand(cmdLine)))
                    keepRunning = 0;                     /* Stop the main loop if 'exit' received */
                else                                    
                    CheckCompletedProcesses(processList);          
                    DisplayPrompt(&cursorPos);       
                break;
        
            default:                                     /* ANY OTHER KEY */
                if (cursorPos < MAX_BUFFER) {            /* Make sure there's room in the buffer */
                    write(STDOUT_FILENO, &keystroke, 1);
                    cmdLine[cursorPos++] = keystroke;
                } else
                    ErrorBell();
        }                                                /* End switch statement */
    }                                                    /* End Main Loop */

    if (processList->count) {                            /* If background commands are still running */
        ThrowError("Error: active jobs still running") ; /* Report the error */
            if(tryExit) {                                /* If the command was "exit" (as opposed to Ctrl+D) */
                CompleteCmd("exit", 1);                  /* Print '+ completed' message */
                tryExit = 0;                             /* Reset the variable */
            }
        keepRunning = 1;                                 /* Set the while loop to continue running */
        CheckCompletedProcesses(processList);            /* Check for completed processes */
        DisplayPrompt(&cursorPos);                       /* Reprint the prompt */
        goto mainLoop;                                   /* Re-enter main loop */
    }

    ResetCanMode();                                      /* Switch back to previous terminal mode */
    SayGoodbye();                                        /* Print the exit message */
    
    return EXIT_SUCCESS;
}
/* **************************************************** */
