#ifndef _PROCESS_H
#define _PROCESS_H

/* **************************************************** */
/*                Process Structures                    */
/* **************************************************** */
typedef struct Process {                                /* Process Node                             */
    pid_t PID;	                                        /* PID of command that was run              */
    char *cmd;                                          /* command that was executed                */
    char running;                                       /* 1 if running, 0 if complete              */
    char status;                                        /* Completion Status when process completed */
    struct Process *next;                               /* points to next process in list           */
} Process;

typedef struct ProcessList {                  		/* Maintains list of background processes   */
    unsigned int count;                                 /* Number of outstanding processes          */
    Process *top;                                       /* Process List                             */
} ProcessList;
/* **************************************************** */

/* **************************************************** */
/*                Global Structures                     */
/* **************************************************** */
ProcessList *processList; 
/* **************************************************** */

/* **************************************************** */
/*                       Process                        */
/* **************************************************** */
char MarkProcessDone(ProcessList *pList, pid_t PID, int status);   /* Mark process with matching PID as completed          */
void CheckCompletedProcesses(ProcessList *pList);                  /* Check if any processes have completed                */
void AddProcess(ProcessList *pList, pid_t PID, char *cmd);         /* Add a process to the list of background processes    */
/* **************************************************** */

#endif
