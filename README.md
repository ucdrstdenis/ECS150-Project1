# ECS150-Project1 #
A simple shell written in c. Better description here.

# Features #
Look through the Project 1 instructions, and basically re-iterate what is there, but salesmanshipy.
Supports output redirection via '>'

Supports input redirection via '<'

Handles background processes via '&'

Supports command piplining via '|'

Handles exit codes'

Tracks background processes using a unique data structure

Automatically searches the PATH using execvp and SearchPath()

Tracks command line history and allows easy viewing with the Up/Down Arrow keys.

Built in commands 'exit', 'cd', and 'pwd''

Etc. I'm pretty sure we meet every requirement, but someone should double check'


# SShell Rundown #
A basic overview of how this program works. 

main(), located in sshell.c does 3 things - Initialize the shell with ShellInit(), process the keystroke, and handle exiting the application.
ShellInit() does 4 things - alloc/init the local history structure, alloc/init the global process structure, and define the SIGCHLD interrupt handler. It also sets the terminal to non-cannonical mode via Joel's routines in noncanmode.c (with our added noncanmode.h file).

Keystroke processing is very straight forward - when a user presses a key, the keystroke is written to STDOUT and copied to a local buffer. Up/Down arrows call DisplayNextEntry() and DisplayLastEntry() from [history.h/.c], TAB, LEFT, and RIGHT arrow keys call the ErrorBell() function to make the shell go BOOP.

When a user presses the RETURN key, 3 things happen, in order:
1) The contents of the command line are added to the shells history.
2) The command is processed with the RunCommand() wrapper routine.
3) The process list is checked for any processes that may have completed, and if they have, it prints thier +completed message to STDERR and removes them from the list.

The RunCommand() routine does 3 things:
1) Performs initial layer of command checking.
2) Parses the command into a ***char array, based on the pipe '|' characters.  For example, if the command "ls -la|grep common> outfile" would be transformed into "{{"ls","-la",NULL},{"grep", "common", ">", "outfile", NULL},NULL}. This is done with the Pipes2Arrays() and Cmd2Array() routines.
3) The command is checked for built-in calls which are 'exit' cd' and 'pwd'. pwd also handles output redirects. If the command is not built in, it calls ExecProgram() which executes the commands, and uses a while loop to chain commands together if they are piped. It also checks the command arrays for I/O redirects with a call to CheckRedirects(), which calls SetupRedirects() to return the I/O file descriptors and to do second and third level error checking.  (i.e checking the output file descriptor against any pipes the output may need to be sent to).

At this point, it's worth introducing the process structure, since this is what gets passed around from function to function.

Although the process structures'  [located in process.c/.h] main objective was to handle background routines, it also evolved into a convenient way to handle program execution, file redirecting, and command pipelining, since the I/O file descriptors, background flags, command contents, and more, can all be stored in the Process object, whose main constructor is AddProcess(). 







## Header Files (API) ##
1. common.h - Is there a way we can just copy-past the code and make it look nice for this section?
2. history.h -
3. noncanmode.h - 
4. sshell.h  - 


### References ###
1. [Example of linked list history](http://stackoverflow.com/questions/20588556/linked-list-implementation-to-store-command-history-in-my-shell)

2. [Makefile Reference 1](http://stackoverflow.com/questions/1484817/how-do-i-make-a-simple-makefile-for-gcc-on-linux)

3. [Makefile Reference 2](http://mrbook.org/blog/tutorials/make/)

4. [GitHub Markdown Cheatsheet](https://github.com/adam-p/markdown-here/wiki/Markdown-Cheatsheet)

### Unused References ###
1. [Example of recursive piping](https://gist.github.com/zed/7835043)
