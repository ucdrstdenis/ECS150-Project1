#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "common.h"
#include "history.h"

/* **************************************************** */
/* Displays next history entry in the command line      */
/* **************************************************** */
void DisplayNextEntry(History *history, char *cmdLine, int *cursorPos)
{
    if (history->count == 0 || history->traversed == MAX_HIST_ITEMS || history->traversed == history->count)
        ErrorBell();
    else {
        if (history->traversed == 0)
            history->current = history->top;
        else
            history->current = history->current->next;
        
        history->traversed++;
        ClearCmdLine(cmdLine, cursorPos);
        write(STDIN_FILENO, history->current->command, strlen(history->current->command));
        strcpy(cmdLine, history->current->command);
        *cursorPos = strlen(cmdLine);

    }
}
/* **************************************************** */

/* **************************************************** */
/* Displays previous history entry in the command line  */
/* **************************************************** */
void DisplayPrevEntry(History *history, char *cmdLine, int *cursorPos)
{	   
    if (history->count == 0 || history->traversed == 0)
        ErrorBell();
        
    else {
        history->traversed--;							
        history->current = history->current->prev;
        ClearCmdLine(cmdLine, cursorPos);
        
        if (history->traversed != 0) {
          write(STDIN_FILENO, history->current->command, strlen(history->current->command));
          strcpy(cmdLine, history->current->command);
          *cursorPos = strlen(cmdLine);
        }
    }
}
/* **************************************************** */

/* **************************************************** */
/* Removes the entry at the bottom of the history list  */
/* **************************************************** */
void RemoveLastEntry(History *history)
{
    Entry *current = history->top;
    Entry *previous = history->top;
    while (current->next != NULL) {
        previous = current;
        current = current->next;
    }
    free(current);
    previous->next = NULL;
}
/* **************************************************** */

/* **************************************************** */
/* Adds a new entry to the history list                 */
/* **************************************************** */
void AddHistory(History *history, char *cmdLine, int cmdLen)
{
    Entry *h = (Entry*) malloc(sizeof(Entry));
    h->command  = (char*) malloc(sizeof(char) * (cmdLen+1));
    strcpy(h->command, cmdLine);
    
    if (history->count == 0) {
        h->next = NULL;
        h->prev = NULL;
        history->top = h;											
    } else {
        history->top->prev = h;			
        h->next = history->top;
        h->prev = NULL;
        history->top = h;
    }
    
    if (history->count == MAX_HIST_ITEMS)
        RemoveLastEntry(history);
    else
        history->count++;
    
    history->current = NULL;
    history->traversed = 0;
}
/* **************************************************** */
