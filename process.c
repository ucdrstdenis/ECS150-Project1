#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

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
    Process *curr;
    Process *me = (Process*) malloc(sizeof(Process));
    me->cmd     = (char*) malloc(strlen(cmd)+1);        /* Alloc space for the cmd                  */
    me->PID     = PID;                                  /* Set the PID                              */
    me->status  = 0;                                    /* exit code                                */
    me->running = 1;                                    /* 1 if running, 0 if complete              */
    me->isBG    = isBG;                                 /* 1 if background command, 0 otherwise     */
    me->nPipes  = nPipes;                               /* Number of pipes in the command           */
    me->child   = NULL;	                       	        /* NULL if no children processes            */
    me->parent  = NULL;                                 /* @TODO Should be set to main shell PID    */
    me->fd[0]   = fd[0];                                /* Input file descriptor                    */
    me->fd[1]   = fd[1];                                /* Output file descriptor                   */
    me->printMe = 1;                                    /* By default, print '+ completed' messages */
    strcpy(me->cmd, cmd);                               /* Copy the command string                  */
    me->next = NULL;                                    /* No newer nodes in list yet 	            */
    if (pList->top == NULL)                             /* Setup pointer to next process in list    */
        pList->top = me;
    else {                                              /* If already entries in the list 	    */
        curr = pList->top;
        while (curr->next != NULL) curr = curr->next;   /* Go to the end of the list                */
        curr->next = me;                                /* Set the next pointer to the new process  */
   }

    pList->count++;                                     /* Increment count of processes in the list */
    return me;
}
/* **************************************************** */

/* **************************************************** */
/* Add a process as a child of another processs         */
/* **************************************************** */
Process *AddProcessAsChild(ProcessList *pList, Process *P, pid_t cPID, char *cmd)
{
    Process *child = (Process*) AddProcess(pList, cPID, cmd, P->nPipes, P->isBG, P->fd);
    P->child = child;                                   /* Mark the process as "child" of parent    */
    child->parent = P;                                  /* Mark the parent of the "child"           */
    return child;                                       /* Return the pointer                       */
}
/* **************************************************** */

/* **************************************************** */
/* Prints '+ completed' messages for piped commands     */
/* **************************************************** */
void CompleteChain (Process *P, int *xArray)
{
    char msg[2*MAX_BUFFER];                             /* @TODO minimize this buffer size          */
    int i = 0;
    sprintf(msg, "+ completed '%s' [%d]", P->cmd, xArray[i]);

    while(++i < P->nPipes) 
	sprintf(msg, "%s[%d]", msg, xArray[i]);

    sprintf(msg, "%s\n", msg);
    write(STDERR_FILENO, msg, strlen(msg));
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
    int *status = (int *)malloc(4*(P->nPipes));         /* Allocate space for status array  */
    while(My->child != NULL) {                          /* Iterate through children         */
	    status[i++] = My->child->status;            /* Add the value to the array       */
	    if (My->child->child == NULL) break;        
	    P->child = CopyDelete(My->child, My->child->child);       
        P->child->parent = P;                      
    }
    status[i] = P->status;                              /* Parent is always last command    */
    P->next = My->child->next;                          /* Remove pointer from the list     */
    P->child = NULL;                                    /* Deleted all the children         */
    free(My->child);                                    /* Free the child -delete from list */
    if(processList->count) processList->count--;        /* Decrement the process count      */
    return status;                                      /* Return the pointer               */
}
/* **************************************************** */
/* **************************************************** */
/* Check if chained process's children have finished    */
/* Return 1 if all finished, 0 if any still running     */
/* **************************************************** */
char CheckChildrenDone(Process *My)
{
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
   
    while (curr != NULL) {                              /* Iterate through the list                     */
        if ((curr->running==0)&&(curr->parent==NULL)){  /* If process completed, and no children exist  */
            if (curr->nPipes > 1) {                     /* If it's a chained process                    */
                if(CheckChildrenDone(curr)) {           /* Check all children completed                 */
                    stArray = GetChainStatus(curr);     /* Save exit status, delete all                 */
                    CompleteChain(curr, stArray);       /* Print completed message                      */
                }
            }
            else if(curr->printMe) {                    /* Otherwise,not piped, check print enabled     */
                CompleteCmd(curr->cmd, curr->status);   /* If it is, print + completed message          */
            }
	        
            if (curr->next != NULL) {                   /* If there are more processes in the list      */
                CopyDelete(curr, curr->next);           /* Copy curr->next to curr and delete next      */
                if (prev == NULL)                       /* If this is the top node in the list          */
		            pList->top = curr;          /* Assign the top pointer to the current node   */
	        }
           
            else {                                      /* Otherwise, no more processes in the list     */
                free(curr);                             /* So free the node                             */
		        pList->top = NULL;              /* Point the top to NULL                        */
		        if(pList->count) pList->count--;/* Decrement the process count, prevent -1      */
	    	    break;                              /* Break from the while  loop                   */
            }
            if(pList->count) pList->count--;            /* Decrement the process count, loop again      */
        } else {                                        /* No completed processes found yet, keep going */
            prev = curr;            
            curr = curr->next;    
        }                                               /* End if process completed with no children    */
    }						        /* End while loop 				*/
}

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
/* **************************************************** */
/* Copy and delete a process from the list              */
/* **************************************************** */
Process *CopyDelete(Process  *To, Process *From)
{
    if (From !=NULL) {                                  /* Don't do anything if From node is NULL 	*/
        To->cmd     = From->cmd;                        /* Copy the command string                      */
        To->PID     = From->PID;                        /* Copy the PID                                 */
        To->status  = From->status;                     /* Copy the exit status                         */
        To->running = From->running;                    /* Copy the state                               */	 			     
        To->isBG    = From->isBG;                       /* Copy the background flag                     */
        To->nPipes  = From->nPipes;                     /* Copy the number of piped commands            */
        To->child   = From->child;                      /* Copy the pointer to a child                  */             
        To->parent  = NULL;                             /* Avoid segfaults, set this later if needed    */
        To->fd[0]   = From->fd[0];                      /* Copy the input file descriptor               */        
        To->fd[1]   = From->fd[1];                      /* Copy the input file descriptor               */     
        To->printMe = From->printMe;                    /* Copy the print settings descriptor 		*/    
        To->next    = From->next;                       /* Copy the pointer to the next node in list    */
        free(From);                                     /* Delete the From node           		*/
	    if(processList->count)                      /* Prevent from becoming -1                     */
            processList->count--;                       /* Decrement the process count                  */
    }
    return To;                                          /* Return the copy of the process 	        */
}
/* **************************************************** */
