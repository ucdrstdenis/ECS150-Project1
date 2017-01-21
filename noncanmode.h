#ifndef _NONCANMODE_H
#define _NONCANMODE_H

/* ************************************ */
/*  Originates from Joel's noncanmode.c */
/* ************************************ */
inline char GetChar (void);             /* Read one character from the keyboard                     */
void ResetCanMode (void);               /* Reset the terminal to the saved parameters               */
void ResetHandler (int signum);         /* Reset the terminal to the saved parameters (for signals) */
void SetNonCanMode (void);              /* Set the terminal to non-canonical mode                   */
/* ************************************ */

#endif
