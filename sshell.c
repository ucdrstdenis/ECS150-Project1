#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

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
    while ((PID = waitpid(-1, &status, WNOHANG)) > 0)    /* Allow multiple child processes to terminate if necessary */
        MarkProcessDone(processList, PID, xStat(status));/* Mark the process as completed                            */
}
/* **************************************************** */

/* **************************************************** */
/* Searches the PATH variable for the location of       */
/* the specified program                                */
/* Uses the first entry in PATH that has valid entry    */
/*                                                      */
/* NOT NEEDED IF EXECVP USED                            */
/* Example - *PATH = /usr/bin:/opt/bin                  */
/*           *prog = "ls"                               */
/*           returns "/usr/bin/ls"                      */
/* **************************************************** */
char *SearchPath(char *prog) {
    unsigned int len = strlen(prog);                    /* Length of the string of the passed program  */
    char *binary  = (char *) malloc(MAX_BUFFER+len);    /* Pointer to hold the full name of the binary */
    char *PATH = getenv("PATH");                        /* Store contents of the PATH variable         */
    char *semi = strchr(PATH, ':');                     /* semi points to the first place ':' occurs   */

    while(semi != NULL) {                               /* Repeat until no more ':' found              */
        *semi = '\0';                                   /* Terminate the string where ':' was          */
        sprintf(binary, "%s/%s", PATH, prog);           /* Append the first path to binary name        */
        if(access(binary, F_OK) != -1)                  /* If binary exists                            */
            return binary;                              /* Return the full name of the binary          */
        PATH = semi+1;                                  /* Update the address PATH points to           */
        semi = strchr(PATH, ':');                       /* semi points to the next place ':' occurs    */
    }
    
    sprintf(binary, "%s/%s", PATH, prog);               /* Append binary to last entry in path         */
    if(access(binary, F_OK) != -1)                      /* If binary exists                            */
        return binary;
    else                                                /* If it doesn't exists                        */
        binary = prog;                                  /* Just store the argument that was passed     */

    return binary;                                      /* Return the binary name                      */
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
    getcwd(workingDir, MAX_BUFFER);                     /* Write working directory into workingDir */
    sprintf(workingDir, "%s\n", workingDir);            /* Add a new line character                */
    write(STDOUT_FILENO, workingDir, strlen(workingDir));

    // @TODO search through args[] and setup output redirect if necesary

    return 0;
}
/* **************************************************** */

/* **************************************************** */
/* Check for invalid placement of special characters    */
/* Also set background flag if '&' is last character    */
/* **************************************************** */
char CheckCommand(char *cmd, char *isBackground)
{
    char s  = *cmd;                                     /* Get the first character in the array   */
    char *end = strchr(cmd, '\0')-1;                    /* Get the last character in the array    */
    *isBackground = 0;                                  /* Set the default flag                   */
    
    if (*end == '|' || *end == '>' || *end == '<') {    /* Check the character at the end         */
        ThrowError("Error: invalid command line");
        return 1;
    }
    
    if (s == '|' || Check4Special(s)) {                 /* Check the character at the beginning   */
        ThrowError("Error: invalid command line");
        return 1;
    }
    
    if (*end == '&') {                                  /* If '&' is last character               */
        *isBackground = 1;                              /* Set the Background flag                */
        *end = '\0';                                    /* Remove '&' from the command            */
    }
    return 0;
}
/* **************************************************** */

/* **************************************************** */
/* Breaks up a command into a NULL terminated           */ 
/* dynamically allocated array.                         */
/*                                                      */
/*    "ls -l -a" -> {"ls","-l","-a", NULL};             */
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
/*                     Example  3                       */
/* "ls -la>outfile" -> {args0, NULL}                    */
/* where args0 = {"ls", "-la", ">", "outfile", NULL}    */
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
/* Wrapper to execute whatever is sent from command line*/
/* **************************************************** */
char RunCommand(char *cmdLine)
{
    char ***Cmds = (char ***) malloc(MAX_TOKENS * sizeof(char**));
    Process *P;                                          /* New Process Pointer                   */
    char isBg = 0;                                       /* Flag for background commands          */
    char *cmdCopy = (char *) malloc(strlen(cmdLine)+1);  /* Holds copy of the command line        */
    
    strcpy(cmdCopy, cmdLine);                            /* Make the copy                         */
    cmdLine = InsertSpaces(cmdLine);                     /* Add spaces before and after <> or &   */
    cmdLine = RemoveWhitespace(cmdLine);                 /* Remove leading/trailing whitespace    */
    
    if (CheckCommand(cmdLine, &isBg)) return 0;          /* Check for invalid character placement */
    Cmds = Pipes2Arrays(cmdLine);                        /* Breakup command into  *array[][]      */
    
    if (Cmds[0] == NULL)              return 0;          /* Return if nothing in command line     */
    if (!strcmp(Cmds[0][0], "exit"))  return 1;          /* 'exit' forces main loop to break      */
    
    if (!strcmp(Cmds[0][0], "cd"))                       /* If first command = "cd"               */
        CompleteCmd(cmdCopy, ChangeDir(&Cmds[0][1]));    /* cd and print + completed message      */
    
    else if (!strcmp(Cmds[0][0], "pwd"))                 /* If first command = "pwd"              */
        CompleteCmd(cmdCopy, PrintWDir(&Cmds[0][1]));    /* pwd & print + completed message       */
    
    else {                                               /* Otherwise, try executing the program  */
        P = AddProcess(processList, fork(), cmdCopy, isBg, SI); /* Add to list of processes            */
        
        switch(P->PID) {
            case -1:                                    /* fork() failed                          */
                perror("fork");                         /* Report the error                       */
                exit(EXIT_FAILURE);                     /* Exit with failure                      */
            case 0:                                     /* Child Process                          */
                ExecProgram((char ***)Cmds, 0, P);      /* Execute recursive piping               */
            default:                                    /* Parent Process (PID > 0)               */
                Wait4Me(P);                             /* Wait w/ blocking or non-blcoking       */
        }
    }
    return 0;
}
/* **************************************************** */

/* **************************************************** */
/* Function to execute blocking or nonblocking wait()   */
/* **************************************************** */
void Wait4Me(Process *P)
{
    int status;
    if (P->isBG) {                                      /* If it's to be run in background        */
        waitpid(P->PID, &status, WNOHANG);              /* Non-blocking call to waitpid           */
        P->status = xStat(status);                      /* Set the temporary status               */
    } else {                                            /* Otherwise                              */
        waitpid(P->PID, &status, 0);                    /* wait for child to exit                 */
        MarkProcessDone(processList, P->PID, xStat(status));
    }
}
/* **************************************************** */

/* **************************************************** */
/* Function to redirect file descriptors                */
/* **************************************************** */
void Dup2AndClose(int fdOld, int fdNew)
{
    if (fdOld != fdNew) {                               /* Check file descriptors are not the same */
        if(dup2(fdOld, fdNew) != -1) {                  /* Check dup2() succeeds                   */
            if (close(fdOld) == -1)                     /* Check close() doesn't fail              */
                perror("close");                        /* Report the error if close fails         */
        } else {                                        /* If dup2() fails                         */
            perror("dup2");                             /* Report the error                        */
            exit(EXIT_FAILURE);                         /* Exit with failure                       */
        }
    }
}
/* **************************************************** */

/* **************************************************** */
/* Function to execute program commands                 */
/* If function is piped, execProgram() is recursive     */
/* See 'recursive-piping' reference                     */
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
            
            case 0:                                     /* Child Process                            */
                close(fdOut[0]);                        /* Don't need to read from pipe             */
                Dup2AndClose(P->fdIn, SI);              /* Link Input file descriptor to the pipe   */
                Dup2AndClose(fdOut[1], SO);             /* Link output file descripter to STDOUT    */
                //cmds[N][0] = SearchPath(cmds[N][0]);  /* Replace with full PATH to binary name    */
                execvp(cmds[N][0], cmds[N]);            /* Execute the command                      */
                perror("execvp");                       /* Coming back here is an error             */
                exit(EXIT_FAILURE);                     /* Exit failure                             */
            
            default:                                    /* Parent Process                           */                close(fdOut[1]);                        /* Don't need to write to pipe              */
                close(P->fdIn);                         /* Close existing stdout                    */
                Wait4Me(cP);                            /* Blocking or non-blocking wait            */
                ExecProgram(cmds, N+1, cP);
        }
    }
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
    
    /* Setup SIGCHLD interrupt handler */
    struct sigaction act;                               /* Sigaction struct for SIGCHLD signal handlers     */
    act.sa_handler = ChildSignalHandler;                /* Setup interrupt handler                          */
    
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
