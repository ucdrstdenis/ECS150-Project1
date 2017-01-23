#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "common.h"

/* Shell print characters */
extern const char *SHELL_PROMPT;    /* Defined in sshell.h */
extern const char *BELL;            /* Defined in sshell.h */
extern const char *NEWLINE;         /* Defined in sshell.h */
extern const char *EXITLINE;        /* Defined in sshell.h */
extern const char *BACKSPACE_CHAR;  /* Defined in sshell.h */

/* Sound Bell noise */
void ErrorBell(void)
{
    write(STDERR_FILENO, BELL, 1);
}


/* Displace the main sshell$ prompt */
void DisplayPrompt(int *cursorPos, char newline)
{
    if (newline)
        write(STDOUT_FILENO, NEWLINE, strlen(NEWLINE));
    
    write(STDOUT_FILENO, SHELL_PROMPT, strlen(SHELL_PROMPT));
    *cursorPos = 0;
}

/* Clear the current cmdLine buffer and STDIN */
void ClearCmdLine(char *cmdLine, int *cursorPos)
{
    while (*cursorPos) {
        write(STDIN_FILENO, BACKSPACE_CHAR, strlen(BACKSPACE_CHAR));
        *cursorPos -= 1;
    }
}

/* Print a message to the shell, used in debugging */
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
/* ************************************ */
