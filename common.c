#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "common.h"

/* **************************************************** */
/* Shell print characters                               */
/* **************************************************** */
extern const char *SHELL_PROMPT;                        /* Defined in sshell.h */
extern const char *BELL;                                /* Defined in sshell.h */
extern const char *NEWLINE;                             /* Defined in sshell.h */
extern const char *EXITLINE;                            /* Defined in sshell.h */
extern const char *BACKSPACE_CHAR;                      /* Defined in sshell.h */
/* **************************************************** */

/* **************************************************** */
/*                  Sound Bell noise                    */
/* **************************************************** */
void ErrorBell(void)
{
    write(STDERR_FILENO, BELL, 1);
}
/* **************************************************** */

/* **************************************************** */
/* Print error message to STDERR                        */
/* **************************************************** */
void ThrowError (char *msg)
{
    char newMsg[MAX_BUFFER];
    sprintf(newMsg, "%s\n", msg);
    write(STDERR_FILENO, newMsg, strlen(newMsg));
}                    
/* **************************************************** */

/* **************************************************** */
/* Prints the exit message                              */
/* **************************************************** */
void SayGoodbye (void)
{
    write(STDERR_FILENO, EXITLINE, strlen(EXITLINE));
}                    
/* **************************************************** */

/* **************************************************** */
/* Displace the main sshell$ prompt                     */
/* **************************************************** */
void DisplayPrompt(int *cursorPos)
{
    write(STDOUT_FILENO, SHELL_PROMPT, strlen(SHELL_PROMPT));
    *cursorPos = 0;
}
/* **************************************************** */

/* **************************************************** */
/* Clear the current cmdLine buffer and STDIN           */
/* **************************************************** */
void ClearCmdLine(char *cmdLine, int *cursorPos)
{
    while (*cursorPos) {
        write(STDOUT_FILENO, BACKSPACE_CHAR, strlen(BACKSPACE_CHAR));
        *cursorPos -= 1;
    }
}
/* **************************************************** */

/* **************************************************** */
/* Print a message to the shell, used in debugging      */
/* **************************************************** */
void Print2Shell(int fd, char *message, char newline)
{
    if (fd == 0)
        fd = STDOUT_FILENO;
    
    char msg2print[MAX_BUFFER]; 
    int cursorPos = 0;
    int i = 0;

    if (newline)
        msg2print[cursorPos++] = '\n';

    while (message[i] != '\0') 
        msg2print[cursorPos++] = message[i++];
    
    msg2print[cursorPos] = '\0'; 
    write(fd, msg2print, strlen(msg2print));
    ErrorBell();

}
/* **************************************************** */

/* **************************************************** */
/* Prints '+ completed' messages to STDERR              */
/* **************************************************** */
void CompleteCmd (char *cmd, char exitCode)
{
    char msg[MAX_BUFFER + 25];
    sprintf(msg, "+ completed '%s' [%d]\n", cmd, exitCode);

    write(STDERR_FILENO, msg, strlen(msg));
}
/* **************************************************** */

/* **************************************************** */
/* Checks if character is whitespace or not             */
/* **************************************************** */
char Check4Space(char key)
{
    if (key == ' ' || key == '\t' || key == '\n')
        return 1;
    else
        return 0;
}
/* **************************************************** */

/* **************************************************** */
/* Checks if special character of not                   */
/* **************************************************** */
char Check4Special(char *key)
{
    switch (*key) {
        case '&': return '&';
        case '<': return '<';
        case '>': return '>';
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
    
    while (Check4Space(string[i])) string[i--]='\0';    /* Remove trailing whitespace               */
    while (Check4Space(*string))   string++;            /* Remove leading whitespace                */
    
    return string;                                      /* Return updated start address of string   */
}
/* **************************************************** */

/* **************************************************** */
/* Ensures a space before and after <>& characters      */
/* **************************************************** */
char *InsertSpaces(char *cmd)
{
    char cVal;
    char *sLoc;
    char *newCmd = (char *) malloc(2*MAX_BUFFER);       /* Ensure buffer is big enough to hold cmd w/ new spaces */
    char specialChar[] = "<>&";                         /* Special characters to insert spaces before and after  */
    *newCmd = '\0';                                     /* Terminate the empty string                            */
    sLoc  = strpbrk(cmd, specialChar);                  /* Points to first occurance of (<> or &)                */
    
    while(sLoc != NULL) {                               /* Repeat until no more <>& are found                    */
        cVal = Check4Special(sLoc);                     /* Save the type of character it is (<> or &)            */
        *sLoc = '\0';                                   /* Terminate the string                                  */
        sprintf(newCmd, "%s%s %c ", newCmd, cmd, cVal); /* Add spaces before and after the character             */
        cmd = sLoc+1;
        sLoc = strpbrk(cmd, specialChar);               /* Points to the next occurance of (<> or &)             */
    }
    
    if (*cmd != '\0')                                   /* If there are still characters in command              */
        sprintf(newCmd, "%s%s",newCmd,cmd);             /* Copy them to newCmd                                   */
    
    return newCmd;                                      /* Return the pointer                                    */
}
/* **************************************************** */
