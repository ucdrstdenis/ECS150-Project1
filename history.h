#ifndef _HISTORY_H
#define _HISTORY_H

/* **************************************************** */
/*                   History Structures                 */
/* **************************************************** */
typedef struct Entry {                                  /* History Entry item                               */
    char *command;                                      /* command to save                                  */
    struct Entry *next;                                 /* pointer to next entry                            */
    struct Entry *prev;                                 /* pointer to previous entry                        */
} Entry;

typedef struct History {
    unsigned int count;                                 /* No. items in history linked list                 */
    unsigned int traversed;                             /* Keeps track of currently traversed entries       */
    Entry *top;                                         /* Most recent entry                                */
    Entry *current;                                     /* Entry currently being viewed via. up/down arrows */
} History;

/* **************************************************** */
/*                   History Functions                  */
/* **************************************************** */
void AddHistory(History *history, char *cmdLine, int cmdLen); 	        /* Adds a new entry to the history list                */
void DisplayNextEntry(History *history, char *cmdLine, int *cursorPos); /* Displays next history entry in the command line     */
void DisplayPrevEntry(History *history, char *cmdLine, int *cursorPos); /* Displays previous history entry in the command line */
void RemoveLastEntry(History *history);                                 /* Removes the entry at the bottom of the history list */
/* **************************************************** */

#endif
