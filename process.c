#include <stdlib.h>
#include <string.h>

/* **************************************************** */
/*              User - defined .h files                 */
/* **************************************************** */
#include "process.h"                                    /* Function prototype and structs for process.c   */
#include "common.h"                                     /* Keystrokes and common functions                */
/* **************************************************** */

/* **************************************************** */
/* Add a process to the list of background processes    */
/* **************************************************** */
void AddProcess(ProcessList *pList, pid_t PID, char *cmd) {
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
/* @TODO the current node should be freed at some point */
/* @TODO eliminate use of "prev" node since not needed  */
/* **************************************************** */
void CheckCompletedProcesses(ProcessList *pList) {
    Process *current = pList->top;
    while (current != NULL) {
        if (current->running == 0) {                    /* If process has completed */
            CompleteCmd(current->cmd, current->status); /* Print completed message */
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
/* return 1 if matching PID in list, 0 otherwise        */
/* **************************************************** */
char MarkProcessDone(ProcessList *pList, pid_t PID, int status) {
    Process *current = pList->top;                      
    while(current != NULL) {                            /* Iterate through the process list */
        if (current->PID == PID) {                      /* Check to find the PID = completed PID */
            current->running = 0;
            current->status = status;
            return 1;
        }
        current = current->next;
    }
    return 0;                                           /* Process was not in the list */
}
/* **************************************************** */
