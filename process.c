#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

/* **************************************************** */
/*              User - defined .h files                 */
/* **************************************************** */
#include "process.h"					/* Function prototype and structs for process.c   */
#include "common.h"                                     /* Keystrokes and common functions                */
/* **************************************************** */

/* **************************************************** */
/* Add a process to the list of background processes    */
/* **************************************************** */
void AddProcess(BackgroundProcessList *pList, pid_t PID, char *cmd) {
    Process *p = (Process*) malloc(sizeof(Process));
    p->cmd = (char*) malloc(strlen(cmd)+1);
    p->PID = PID;
    p->status = 0;
    p->running = 1;
    strcpy(p->cmd, cmd);

    if (pList->count == 0) {
        p->next = NULL;
        p->prev = NULL;
        pList->top = p;
    } else {
        pList->top->prev = p;
        p->next = pList->top;
        p->prev = NULL;
        pList->top = p;
    }
    pList->count++;
}
/* **************************************************** */

/* **************************************************** */
/* Check if any processes have completed                */
/* Print completed message if they have                 */
/* **************************************************** */
void CheckCompletedProcesses(BackgroundProcessList *pList) {
    Process *current = pList->top;
    while (current != NULL) {
        if (current->running == 0) {                    /* If process has completed */
            CompleteCmd(current->cmd, current->status); /* Print completed message */     
            // @TODO the current node should be freed at some point
            if ((current->prev == NULL) && (current->next == NULL))    
                pList->top = NULL;        
            if (current->prev != NULL)   
                current->prev->next = current->next;
            if (current->next != NULL) {                /* Then remove the node  from the list */
                current->next->prev = current->prev;
                current = current->prev;
            }    
        }   
        current = current->next;  
    }
}
/* **************************************************** */

/* **************************************************** */
/* Mark process with matching PID as completed          */
/* **************************************************** */
void MarkProcessDone(BackgroundProcessList *pList, pid_t PID, int status) {
    Process *current = pList->top;                      
    while(current != NULL) {                            /* Iterate through the process list */
        if (current->PID == PID) {                      /* Check to find the PID = completed PID */
            current->running = 0;
            current->status = status;
            return;        
        }
        current = current->next;
    }
}
/* **************************************************** */
