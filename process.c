#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

/* **************************************************** */
/*              User - defined .h files                 */
/* **************************************************** */
#include "process.h"                                    /* Function prototype and structs for process.c   */
#include "common.h"                                     /* Keystrokes and common functions                */
/* **************************************************** */

/* **************************************************** */
/* Add a process to the list of running processes       */
/* **************************************************** */
Process *AddProcess(ProcessList *pList, pid_t PID, char *cmd, char nPipes, char isBG, int *fd)
{
    Process *proc = (Process*) malloc(sizeof(Process));
    proc->cmd     = (char*) malloc(strlen(cmd)+1);      /* Alloc space for the cmd                  */
    proc->PID     = PID;                                /* Set the PID                              */
    proc->status  = 0;                                  /* exit code                                */
    proc->running = 1;		 			                /* 1 if running, 0 if complete              */
    proc->isBG    = isBG;                               /* 1 if background command, 0 otherwise     */
    proc->nPipes  = nPipes;                             /* Number of pipes in the command           */
    proc->child   = NULL;	                            /* NULL if no children processes            */
    proc->parent  = NULL;                               /* @TODO Should be set to main shell PID    */
    strcpy(proc->cmd, cmd);                             /* copy the command string                  */
    
    if (fd == NULL){                                    /* Setup the I/O file descriptors           */
        proc->fd[0] = STDIN_FILENO;
        proc->fd[1] = STDOUT_FILENO;
    } else {
        proc->fd[0] = fd[0];
        proc->fd[1] = fd[1];
    }

    if (pList->count == 0) {                            /* Setup pointer to the next process in list    */
        proc->next = NULL;
        pList->top = proc;
    } else {
        proc->next = pList->top;
        pList->top = proc;
    }
    pList->count++;                                     /* Increment the count of processes in the list */
    return pList->top;
}
/* **************************************************** */

/* **************************************************** */
/* Add a process as a child of another processs         */
/* **************************************************** */
Process *AddProcessAsChild(ProcessList *pList, pid_t pPID, pid_t cPID, char *cmd, char nPipes, char isBG, int *fd)
{
    Process *child = (Process*) AddProcess(pList, cPID, cmd, nPipes, isBG, fd);
    Process *parent = pList->top;
    while (parent != NULL) {
        if (parent->PID == pPID) {
            parent->child = child;
            child->parent = parent;
            return child;
        }
        parent = parent->next;
    }
    return NULL;
}
/* **************************************************** */

/* **************************************************** */
/* Check if any processes have completed                */
/* Print completed message if they have                 */
/* **************************************************** */
void CheckCompletedProcesses(ProcessList *pList)
{
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
            pList->count--;                             /* Decrement the process count  */		
	        break;                                      /* Break from the loop          */
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
char MarkProcessDone(ProcessList *pList, pid_t PID, int status)
{
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
