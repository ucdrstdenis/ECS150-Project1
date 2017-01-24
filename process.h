#ifndef _PROCESS_H
#define _PROCESS_H

/* **************************************************** */
/*                Process Structures                    */
/* **************************************************** */
typedef struct Process {                                /* Process Node                             */
    pid_t PID;	                                        /* PID of command that was run              */
    char running;                                       /* 1 if running, 0 if complete              */
    char isBG;                                          /* 1 if background command, 0 otherwise     */
    char *cmd;                                          /* command that was executed                */
    int status;                                         /* Completion status when process completed */
    int fdIn;                                           /* Input file descriptor                    */
    struct Process *next;                               /* points to next process in list           */
    struct Process *child;                              /* Points to child process if it exists     */
} Process;

typedef struct ProcessList {                            /* Maintains list of running processes      */
    unsigned int count;                                 /* Number of outstanding processes          */
    Process *top;                                       /* Process List                             */
} ProcessList;
/* **************************************************** */

/* **************************************************** */
/*                  Global Structures                   */
/* **************************************************** */
ProcessList *processList; 
/* **************************************************** */

/* **************************************************** */
/*                       Process                        */
/* **************************************************** */
char MarkProcessDone(ProcessList *pList, pid_t PID, int status);                      /* Mark process with matching PID as completed          */
void CheckCompletedProcesses(ProcessList *pList);                                     /* Check if any processes have completed                */
Process *AddProcess(ProcessList *pList, pid_t PID, char *cmd, char isBG, int fdIn);   /* Add a process to the list of background processes    */
void AddProcessAsChild(ProcessList *pList, pid_t pPID, pid_t cPID, char *cmd, char isBG, int fdIn);
/* **************************************************** */

#endif
