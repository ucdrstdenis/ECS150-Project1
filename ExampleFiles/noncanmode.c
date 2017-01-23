#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

/* Read one char from the keyboard */
static inline char get_one_char(void)
{
	int res = 0;
	char c;

	while (res <= 0) {
		res = read(STDIN_FILENO, &c, 1);

		if (res < 0) {
			if (errno == EINTR)
				/* read() was interrupted, try again */
				continue;
			/* Otherwise, it's a failure */
			return -1;
		}
	}
	return c;
}

/*
 * Non-canonical mode management
 */
static struct termios saved_termios;
static pid_t shell_pid;

/* Reset the terminal to the saved parameters */
static void termios_reset(void)
{
	tcsetattr(STDOUT_FILENO, TCSANOW, &saved_termios);
}

/* Reset the terminal to the saved parameters (for signals) */
static void termios_reset_sig(int signum)
{
	if (getpid() == shell_pid)
		termios_reset();
	exit(1);
}

/* Set the terminal in non-canonical mode */
static void termios_set_noncanonical(void)
{
	struct termios attr;
	struct sigaction act;
	int fd = STDOUT_FILENO;
	int err = 0;

	/* Nothing to do if we're not in a terminal */
	if (!isatty(fd))
		return;

	/* Save current attributes */
	tcgetattr(fd, &saved_termios);

	/* Set new "raw" attributes */
	tcgetattr(fd, &attr);
	attr.c_lflag &= ~ICANON;	/* Disable canonical mode */
	attr.c_lflag &= ~ECHO;		/* Disable character echoing */
	attr.c_cc[VMIN] = 1;		/* Read at least one character */
	attr.c_cc[VTIME] = 0;		/* No timeout */
	tcsetattr(fd, TCSAFLUSH, &attr);

	/* Make sure that we restore the previous mode upon exit */
	shell_pid = getpid();
	act.sa_handler = termios_reset_sig;
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

int main(int argc, char **argv)
{
	char c;

	/* Switch to non-canonical terminal mode */
	termios_set_noncanonical();

	/* Shell main loop */
	while (1) {
		 c = get_one_char();
		 if (c == 0x04) {
			 /* Ctrl-D */
			 break;
		 } else {
			 if (isprint(c))
				 printf("RX: '%c' 0x%02X\n", c, c);
			 else
				 printf("RX: ' ' 0x%02X\n", c);
		 }
	}

	/* Switch back to previous terminal mode */
	termios_reset();

	return EXIT_SUCCESS;
}

