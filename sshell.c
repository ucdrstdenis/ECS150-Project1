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
    while ((PID = waitpid(-1, &status, WNOHANG)) > 0){   /* Allow multiple child processes to terminate if necessary  */
        MarkProcessDone(processList, PID, xStat(status));/* Mark the process as completed                             */
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
    int fd;                                             /* File descriptor                          */
    getcwd(workingDir, MAX_BUFFER);                     /* Write working directory into workingDir  */
    sprintf(workingDir, "%s\n", workingDir);            /* Add a new line character                 */
   
    if (args[1]==NULL) {                                /* If no additional args, wrtie to STDOUT   */
        write(STDOUT_FILENO, workingDir, strlen(workingDir));
        return 0;
    }

    if (!strcmp(args[1], ">") && args[2] != NULL) {     /* If the first argument is '>', set out fd */
        fd = open(args[2], WMODE);                      /* (filename,Access mode, premissions)      */
        write(fd, workingDir, strlen(workingDir));
        return 0;
    } else {
        ThrowError("Error: invalid argument to pwd");
        return 1;
    }
}
/* **************************************************** */

/* **************************************************** */
/* Breaks up a command into a NULL terminated array.    */
/*                                                      */
/* cmd = "ls -l -a" returns {"ls","-l","-a", NULL};     */
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
/* 2D array of command pointers.                        */
/*                     Example  1                       */
/* "ls -la|grep filename" -> {args0, args1, NULL}       */
/* where args0 = {"ls", "-la", NULL}                    */
/* where args1 = {"grep", "filename", NUll}             */
/*                     Example  2                       */
/* "ls -la" -> {args0, NULL}                            */
/* where args0 = {"ls", "-la", NULL}                    */
/* **************************************************** */
char ***Pipes2Arrays(char *cmd, char *numPipes)
{
    unsigned int i = 0;
    char ***pipes =  (char ***) malloc(MAX_TOKENS * sizeof(char**));
    cmd = RemoveWhitespace(cmd);                        /* Remove leading/trailing whitespace                   */
    char *bar = strchr(cmd, '|');                       /* bar points to the first occurance of '|' in cmd      */
    
    while(bar != NULL) {                                /* Repeat until no more '|' found or max tokens reached */
        *bar = '\0';                                    /* Replace '|' with '\0                                 */
        pipes[i++] = Cmd2Array(cmd);                    /* Put the cmd array into the pipes array               */
        cmd = RemoveWhitespace(bar+1);                  /* Remove leading/trailing whitespace for rest command  */
        bar = strchr(cmd, '|');                         /* bar points to the first occurance of '|' in cmd      */
    }      
    
    if (*cmd != '\0')                                   /* If there are still characters in cmd                 */
        pipes[i++] = Cmd2Array(cmd);                    /* Add them to the array                                */
         
    pipes[i] = NULL;                                    /* Set the last entry to be NULL                        */
    *numPipes = i-1;                                    /* Number of Pipes in commmand + 1                      */
    return pipes;                                       /* Return the pointer                                   */
}
/* **************************************************** */
/* **************************************************** */
/* Function to execute single program call              */
/* Redirects i/o from/to Process file descriptors       */
/* **************************************************** */
void ExecProg(char *cmds[], Process *P)
{
    Dup2AndClose(P->fd[0], STDIN_FILENO);               /* Read from fd[0]                          */
    Dup2AndClose(P->fd[1], STDOUT_FILENO);              /* Write to fd[1]                           */
    execvp(cmds[0], cmds);                              /* Execute command                          */
    perror("execvp");                                   /* Report an error if code gets here        */
    exit(EXIT_FAILURE);                                 /* Exit with  failure                       */
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
/* Forks a process. Child executes, parent waits        */
/* **************************************************** */
void ForkMe(char *cmds[], Process *Me)
{
    Me->PID = fork();                                   /* Fork the process, set the PID          */
    switch(Me->PID) {                                   /* Switch statemnt on PID                 */
        case -1:                                        /* -1 means fork() failed                 */
            perror("fork");                             /* Report the error                       */
            exit(EXIT_FAILURE);                         /* fork failed, kill the process          */
        case 0:                                         /* Child Process                          */
            ExecProg(cmds, Me);                         /* Execute the program                    */
        default:                                        /* Parent Process (PID > 0)               */
            if (Me->fd[0] != SI) close(Me->fd[0]);      /* Parent closes the read pipes           */
            if (Me->fd[1] != SO) close(Me->fd[1]);      /* Parent closes the write pipe           */
            Wait4Me(Me);                                /* Wait w/ blocking or non-blcoking       */
    }
}
/* **************************************************** */

/* **************************************************** */
/* Execute program commands, looped if they are piped   */
/* **************************************************** */
char ExecProgram(char **cmds[], Process *P)
{
    Process *Me, *cP, *cP2;                             /* Pointers to process structures       */
    int N = 0;                                          /* Pipe iterator                        */         
    int firstPipe[2];                                   /* FD for chaining pipes together       */
    int secPipe[2];                                     /* FD for chaining pipes togetehr       */
    int *myPipe;                                        /* Pointer that points to 1 of 2 pipes  */
    Me = P;                                             /* Me points to the parent in the chain */
    while ((cmds[N+1] != NULL) && cmds[N+2] != NULL) {  /* While pipes to chain together exist  */
         cP = AddProcessAsChild(processList, Me, 1, "\0", Me->nPipes-1, Me->isBG, Me->fd);

        /* Setup Pipes from P1 to P2 */
        if (CheckRedirect(cmds, cP, N)) return 1;       /* Setup redirects, check against pipes */
        pipe(firstPipe);                                /* Create the Pipe                      */
        cP->fd[1] = firstPipe[1];                       /* Child will write to the pipe         */
        ForkMe(cmds[N++], cP);                          /* Fork the process, exec, close & wait */
        
        /* Setup Pipes from P2 to P3 */
        cP2 = AddProcessAsChild(processList, cP, 1, "\0", cP->nPipes-1, cP->isBG, cP->fd);
        if (CheckRedirect(cmds, cP2, N)) return 1;      /* Setup redirects, check against pipes */
        pipe(secPipe);                                  /* Create the Pipe                      */
        cP2->fd[0] = firstPipe[0];                      /* Child will read from last pipe       */
        cP2->fd[1] = secPipe[1];                        /* but will write to the next pipe      */              
        ForkMe(cmds[N++], cP2);                         /* Fork the process, exec, close & wait */

        Me = cP2;                                       /* Parent now becomes child process 2   */
        myPipe = secPipe;                               /* myPipe points to secPipe             */
    }

    /* Only 2 commands to pipe left */ 
    if (cmds[N+1] != NULL) {                                          
        cP = AddProcessAsChild(processList, Me, 1, "\0", Me->nPipes-1, Me->isBG, Me->fd);
        if (CheckRedirect(cmds, cP, N)) return 1;       /* Setup redirects, check against pipes */
        pipe(firstPipe);                                /* Create the Pipe                      */
        cP->fd[1] = firstPipe[1];                       /* Child will write to the pipe         */
        ForkMe(cmds[N++], cP);                          /* Fork the process, exec program, wait */
        myPipe = firstPipe;                             /* myPipe points to firstPipe           */
    } 

    /* Only 1 command to left to run  */
    if (CheckRedirect(cmds, P, N)) return 1;            /* Setup redirects, check against pipes */
    if (N != 0)  P->fd[0] = myPipe[0];                  /* If last in a chain, get piped input  */
    ForkMe(cmds[N], P);
    return 0;
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
    char numPipes = 0;					                /* Num pipes in the command +1 (max 255  */
    int fd[2] = {SI, SO};                               /* Holds I/O file descriptors            */
    
    strcpy(cmdCopy, cmdLine);                           /* Make the copy                         */
    cmdLine = InsertSpaces(cmdLine);                    /* Add spaces before and after <>&       */
    cmdLine = RemoveWhitespace(cmdLine);                /* Remove leading/trailing whitespace    */
    
    if (CheckCommand(cmdLine, &isBg)) return 0;         /* Check for invalid character placement */
    Cmds = Pipes2Arrays(cmdLine, &numPipes);            /* Breakup command into  *array[][]      */
    
    if (Cmds[0] == NULL)              return 0;         /* Return if nothing in command line     */
    if (!strcmp(Cmds[0][0], "exit"))  return 1;         /* 'exit' forces main loop to break      */
    
    if (!strcmp(Cmds[0][0], "cd"))                      /* If first command = "cd"               */
        CompleteCmd(cmdCopy, ChangeDir(&Cmds[0][1]));   /* cd and print + completed message      */
    
    else if (!strcmp(Cmds[0][0], "pwd"))                /* If first command = "pwd"              */
        CompleteCmd(cmdCopy, PrintWDir(Cmds[0]));       /* pwd & print + completed message       */
    
    else {                                              /* Otherwise, try executing the pipes    */
        P = AddProcess(processList, 0, cmdCopy, numPipes, isBg, fd);
        if(ExecProgram((char ***)Cmds, P)) {            /* If this returns 1, something failed   */  
            P->running = 0;                             /* Mark the process as done              */
            P->status = 1;                              /* Set the failure status                */
        } 
    }
    return 0;                                           /* Continue main loop                     */
}
/* **************************************************** */

/* **************************************************** */
/* Sets up redirects and checks if piped                */
/* **************************************************** */
char CheckRedirect(char **cmds[], Process *P, int N)
{
    if (Redirect(cmds[N], P->fd)) return 1;             /* Bad command, return 1                */
    if ((P->fd[0] != SI) && (N != 0))  return 1;        /* Can't have piped input and file input*/
    if ((P->fd[1] != SO) && (cmds[N+1]!= NULL))         /* Can't have piped output and file out */ 
        return 1;                                       /* Bad command, return 1                */

    return 0;                                           /* No bad connections                   */
}
/* **************************************************** */

/* **************************************************** */
/* Sets up redirective file descriptors                 */
/* Returns file descriptors via fd pointer              */
/* Returns 0 if good command, 1 if bad command          */
/* **************************************************** */
char Redirect(char *args[], int *fd)
{                                    
    char sym;                                           /* Holds redirect character               */
    int i = 0;                                          /* Iterator                               */        
    fd[0] = STDIN_FILENO;                               /* Inputfile descriptor to return         */
    fd[1] = STDOUT_FILENO;                              /* Output file descriptor to return       */
	while (args[i++] != NULL) {                         /* Iterate through the arguments          */
        if ((sym = Check4Special(*args[i-1]))) {        /* Check if args] = <> or &               */            
            if (args[i] == NULL) {                      /* If no argument afer <>                 */
               ThrowError("Error: invalid command line");             
               return 1;                                /* Bad command, return 1                  */
            }

            args[i-1] = NULL;                           /* Replace <> with NULL to terminate arry */
            if((sym == '>') && (fd[1] == SO))           /* If output redirect, and out fd not set */
                fd[1] = open(args[i], WMODE);           /* Open for writing, WMODE in sshell.h    */          
            else if ((sym == '<') && (fd[0] == SI))     /* If input redirect, and in fd not set   */
                fd[0] = open(args[i], O_RDONLY);        /* Set the input file descriptor          */
                                                        /* @TODO Setup catch in case this fails   */
            else { 
                ThrowError("Error: invalid command line");
                return 1;                               /* Bad command, return 1                  */
            }
        }
	}
    return 0;                                           /* Good command, return 0                 */
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
                else {                                    
                    CheckCompletedProcesses(processList);          
                    DisplayPrompt(&cursorPos);       
		}                
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
