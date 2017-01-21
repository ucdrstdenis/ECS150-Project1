#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "common.h"
#include "history.h"
#include "noncanmode.h"
#include "sshell.h"

/* **************************************************** */
/* Prints '+ completed' messages to STDOUT              */
/* **************************************************** */
void CompleteCmd (char *cmd, char exitCode, char newLn)
{
    char msg[BUFFER_SIZE + 25];
    if (newLn)
        sprintf(msg, "\n+ completed '%s' [%d]", cmd, exitCode);
    else
        sprintf(msg, "+ completed '%s' [%d]", cmd, exitCode);
    write(STDOUT_FILENO, msg, strlen(msg));
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
/* Change Directory Command                             */
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
/* Print Working Directory                              */
/* **************************************************** */
/* WORK IN PROGRESS */
char PrintDir(char *args[])
{
    //getcwd();
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
    unsigned int i = strlen(string);
    
    while (isspace(string[i])) string[i--] = '\0';      /* Remove trailing whitespace */
    while (isspace(*string)) string++;                  /* Remove leading whitespace  */

    return string;                                      /* Return updated start address of string */
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
    args[i] = (char *) malloc(sizeof(char*));           /* Allocate space for the NULL character                */
    args[i] = NULL;
    return args;
}
/* **************************************************** */

/* **************************************************** */
/* Breaks up  a command into a double array of cmds     */
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
        CompleteCmd(cmdCopy, ChangeDir(&Cmds[0][1]), 0); /* CD and print +completed message       */

    else if (!strcmp(Cmds[0][0], "pwd")) {               /* first command = "pwd"                 */
        CompleteCmd(cmdCopy, PrintDir(&Cmds[0][1]), 1);  /* pwd print +completed message          */
        
    } else {
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
    int status = 0;                                     /* Holds status */
    pid_t PID;                                          /* Holds the PID */
    
    if (cmds[N+1] == NULL) {                            /* If there's only 1 command */
        if ((PID = fork()) != 0) {                      /* Parent */
            if (BG) {                                   /* If it's to be run in background */
                backgroundCmdRunning = 1;               /* SetBackgroundCmd Flag   */
                return status;                          /* Don't wait for prcess to return  */
            }
            waitpid(-1, &status, 0);                    /* wait for child to exit */
            return status;
            
        } else {
            execv(cmds[N][0], cmds[N]);                 /* execute command */
            perror("execv");                            /* coming back here is an error */
            exit(1);                                    /* exit failure */
        }
    }
    
    /* THIS PART DOESN"T WORK YET */
    else {
        int fdOut[2];                                    /* Create file descriptor */
    
        pipe(fdOut);                                     /* Create pipe */
        if ((PID = fork()) == 0) {                       /* Parent Process*/
            close(fdOut[0]);                             /* Don't need to read from pipe */
            dup2(FD, STDIN_FILENO);                      /* Replace stdout with the pipe */
            dup2(fdOut[1], STDOUT_FILENO);
            execvp(cmds[N][0], cmds[N]);
        } else if (PID > 0) {                            /* Child Process*/
            close(fdOut[1]);                             /* Don't need to write to pipe */
            close(FD);                                   /* Close existing stdout */
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
    /* Initialize history */
    history->count = 0;
    history->traversed = 0;
    history->top = NULL;
    history->current = NULL;
    
    SetNonCanMode();                                    /* Switch to non-canonical terminal mode */
    DisplayPrompt(cursorPos, 0);                        /* Print the prompt and clear the cursor position */
}
/* **************************************************** */

/* **************************************************** */
/*                          MAIN                        */
/* **************************************************** */
int main(int argc, char *argv[], char *envp[])
{
    int cursorPos = 0;
    char keystroke, cmdLine[BUFFER_SIZE];
    unsigned char tryExit = 0;

    History *history = (History*)malloc(sizeof(History));

    InitShell(history, &cursorPos);                     /* Initialize the shell */

mainLoop:                                               /* Shell main loop label*/
    while (1) {
        keystroke = GetChar();

        /* CTRL + D */
        if (keystroke == CTRL_D)
            break;
        
        /* TAB KEY */
        else if (keystroke == TAB)
            ErrorBell();
        
        /* BACKSPACE */
        else if (keystroke == BACKSPACE) {
            if (cursorPos) {
                write(STDIN_FILENO, BACKSPACE_CHAR, strlen(BACKSPACE_CHAR));
                cursorPos -= 1;
            }
            else
                ErrorBell();
            continue;
        }
        
        /* ARROW KEYS */
        else if (keystroke == ESCAPE) {
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
            continue;
        }
       
        /* ENTER KEY */
        else if (keystroke == RETURN) {
            cmdLine[cursorPos] = '\0';
            AddHistory(history, cmdLine, cursorPos);
            write(STDOUT_FILENO, NEWLINE, strlen(NEWLINE));
            if((tryExit = RunCommand(cmdLine)))
                break;
            
            DisplayPrompt(&cursorPos, 1);
            continue;
        }
        
        /* ANY OTHER KEY */
        else {
            if (cursorPos < BUFFER_SIZE) {
                write(STDIN_FILENO, &keystroke, 1);
                cmdLine[cursorPos++] = keystroke;
            } else
                ErrorBell();
        } 
    }                                                       /* End Main Loop */
    

    if (backgroundCmdRunning) {                             /* If background commands are still running */
        ThrowError("Error: active jobs still running");     /* Report the error */
            if(tryExit) {                                   /* If the command was "exit" (as opposed to Ctrl+D) */
                CompleteCmd("exit", 1, 1);                  /* Print '+ completed' message */
                tryExit = 0;                                /* Reset the variable */
            }

        DisplayPrompt(&cursorPos, 1);                       /* Reprint the prompt */
        goto mainLoop;                                      /* Re-enter main loop */
    }

    write(STDOUT_FILENO, EXITLINE, strlen(EXITLINE));
    ResetCanMode();                                         /* Switch back to previous terminal mode */
    
    return EXIT_SUCCESS;
}
/* ************************************ */
