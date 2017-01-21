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


/* Stripes trailing or leading whitespace from a string */
char *RemoveWhitespace(char *string)
{
    unsigned int i = strlen(string);
    
    while (isspace(string[i])) string[i--] = '\0';      /* Remove trailing whitespace */
    while (isspace(*string)) string++;                  /* Remove leading whitespace  */

    return string;                                      /* Return updated start address of string */
}

/* Breaks up a command into a dynamically allocated array of arguments  */
/* "ls -l -a" -> {"ls","-l","-a", NULL};                                */
char **Cmd2Array(char *cmd)
{
    char **args =  (char **)malloc(MAX_TOKENS * sizeof(char*)); 
    char *space;
    unsigned int i = 0; 
    cmd = RemoveWhitespace(cmd);                        /* Remove leading/trailing whitespace */
    space = strchr(cmd, ' ');                           /* space points to the first occurance of ' ' in cmd */
   
    while(space != NULL) {                              /* Repeat until no more ' ' found */
        *space = '\0';                                  /* Replace ' ' with '\0' */
        args[i] = (char *)malloc(strlen(cmd)+1);        /* Allocate space for the cmd string */
        args[i++] = cmd;                                /* Put the null terminated string into the arguments array */
        cmd = RemoveWhitespace(space + 1);              /* Remove leading/trailing whitespace of the remaining part of the command*/
        space = strchr(cmd, ' ');                       /* space points to the first place ' ' occurs in cmd */
    }

    if (*cmd != '\0') {
        args[i] = (char *)malloc(strlen(cmd)+1);        /* Allocate space for the remaining portion of cmd  */
        args[i++] = cmd;    
    }    

    args[i] = NULL;
    return args;
}

/* Breaks up  a command into a double array of pipes    */
/* "ls -la|grep filename" -> {args0,args1,NULL}         */
/* where args0 = {"ls", "-la", NULL} and                */
/* where args1 = {"grep","filename", NUll}              */

/* "ls -la" -> {args0, NULL}                            */
/* where args0 = {"ls", "-la", NULL}                    */

char ***Pipes2Arrays(char *cmd){
    unsigned int i = 0;
    char ***pipes =  (char ***)malloc(MAX_TOKENS * sizeof(char**));
    char *psizzle = strchr(cmd, '|');                   /* psizzle points to the first occurance of '|' in cmd */

    if (psizzle == NULL)                                /* If no '|' found */
        pipes[i++] = Cmd2Array(cmd);                    /* Make sure Cmd2Array is called once for allocation */

    while(psizzle != NULL) {                            /* Repeat until no more '|' found */
        *psizzle = '\0';                                /* Replace '|' with '\0; */
        pipes[i++] = Cmd2Array(cmd);                    /* Put the null terminated array into the pipes array */
        cmd = RemoveWhitespace(psizzle+1);              /* Remove leading/trailing whitespace of the remaining part of the command*/
        psizzle = strchr(cmd, '|');          
    }

    if (*cmd != '\0')
        pipes[i++] = Cmd2Array(cmd);

    pipes[i] = NULL;
    return pipes;    
}
/* ************************************ */
char RunCommand(char *cmdLine)
{
   cmdLine = RemoveWhitespace(cmdLine); 
   char ***pipes = Pipes2Arrays(cmdLine);

   if (pipes[0][0] == NULL) /* Return if nothing in command line */
        return 0;
    
    if (!strcmp(pipes[0][0], "exit")) /* Force main loop to break */
        return 1;
    
    if (!strcmp(pipes[0][0], "cd")) {
        
    } else if (!strcmp(pipes[0][0], "pwd")) {
        
    } else {
        
    }
    return 0;
}

void InitShell(History *history, int *cursorPos)
{
    /* Initialize history */
    history->count = 0;
    history->traversed = 0;
    history->top = NULL;
    history->current = NULL;
    
    SetNonCanMode();               /* Switch to non-canonical terminal mode */
    DisplayPrompt(cursorPos, 0);   /* Print the prompt and clear the cursor position */
}

int main(int argc, char *argv[], char *envp[])
{
    int cursorPos = 0;
    char keystroke, cmdLine[BUFFER_SIZE];
    //char *PATH = getenv("PATH");
    History *history = (History*) malloc(sizeof(History));
    InitShell(history, &cursorPos);

//mainLoop:     /* Shell main loop label*/
    while (1) {
        keystroke = GetChar();
        
        // CTRL + D
        if (keystroke == CTRL_D)
            break;
        
        // TAB KEY
        else if (keystroke == TAB)
            ErrorBell();
        
        // BACKSPACE
        else if (keystroke == BACKSPACE) {
            if (cursorPos) {
                write(STDIN_FILENO, BACKSPACE_CHAR, strlen(BACKSPACE_CHAR));
                cursorPos -= 1;
            }
            else
                ErrorBell();
            continue;
        }
        
        // ARROW KEYS
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
       
        // ENTER KEY 
        else if (keystroke == RETURN) {
            cmdLine[cursorPos] = '\0';
            AddHistory(history, cmdLine, cursorPos);
            if(RunCommand(cmdLine))
                break;
            
            DisplayPrompt(&cursorPos, 1);
            continue;
        }
        
        // ANY OTHER KEY 
        else {
        /*    UNCOMMENT FOR CHARACTER DEBUG     */
        //    if (isprint(keystroke))
        //        printf("RX: '%c' 0x%02X\n", keystroke, keystroke);
        //    else
        //        printf("RX: ' ' 0x%02X\n", keystroke);
            
            if (cursorPos < BUFFER_SIZE) {
                write(STDIN_FILENO, &keystroke, 1);
                cmdLine[cursorPos++] = keystroke;
            } else
                ErrorBell();
        } 
    }

    /* Check if background command running first placeholder() */
    write(STDOUT_FILENO, EXITLINE, strlen(EXITLINE));
    ResetCanMode();     /* Switch back to previous terminal mode */
    
    return EXIT_SUCCESS;
}
/* ************************************ */
