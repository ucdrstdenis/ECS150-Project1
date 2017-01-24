#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "common.h"

/* **************************************************** */
/*               Shell Print Characters                 */
/* **************************************************** */
const char *SHELL_PROMPT = "sshell$ ";
const char *BELL = "\a";
const char *NEWLINE = "\r\n";
const char *EXITLINE = "Bye...\n";
const char *BACKSPACE_CHAR = "\b \b";
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
    char *newMsg = (char *) malloc(strlen(msg)+2);
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
/* Prints the backspace character to STDOUT             */
/* **************************************************** */
void PrintBackspace (void)
{
    write(STDOUT_FILENO, BACKSPACE_CHAR, strlen(BACKSPACE_CHAR));
}
/* **************************************************** */

/* **************************************************** */
/* Prints the newline character to STDOUT               */
/* **************************************************** */
void PrintNL (void)
{
    write(STDOUT_FILENO, NEWLINE, strlen(NEWLINE));
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
/* Prints '+ completed' messages to STDERR              */
/* **************************************************** */
void CompleteCmd (char *cmd, int exitCode)
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
    if ((key == ' ') || (key == '\t') || (key == '\n'))
        return 1;
    else
        return 0;
}
/* **************************************************** */

/* **************************************************** */
/* Checks if special character of not                   */
/* **************************************************** */
char Check4Special(char key)
{
    switch (key) {
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
    int i = strlen(string) - 1;                         /* String length, -1 to acct for offset     */
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
    char *newCmd = (char *) malloc(2*MAX_BUFFER);       /* Ensure buffer is big enough to hold cmd w/ new spaces */
    char specialChar[] = "<>&";                         /* Special characters to insert spaces before and after  */
    char *sLoc  = strpbrk(cmd, specialChar);            /* Points to first occurance of (<> or &)                */
    
    while(sLoc != NULL) {                               /* Repeat until no more <>& are found                    */
        cVal = Check4Special(*sLoc);                    /* Save the type of character it is (<> or &)            */
        *sLoc = '\0';                                   /* Terminate the string                                  */
        sprintf(newCmd, "%s%s %c ", newCmd, cmd, cVal); /* Add spaces before and after the character             */
        cmd = sLoc+1;
        sLoc = strpbrk(cmd, specialChar);               /* Points to the next occurance of (<> or &)             */
    }
    
    if (*cmd != '\0')	                                /* If there are still characters in command              */
        sprintf(newCmd, "%s%s",newCmd,cmd);             /* Copy them to newCmd                                   */

    return newCmd;                                      /* Return the pointer                                    */
}
/* **************************************************** */
