#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "common.h"

/* **************************************************** */
/*               Shell Print Characters                 */
/* **************************************************** */
const char *SHELL_PROMPT   = "sshell$ ";
const char *EXITLINE       = "Bye...\n";
const char *HELLO          = "Hello!\n";
const char *BELL           = "\a";
const char *NEWLINE        = "\r\n";
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
/* Prints the exit message.                             */
/* **************************************************** */
void SayGoodbye (void)
{
    write(STDERR_FILENO, EXITLINE, strlen(EXITLINE));
}
/* **************************************************** */
/* **************************************************** */
/* Prints the welcome message. Removed for auto-testing */
/* **************************************************** */
void SayHello (void)
{
//    write(STDERR_FILENO, HELLO, strlen(HELLO));
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
/* No Input File Error message                          */
/* **************************************************** */
void NoInputFile (void)
{
    ThrowError ("Error: no input file");
}
/* **************************************************** */
/* **************************************************** */
/* No Input File Error message                          */
/* **************************************************** */
void NoOutputFile (void)
{
    ThrowError ("Error: no output file");
}
/* **************************************************** */
/* **************************************************** */
/* Error: Invalid command message                       */
/* **************************************************** */
void InvalidCommand (void)
{
    ThrowError ("Error: invalid command line");
}
/* **************************************************** */
/* **************************************************** */
/* Error: mislocated input redirection                  */
/* **************************************************** */
void BadInputRedirect (void)
{
    ThrowError ("Error: mislocated input redirection");
}
/* **************************************************** */
/* **************************************************** */
/* Error: mislocated output redirection                 */
/* **************************************************** */
void BadOutputRedirect (void)
{
    ThrowError ("Error: mislocated output redirection");
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
/* Prints '+ completed' messages for piped commands     */
/* **************************************************** */
void CompleteChain (char *cmd, int *xArray)
{
    char msg[2*MAX_BUFFER];                             /* @TODO minimize this buffer size          */
    char len = (char)(sizeof(xArray)/8);                /* int* is 8 bytes, cast as a char          */
    int i = 0;
    sprintf(msg, "+ completed '%s' [%d]", cmd, xArray[i]);

    for(i = 1; i <= len; i++) 
	sprintf(msg, "%s[%d]", msg, xArray[i]);

    sprintf(msg, "%s\n", msg);
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
/* Checks if special character or not                   */
/* **************************************************** */
char Check4Special(char key)
{
    switch (key) {
        case '&': return '&';
        case '<': return '<';
        case '>': return '>';
        default: return 0;
    }
}
/* **************************************************** */
/* **************************************************** */
/* Strips trailing and leading whitespace.              */
/* **************************************************** */
char *RemoveWhitespace(char *string)
{
    int i = strlen(string) - 1;                         /* String length, -1 to acct for offset     */
    while (Check4Space(string[i])) string[i--] = '\0';  /* Remove trailing whitespace               */
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
/* **************************************************** */
/* Run dup2() and close(). Handle errors.               */
/* **************************************************** */
void Dup2AndClose(int old, int new)
{
    if (old != new) {                                   /* Check file descriptors are not the same     */
        if(dup2(old, new) != -1) {                      /* Check dup2() succeeds                       */
            if (close(old) == -1)                       /* Check close() doesn't fail                  */
                perror("close");                        /* Report the error if close fails             */
        } else {                                        /* If dup2() fails                             */
            perror("dup2");                             /* Report the error                            */
            exit(EXIT_FAILURE);                         /* Exit with failure                           */
        }
    }
}
/* **************************************************** */
/* **************************************************** */
/* Check for invalid placement of special characters    */
/* Set background flag if '&' is last character         */
/* Then remove '&' from the string                      */
/* **************************************************** */
char CheckCommand(char *cmd, char *isBackground)
{
    char s  = *cmd;                                     /* Get the first character in the array       */
    char *end = strchr(cmd, '\0')-1;                    /* Get the last character in the array        */
    switch (*end){                                      /* Check the character at the end             */
	    case '|':
            InvalidCommand();                           /* Invalid command line error                 */
		    return 1;                                   /* Bad command, return 1                      */

	    case '>':
		    if (s != *end) NoOutputFile();              /* If > is not the only character in the cmd  */
            else InvalidCommand();                      /* Otherwise Throw invalid command error      */
            return 1;                                   /* Bad command, return 1                      */

        case '<':
            if (s != *end) NoInputFile();               /* If > is not the only character in the cmd  */  
            else InvalidCommand();                      /* Otherwise Throw invalid command error      */
            return 1;                                   /* Bad command, return 1                      */
    }

    if (s == '|' || Check4Special(s)) {                 /* Check the character at the beginning       */
        InvalidCommand();
        return 1;
    }

    if (*end == '&') {                                  /* If '&' is last character                   */
        *isBackground = 1;                              /* Set the Background flag                    */
        *end = '\0';                                    /* Remove '&' from the command                */
    }
    return 0;
}
/* **************************************************** */

/* **************************************************** */
/* Searches the PATH variable for the location of       */
/* the specified program                                */
/* Uses the first entry in PATH that has valid entry    */
/*                                                      */
/*   Works excellent, but not needed if execvp() used.  */
/*                      * sigh *                        */
/*                                                      */
/* Example - *PATH = /usr/bin:/opt/bin                  */
/*           *prog = "ls"                               */
/*           returns "/usr/bin/ls"                      */
/* **************************************************** */
//char *SearchPath(char *prog) {
//    unsigned int len = strlen(prog);                  /* Length of the string of the passed program  */
//    char *binary  = (char *) malloc(MAX_BUFFER+len);  /* Pointer to hold the full name of the binary */
//    char *PATH = getenv("PATH");                      /* Store contents of the PATH variable         */
//    char *semi = strchr(PATH, ':');                   /* semi points to the first place ':' occurs   */
//    while(semi != NULL) {                             /* Repeat until no more ':' found              */
//        *semi = '\0';                                 /* Terminate the string where ':' was          */
//        sprintf(binary, "%s/%s", PATH, prog);         /* Append the first path to binary name        */
//        if(access(binary, F_OK) != -1)                /* If binary exists                            */
//            return binary;                            /* Return the full name of the binary          */
//        PATH = semi+1;                                /* Update the address PATH points to           */
//        semi = strchr(PATH, ':');                     /* semi points to the next place ':' occurs    */
//    }
//
//    sprintf(binary, "%s/%s", PATH, prog);             /* Append binary to last entry in path         */
//    if(access(binary, F_OK) != -1)                    /* If binary exists                            */
//        return binary;
//    else                                              /* If it doesn't exists                        */
//        binary = prog;                                /* Just store the argument that was passed     */
//    
//    return binary;                                    /* Return the binary name                      */
//}
/* **************************************************** */

/* **************************************************** */
/* Function to execute program commands                 */
/* If function is piped, ExecProgram() is recursive.    */
/*                                                      */
/* Excellent recursive function, does the work of 	    */
/* 6 functions in < 35 lines of code. But without using */
/* shared mem or writing to a file, could not get   	*/
/* child exit status' back. Will save for another time. */
/* **************************************************** */
//void ExecProgram(char **cmds[], int N, Process *P)
//{
//    if (cmds[N+1] == NULL) {                          /* If there's only 1 command in the array   */
//        Dup2AndClose(P->fd[0], STDIN_FILENO);         /* Read from fd[0]                          */
//        Dup2AndClose(P->fd[1], STDOUT_FILENO);        /* Write to fd[1]                           */
//        execvp(cmds[N][0], cmds[N]);                  /* Execute command                          */
//        perror("execvp");                             /* Coming back here is an error             */
//        exit(EXIT_FAILURE);                           /* Exit failure                             */       
//    } else {
//        int fdOut[2];                                 /* Create file descriptors                  */
//        Process *cP;                                  /* Pointer to new child process             */
//        pipe(fdOut);                                  /* Create pipe                              */
//        cP = AddProcessAsChild(processList, P->PID, fork(), "\0", P->nPipes, P->isBG, fdOut);
//        switch(cP->PID) {                             /* fork the process                         */
//            case -1:                                  /* If fork fails                            */
//                perror("fork");                       /* Report the error                         */
//                exit(EXIT_FAILURE);                   /* Exit with failure                        */
//            case 0:                                   /* Child Process, writes to the pipe        */
//                close(fdOut[0]);                      /* Don't need to read from pipe             */
//                Dup2AndClose(P->fd[0], SI);           /* Link Input file descriptor to the pipe   */
//                Dup2AndClose(fdOut[1], SO);           /* Link output file descripter to STDOUT    */
//                execvp(cmds[N][0], cmds[N]);          /* Execute the command                      */
//                perror("execvp");                     /* Coming back here is an error             */
//                exit(EXIT_FAILURE);                   /* Exit failure                             */
//            default:                                  /* Parent Process, reads from the  pipe     */
//                close(fdOut[1]);                      /* Don't need to write to pipe              */
//                close(P->fd[0]);                      /* Close existing input file descriptor     */
//                Wait4Me(cP);                          /* Blocking or non-blocking wait            */
//                ExecProgram(cmds, N+1, cP);           /* Execute the first command in the array   */
//        }
//    }
//}
/* **************************************************** */
