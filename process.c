#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* **************************************************** */
/*              User - defined .h files                 */
/* **************************************************** */
#include "process.h"                                    /* Process structures and methods           */
#include "common.h"                                     /* Keystrokes and common functions          */
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
    proc->fd[0]   = fd[0];                              /* Input file descriptor                    */
    proc->fd[1]   = fd[1];                              /* Output file descriptor                   */
    proc->printMe = 1;                                  /* By default, print '+ completed' messages */
    strcpy(proc->cmd, cmd);                             /* copy the command string                  */
   
    if (pList->count == 0) {                            /* Setup pointer to next process in list    */
        proc->next = NULL;
        pList->top = proc;
    } else {
        proc->next = pList->top;
        pList->top = proc;
    }
    pList->count++;                                     /* Increment count of processes in the list */
    return pList->top;
}
/* **************************************************** */

/* **************************************************** */
/* Add a process as a child of another processs         */
/* **************************************************** */
Process *AddProcessAsChild(ProcessList *pList, Process *Parent, pid_t cPID, char *cmd, char nPipes, char isBG, int *fd)
{
    Process *child = (Process*) AddProcess(pList, cPID, cmd, nPipes, isBG, fd);
    pList->count--;                                     /* Don't let child processes affect the count */
    Parent->child = child;
    child->parent = Parent;
    return child;
}
/* **************************************************** */

/* **************************************************** */
/* Record the status of each chained process and free   */
/* it from the list. Return pointer to status array     */
/* **************************************************** */
int *GetChainStatus(Process *P)
{
    int i = 0;
    Process *My = P;
    int *status = (int *) malloc(8*(P->nPipes+1));   	/* Allocate space for status array  */
    while(My->child != NULL) {                          /* Iterate through children         */
	status[i++] = My->child->status;                    /* Add the value to the array       */
        P->next = My->child->next;                      /* Remove the pointer from the list */
        My = My->child;                                 /* Update pointer                   */
    }
    status[i] = P->status;                              /* Last child in the list           */
    P->next = My->next;                                 /* Remove pointer from the list     */
    //free(My);                                         /* Free the child (delte from list  */
    return status;                                      /* Return the pointer               */
}
/* **************************************************** */
/* **************************************************** */
/* Check if chained process's children have finished    */
/* Return 1 if all finished, 0 if any still running     */
/* **************************************************** */
char CheckChildrenDone(Process *P)
{
    Process *My = P;
    while(My->child != NULL) {                          /* Iterate through children         */
        if (My->child->running) return 0;               /* Still running, return 0          */
        My = My->child;                                 /* Update the pointer               */
    }
    if (My->running) return 0;                          /* Last child still running         */
    return 1;                                           /* Otherwise all done               */
}
/* **************************************************** */

/* **************************************************** */
/* Check if any processes have completed                */
/* Print completed message if they have                 */
/* **************************************************** */
void CheckCompletedProcesses(ProcessList *pList)
{
    int *stArray;
    Process *curr = pList->top;
    Process *prev = NULL;
    
    while (curr != NULL) {                             /* Iterate through the list                      */
        if ((curr->running==0)&&(curr->parent==NULL)){ /* If process completed, and no children exist   */
            if (curr->nPipes > 1) {                    /* If it's a chained process                     */
                if(CheckChildrenDone(curr)) {          /* Check all children completed                  */
                    stArray = GetChainStatus(curr);    /* Save exit status, delete all                  */
                    CompleteChain(curr->cmd, stArray); /* Print completed message                       */
                }
            } 
	    else                                            /* Otherwise, its not piped                     */
            if (curr->printMe)                  
                CompleteCmd(curr->cmd, curr->status);   /* Print completed message                      */

        if (prev == NULL)  
		    pList->top = curr->next;                    /* Prepare to delete the node                   */
	    else 
		    prev->next = curr->next;
                	      
	    free(curr);                                     /* Delete the node                              */
            pList->count--;                             /* Decrement the process count                  */
	    break;                                          /* Break from the loop                          */
        }   
	    prev = curr;
        curr = curr->next;  
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
    while(current != NULL) {                            /* Iterate through the process list             */
        if (current->PID == PID) {                      /* Check to find the PID = completed PID        */
            current->running = 0;
            current->status = status;
            return 1;
        }
        current = current->next;
    }
    return 0;                                           /* Process was not in the list                  */
}
/* **************************************************** */
