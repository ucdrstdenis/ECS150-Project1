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
    Process *proc = (Process*) malloc(sizeof(Process));
    proc->cmd = (char*) malloc(strlen(cmd)+1);
    proc->PID = PID;
    proc->status = 0;
    proc->running = 1;
    strcpy(proc->cmd, cmd);

    if (pList->count == 0) {
        proc->next = NULL;
        pList->top = proc;
    } else {
        proc->next = pList->top;
        pList->top = proc;
    }
    pList->count++;                                     /* Increment the count of processes in the list */
}
/* **************************************************** */

/* **************************************************** */
/* Check if any processes have completed                */
/* Print completed message if they have                 */
/* **************************************************** */
void CheckCompletedProcesses(ProcessList *pList) {
    Process *current = pList->top;
    Process *previous = NULL;
    while (current != NULL) {                           /* Iterate through the list 	*/
        if (current->running == 0) {                    /* If process has completed 	*/
            CompleteCmd(current->cmd, current->status); /* Print completed message 	    */
	    
	        if (previous == NULL)                       /* Prepare to delete the node 	*/
                pList->top = current->next;
	        else
                previous->next = current->next;	      
	        free(current);                              /* Delete the node              */			
	        break;                                      /* Break from teh loop          */
        }   
	    previous = current;
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
