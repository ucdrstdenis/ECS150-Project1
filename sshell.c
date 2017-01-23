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
#include "common.h"                                     /* Keystrokes and common functions                */
#include "history.h"                                    /* History structures and related functions       */
#include "noncanmode.h"                                 /* Modified  version of the file provided by Joel */
#include "sshell.h"                                     /* Function prototypes for sshell.c functions     */
/* **************************************************** */

/* **************************************************** */
/* Prints '+ completed' messages to STDERR              */
/* **************************************************** */
void CompleteCmd (char *cmd, char exitCode, char newLn)
{
    char msg[MAX_BUFFER + 25];
    if (newLn)
        sprintf(msg, "\n+ completed '%s' [%d]", cmd, exitCode);
    else
        sprintf(msg, "+ completed '%s' [%d]", cmd, exitCode);

    write(STDERR_FILENO, msg, strlen(msg));
}
/* **************************************************** */

/* **************************************************** */
/* Print error message to STDERR                        */
/* **************************************************** */
void ThrowError (char *msg)
{
    //write(STDERR_FILENO, NEWLINE, strlen(NEWLINE));
    write(STDERR_FILENO, msg, strlen(msg));
}                    
/* **************************************************** */

/* **************************************************** */
/* Searches the PATH variable for the location of       */
/* the specified program                                */
/* Uses the first entry in PATH that has valid entry    */
/*                                                      */
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
        *semi = '\0';                                   /* Terminate the string                        */
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
    if ((args[0] == NULL) || (chdir(args[0]) == -1)) {  /* If no dir specified or chdir() fails  */
        ThrowError("Error: no such directory\n");
        return 1;                                       /* Return error code 1                   */
    }
    return 0;                                           /* Otherwise return error code 0         */
}
/* **************************************************** */

/* **************************************************** */
/* Print Working Directory  (handles pwd)               */
/* **************************************************** */
char PrintWDir(char *args[])
{
    char workingDir[MAX_BUFFER];
    getcwd(workingDir, MAX_BUFFER);                     /* Write working directory into workingDir */
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
    
    if (s == '|' || s == '>' || s == '<' || s == '&') { /* Check the character at the beginning   */
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
/* Strips trailing and leading whitespace from a string */
/* **************************************************** */
char *RemoveWhitespace(char *string)
{
    unsigned int i = strlen(string);                    /* Length of the string                     */
    
    while (isspace(string[i])) string[i--] = '\0';      /* Remove trailing whitespace               */
    while (isspace(*string)) string++;                  /* Remove leading whitespace                */

    return string;                                      /* Return updated start address of string   */
}
/* **************************************************** */

/* **************************************************** */
/* Breaks up a command into NULL terminated dynamically */
/* allocated array.                                     */
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
        cmd = RemoveWhitespace(space + 1);              /* Remove leading/trailing whitespace in  command       */
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
/* 2D array of pointers to commands                     */
/*                     Example  1                       */
/* "ls -la|grep filename" -> {args0, args1, NULL}       */
/* where args0 = {"ls", "-la", NULL}                    */
/* where args1 = {"grep", "filename", NUll}             */
/*                     Example  2                       */
/* "ls -la" -> {args0, NULL}                            */
/* where args0 = {"ls", "-la", NULL}                    */
/* **************************************************** */
char ***Pipes2Arrays(char *cmd){
    unsigned int i = 0;
    char ***pipes =  (char ***)malloc(MAX_TOKENS * sizeof(char**));
    cmd = RemoveWhitespace(cmd); 		                /* Remove leading/trailing whitespace                   */
    char *bar = strchr(cmd, '|');                       /* bar points to the first occurance of '|' in cmd      */
    
    while(bar != NULL) {                                /* Repeat until no more '|' found                       */
        *bar = '\0';                                    /* Replace '|' with '\0;                                */
        pipes[i++] = Cmd2Array(cmd);                    /* Put the cmd array into the pipes array               */
        cmd = RemoveWhitespace(bar+1);                  /* Remove leading/trailing whitespace for remaining part the command*/
        bar = strchr(cmd, '|');                         /* bar points to the first occurance of '|' in cmd      */
    }
    
    if (*cmd != '\0')                                   /* If there are still characters in cmd                 */
        pipes[i++] = Cmd2Array(cmd);                    /* Add them to the array                                */
    
    pipes[i] = NULL;
    return pipes;
}
/* **************************************************** */

/* **************************************************** */
/* Wrapper to execute whatever is sent from command line*/
/* **************************************************** */
char RunCommand(char *cmdLine)
{
    int exitCode = 0;
    char ***Cmds = (char ***) malloc(MAX_TOKENS * sizeof(char**));
    char *isBackground = (char*) malloc(sizeof(char*));  /* Flag for background commands          */
    
    char *cmdCopy = (char *)malloc(strlen(cmdLine)+1);   /* Holds copy of the command line        */
    strcpy(cmdCopy, cmdLine);                            /* Make the copy                         */
    cmdLine = RemoveWhitespace(cmdLine);                 /* Remove leading/trailing whitespace    */
    
    if (CheckCommand(cmdLine, isBackground))             /* Check for invalid character placement */
        return 0;

    Cmds = Pipes2Arrays(cmdLine);                        /* Breakup command into  *array[][]      */

    if (Cmds[0] == NULL)                                 /* Return if nothing in command line     */
        return 0;
    
    if (!strcmp(Cmds[0][0], "exit"))                     /* 'exit' forces main loop to break      */
        return 1;
    
    if (!strcmp(Cmds[0][0], "cd"))                       /* If first command = "cd"               */
        CompleteCmd(cmdCopy, ChangeDir(&Cmds[0][1]), 0); /* CD and print + completed message      */

    else if (!strcmp(Cmds[0][0], "pwd")) {               /* If first command = "pwd"              */
        CompleteCmd(cmdCopy, PrintWDir(&Cmds[0][1]), 1); /* pwd & print + completed message       */
        
    } else {                                             /* Otherwise, try executing the program  */
        exitCode = ExecProgram((char ***)Cmds, 0, STDIN_FILENO, *isBackground);
        CompleteCmd(cmdCopy, exitCode, 0);
    }
    return 0;
}
/* **************************************************** */

/* **************************************************** */
/* Function to execute program commands                 */
/* If function is piped, execProgram() is recursive     */
/* **************************************************** */
int ExecProgram(char **cmds[], int N, int FD, char BG)
{
    int status = 0;                                     /* Holds status                             */
    pid_t PID;                                          /* Holds the PID                            */
    
    if (cmds[N+1] == NULL) {                            /* If there's only 1 command                */
        if ((PID = fork()) != 0) {                      /* Parent Process                           */
            if (BG) {                                   /* If it's to be run in background          */
                backgroundCmdRunning = 1;               /* Set BackgroundCmd Flag                   */
                return status;                          /* Don't wait for process to return         */
            }
            waitpid(-1, &status, 0);                    /* Wait for child to exit                   */
            return status;
        } else {                                        /* Child Process                            */
            cmds[N][0] = SearchPath(cmds[N][0]);        /* Replace with full PATH to binary name    */
            execv(cmds[N][0], cmds[N]);                 /* Execute command                          */
            perror("execv");                            /* Coming back here is an error             */
            exit(1);                                    /* Exit failure                             */
        }
    }
    
    /* THIS PART DOESN"T WORK YET */
    else {
        int fdOut[2];                                   /* Create file descriptor                   */
    
        pipe(fdOut);                                    /* Create pipe                              */
        if ((PID = fork()) == 0) {                      /* Child Process                            */
            close(fdOut[0]);                            /* Don't need to read from pipe             */
            dup2(FD, STDIN_FILENO);                     /* Link Input file descriptor to the pipe   */
            dup2(fdOut[1], STDOUT_FILENO);              /* Link output file descripter to STDOUT    */
            execvp(cmds[N][0], cmds[N]);
            
        } else if (PID > 0) {                           /* Parent Process                           */
            close(fdOut[1]);                            /* Don't need to write to pipe              */
            close(FD);                                  /* Close existing stdout                    */
            ExecProgram(cmds, N+1, fdOut[0], BG);
        }
    }
    return 0;
}

/* **************************************************** */

/* **************************************************** */
/* Shell Initialization function                        */
/* **************************************************** */
void InitShell(History *history, int *cursorPos)
{
    /* Initialize history structure */
    history->count = 0;
    history->traversed = 0;
    history->top = NULL;
    history->current = NULL;
    
    SetNonCanMode();                                    /* Switch to non-canonical terminal mode */
    /* PrintWelcomeMessage() */                         // @TODO
    DisplayPrompt(cursorPos, 0);                        /* Print the prompt and clear the cursor position */
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

    History *history = (History*)malloc(sizeof(History));

    InitShell(history, &cursorPos);                     /* Initialize the shell */

mainLoop:                                               /* Shell main loop label */
    while (keepRunning) {                               /* Main Loop */
        keystroke = GetChar();
        
        switch(keystroke) {                             /* Process the keytroke */
            case CTRL_D:                                /* CTRL + D */
                keepRunning = 0;
                break;
           
            case TAB:                                   /* TAB KEY */
                ErrorBell();
                break;
            
            
            case BACKSPACE:                             /* BACKSPACE */
                if (cursorPos) {
                    write(STDIN_FILENO, BACKSPACE_CHAR, strlen(BACKSPACE_CHAR));
                    cursorPos -= 1;
                }
                else
                    ErrorBell();
                break;
            
            
            case ESCAPE:                                /* ARROW KEYS */
                if (GetChar() == ARROW)
                    switch(GetChar()) {
                        case UP:
                            DisplayNextEntry(history, cmdLine, &cursorPos);
                            break;
                        case DOWN:
                            DisplayPrevEntry(history, cmdLine, &cursorPos);
                            break;
                        case LEFT:
                            ErrorBell();
                            break;
                        case RIGHT:
                            ErrorBell();
                            break;
                    }
                break;
                
       
            case RETURN:                                /* ENTER KEY */
                cmdLine[cursorPos] = '\0';
                AddHistory(history, cmdLine, cursorPos);
                write(STDOUT_FILENO, NEWLINE, strlen(NEWLINE));
                
                if((tryExit = RunCommand(cmdLine)))
                    keepRunning = 0;                    /* Stop the main loop if 'exit' received */
            
                DisplayPrompt(&cursorPos, 1);
                break;
        
                
            default:                                    /* ANY OTHER KEY */
                if (cursorPos < MAX_BUFFER) {           /* Make sure there's room in the buffer */
                    write(STDIN_FILENO, &keystroke, 1);
                    cmdLine[cursorPos++] = keystroke;
                } else
                    ErrorBell();
        }                                               /* End switch statement */
    }                                                   /* End Main Loop */

    if (backgroundCmdRunning) {                         /* If background commands are still running */
        ThrowError("Error: active jobs still running"); /* Report the error */
            if(tryExit) {                               /* If the command was "exit" (as opposed to Ctrl+D) */
                CompleteCmd("exit", 1, 1);              /* Print '+ completed' message */
                tryExit = 0;                            /* Reset the variable */
            }

        DisplayPrompt(&cursorPos, 1);                   /* Reprint the prompt */
        keepRunning = 1;                                /* Set the while loop to continue running */
        goto mainLoop;                                  /* Re-enter main loop */
    }

    write(STDOUT_FILENO, EXITLINE, strlen(EXITLINE));   /* Write an exit message */
    ResetCanMode();                                     /* Switch back to previous terminal mode */
    
    return EXIT_SUCCESS;
}
/* ************************************ */
