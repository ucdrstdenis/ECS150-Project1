#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

#include "noncanmode.h"

/* ************************************ */
/* Originates from Joel's noncanmode.c  */
/* ************************************ */

static struct termios savedParameters;	/* Non-canonical mode management                            */
static pid_t shell_pid;                 /* PID of shell                                             */

/* Read one character from the keyboard */
inline char GetChar(void)
{
    char RxChar;
    int result = 0;
    while (result <= 0) {
        result = read(STDIN_FILENO, &RxChar, 1);
        if (result < 0) {
            if (errno == EINTR)
                continue;          /* read() was interrupted, try again */
            return -1;             /* Otherwise, it's a failure */
        }
    }
    return RxChar;
}
/* Reset the terminal to the saved parameters */
void ResetCanMode(void)
{
    tcsetattr(STDOUT_FILENO, TCSANOW, &savedParameters);
}

/* Reset the terminal to the saved parameters (for signals) */
void ResetHandler(int signum)
{
    if (getpid() == shell_pid)
        ResetCanMode();
    exit(1);
}

/* Set the terminal in non-canonical mode */
void SetNonCanMode(void)
{
    struct termios attr;
    struct sigaction act;
    int fd = STDOUT_FILENO;
    int err = 0;
    
    /* Nothing to do if we're not in a terminal */
    if (!isatty(fd))
        return;
    
    /* Save current attributes */
    tcgetattr(fd, &savedParameters);
    
    /* Set new "raw" attributes */
    tcgetattr(fd, &attr);
    attr.c_lflag &= ~ICANON;	/* Disable canonical mode */
    attr.c_lflag &= ~ECHO;      /* Disable character echoing */
    attr.c_cc[VMIN] = 1;        /* Read at least one character */
    attr.c_cc[VTIME] = 0;       /* No timeout */
    tcsetattr(fd, TCSAFLUSH, &attr);
    
    /* Make sure that we restore the previous mode upon exit */
    shell_pid = getpid();
    act.sa_handler = ResetHandler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    err += sigaction(SIGINT, &act, NULL);
    err += sigaction(SIGHUP, &act, NULL);
    err += sigaction(SIGTERM, &act, NULL);
    err += sigaction(SIGQUIT, &act, NULL);
    if (err) {
        perror("sigaction");
        exit(1);
    }
}
/* ************************************ */
