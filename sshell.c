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
    
    while (isspace(*string)) string++;                  /* Remove leading whitespace  */
    while (isspace(string[i])) string[i--] = '\0';      /* Remove trailing whitespace */
    
    return string;                                      /* Return updated start address of string */
}

/* Breaks up  a command into an array of arguments 		*/
/* "ls -l -a" gets changed to {"ls","-l","-a", NULL};   */
void Cmd2Array(char *cmd, char *args[])
{
    unsigned int i = 0;
    cmd = RemoveWhitespace(cmd);                        /* Remove leading/trailing whitespace */
    char *space = strchr(cmd, ' ');                     /* space points to the first occurance of ' ' in cmd */

    while(space != NULL) {                              /* Repeat until no more ' ' found */
        *space = '\0';                                  /* Replace ' ' with '\0' */
        args[i++] = cmd;                                /* Put the null terminated string into the arguments buffer */
        cmd = RemoveWhitespace(space + 1);              /* Remove leading/trailing whitespace */
        space = strchr(cmd, ' ');                       /* space points to the first place ' ' occurs in cmd */
    }

    (*cmd != '\0') ? (args[i++] = cmd) : (args[i] = NULL);
    args[i] = NULL;
}

/*
void Pipes2Arrays(){

}
*/

/* ************************************ */
char RunCommand(char *cmdLine)
{
   char *args[MAX_TOKENS];
   cmdLine = RemoveWhitespace(cmdLine);   
   Cmd2Array(cmdLine, args);

   if (args[0] == NULL) /* Return if nothing in command line */
        return 0;
    
    if (!strcmp(args[0], "exit")) /* Force main loop to break */
        return 1;
    
    if (!strcmp(args[0], "cd")) {
        
    } else if (!strcmp(args[0], "pwd")) {
        
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
    char RxChar, cmdLine[BUFFER_SIZE];
    //char *PATH = getenv("PATH");
    History *history = (History*) malloc(sizeof(History));
    InitShell(history, &cursorPos);

//mainLoop:     /* Shell main loop label*/
    while (1) {
        RxChar = GetChar();
        
        // CTRL + D
        if (RxChar == CTRL_D)
            break;
        
        // TAB KEY
        else if (RxChar == TAB)
            ErrorBell();
        
        // BACKSPACE
        else if (RxChar == BACKSPACE) {
            if (cursorPos) {
                write(STDIN_FILENO, BACKSPACE_CHAR, strlen(BACKSPACE_CHAR));
                cursorPos -= 1;
            }
            else
                ErrorBell();
            continue;
        }
        
        // ARROW KEYS
        else if (RxChar == ESCAPE) {
            if (GetChar() == ARROW) {
                RxChar = GetChar();
                if (RxChar == UP)
                   DisplayNextEntry(history, cmdLine, &cursorPos);
                else if (RxChar == DOWN)
                    DisplayPrevEntry(history, cmdLine, &cursorPos);
                else if (RxChar == LEFT)
                    ErrorBell();
                else if (RxChar == RIGHT)
                    ErrorBell();
            }
            continue;
        }
       
        // ENTER KEY 
        else if (RxChar == RETURN) {
            cmdLine[cursorPos] = '\0';
            AddHistory(history, cmdLine, cursorPos);
            if(RunCommand(cmdLine))
                break;
            
            DisplayPrompt(&cursorPos, 1);
            continue;
        }
        
        // ANY OTHER KEY 
        else {
        //    UNCOMMENT FOR CHARACTER DEBUG
        //    if (isprint(RxChar))
        //        printf("RX: '%c' 0x%02X\n", RxChar, RxChar);
        //    else
        //        printf("RX: ' ' 0x%02X\n", RxChar);
            
            if (cursorPos < BUFFER_SIZE) {
                write(STDIN_FILENO, &RxChar, 1);
                cmdLine[cursorPos++] = RxChar;
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
