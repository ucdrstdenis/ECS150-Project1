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
void CompleteCmd (char *cmd, char exitCode)
{
    char msg[BUFFER_SIZE + 25];                           
    sprintf(msg, "\n+ completed '%s' [%d]", cmd, exitCode);
    write(STDOUT_FILENO, msg, strlen(msg));
}
/* **************************************************** */

/* **************************************************** */
/* Print error message to STDERR                        */
/* **************************************************** */
void ThrowError (char *msg)
{
    write(STDOUT_FILENO, NEWLINE, strlen(NEWLINE));
    write(STDERR_FILENO, msg, strlen(msg));
}                    
/* **************************************************** */

/* **************************************************** */
/* Stripes trailing or leading whitespace from a string */
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
/* Breaks up a command into a dynamically allocated     */
/* array of arguments.                                  */
/*    "ls -l -a" -> {"ls","-l","-a", NULL};             */
/* **************************************************** */
char **Cmd2Array(char *cmd)
{
    char **args =  (char **)malloc(MAX_TOKENS * sizeof(char*)); 
    char *space;
    unsigned int i = 0; 
    cmd = RemoveWhitespace(cmd);                        /* Remove leading/trailing whitespace                   */
    space = strchr(cmd, ' ');                           /* space points to the first occurance of ' ' in cmd    */
   
    while(space != NULL) {                              /* Repeat until no more ' ' found                       */
        *space = '\0';                                  /* Replace ' ' with '\0'                                */
        args[i] = (char *)malloc(strlen(cmd)+1);        /* Allocate space for the cmd string                    */
        args[i++] = cmd;                                /* Put null terminated string into the arguments array  */
        cmd = RemoveWhitespace(space + 1);              /* Remove leading/trailing whitespace in  command       */
        space = strchr(cmd, ' ');                       /* space points to the first place ' ' occurs in cmd    */
    }

    if (*cmd != '\0') {                                 /* If no more spaces, but still not end of cmd          */
        args[i] = (char *)malloc(strlen(cmd)+1);        /* Allocate space for the remaining portion of cmd      */
        args[i++] = cmd;    
    }    

    args[i] = NULL;
    return args;
}
/* **************************************************** */

/* **************************************************** */
/* Breaks up  a command into a double array of cmds     */
/* "ls -la|grep filename" -> {args0,args1,NULL}         */
/* where args0 = {"ls", "-la", NULL} and                */
/* where args1 = {"grep","filename", NUll}              */
/*         AND                                          */
/* "ls -la" -> {args0, NULL}                            */
/* where args0 = {"ls", "-la", NULL}                    */
/* **************************************************** */
char ***Pipes2Arrays(char *cmd){
    unsigned int i = 0;
    char ***pipes =  (char ***)malloc(MAX_TOKENS * sizeof(char**));
    char *bar = strchr(cmd, '|');                       /* bar points to the first occurance of '|' in cmd      */

    if (bar == NULL)                                    /* If no '|' found                                      */
        pipes[i++] = Cmd2Array(cmd);                    /* Make sure Cmd2Array is called once for allocation    */

    while(bar != NULL) {                                /* Repeat until no more '|' found                       */
        *bar = '\0';                                    /* Replace '|' with '\0;                                */
        pipes[i++] = Cmd2Array(cmd);                    /* Put the cmd array into the pipes array               */
        cmd = RemoveWhitespace(bar+1);                  /* Remove leading/trailing whitespace for remaining part the command*/
        bar = strchr(cmd, '|');                         /* bar points to the first occurance of '|' in cmd      */ 
    }

    if (*cmd != '\0')
        pipes[i++] = Cmd2Array(cmd);

    pipes[i] = NULL;
    return pipes;    
}

/* **************************************************** */
/* Change Directory Command                             */
/* **************************************************** */
char ChangeDir(char *args[]) 
{
    if ((args[0] == NULL) || (chdir(args[0]) == -1)) {  /* If no dir specified or chdir() fails  */
        ThrowError("Error: no such directory"); 
        return 1;                                       /* Return error code 1                   */    
    }    
    return 0;                                           /* Otherwise return error code 0         */
}
/* **************************************************** */

/* **************************************************** */
/* Print Working Directory                              */
/* **************************************************** */
char PrintDir(char *args[])
{
    return 0;
}
/* **************************************************** */

/* **************************************************** */
/* Wrapper to execute whatever is on the command line   */
/* **************************************************** */
char RunCommand(char *cmdLine)
{
   char *cmdCopy = (char *)malloc(strlen(cmdLine)+1);   /* Holds copy of the command line        */
   strcpy(cmdCopy, cmdLine);                            /* Make the copy                         */

   cmdLine = RemoveWhitespace(cmdLine); 		        /* Remove leading/trailing whitespace    */
   char ***Cmds = Pipes2Arrays(cmdLine);                /* Breakup command into double array[][] */

   if (Cmds[0][0] == NULL)                              /* Return if nothing in command line     */
        return 0;
    
    if (!strcmp(Cmds[0][0], "exit"))                    /* 'exit' forces main loop to break      */
        return 1;
    
    if (!strcmp(Cmds[0][0], "cd"))                      /* If first command = "cd"              */
        CompleteCmd(cmdCopy, ChangeDir(&Cmds[0][1]));

    else if (!strcmp(Cmds[0][0], "pwd")) {
        CompleteCmd(cmdCopy, PrintDir(&Cmds[0][1]));    
    } else {
        
    }
    return 0;
}
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

int main(int argc, char *argv[], char *envp[])
{
    int cursorPos = 0;
    char keystroke, cmdLine[BUFFER_SIZE];
    unsigned char tryExit = 0;
    History *history = (History*) malloc(sizeof(History));

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
            if (GetChar() == ARROW) {
                keystroke = GetChar();
                if (keystroke == UP)
                   DisplayNextEntry(history, cmdLine, &cursorPos);
                else if (keystroke == DOWN)
                    DisplayPrevEntry(history, cmdLine, &cursorPos);
                else if (keystroke == LEFT)
                    ErrorBell();
                else if (keystroke == RIGHT)
                    ErrorBell();
            }
            continue;
        }
       
        /* ENTER KEY */
        else if (keystroke == RETURN) {
            cmdLine[cursorPos] = '\0';
            AddHistory(history, cmdLine, cursorPos);
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
    } /* End Main Loop */
    

    if (backgroundCmdRunning) {                             /* If background commands are still running */
        ThrowError("Error: active jobs still running");     /* Report the error */
            if(tryExit) {                                   /* If the command was "exit", as opposed to Ctrl+D */
                CompleteCmd("exit", 1);                 /* Print '+ completed' message */ 
                tryExit = 0;                                /* Reset the variable */
            }

        DisplayPrompt(&cursorPos, 0);                       /* Reprint the prompt */
        goto mainLoop;                                      /* Re-enter main loop */
    }

    write(STDOUT_FILENO, EXITLINE, strlen(EXITLINE));
    ResetCanMode();     /* Switch back to previous terminal mode */
    
    return EXIT_SUCCESS;
}
/* ************************************ */
