#ifndef _SSHELL_H
#define _SSHELL_H

/* ************************************ */
/*       Shell Print Characters         */
/* ************************************ */
const char *SHELL_PROMPT = "sshell$ ";
const char *BELL = "\a";
const char *NEWLINE = "\r\n";
const char *EXITLINE = "\r\nBye...\n";
const char *BACKSPACE_CHAR = "\b \b";
/* ************************************ */

/* ************************************ */
/*               SShell                 */
/* ************************************ */
void InitShell (History *history, int *cursorPos);  /* Initialize the shell and relevant objects            */
char RunCommand (char *cmdLine);
char *RemoveWhitespace (char *string);              /* Stripes trailing or leading whitespace from a string */
char **Cmd2Array (char *cmd);                       /* Breaks up  a command into an array of arguments      */
char ***Pipes2Array (char *cmd);                    /* Breaks up command into arrays of piped arguments     */     
/* ************************************ */

#endif
