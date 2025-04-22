#include <stdio.h>
#include "linkedlist.h"
#include "coursework.h"

int main() {
    Process *p;
    double totalResponseTime = 0;
    double totalTurnaroundTime = 0;

    //ready queue
    LinkedList oReadyQueue = LINKED_LIST_INITIALIZER;
    int PQsize = 0;
    //terminate queue
    LinkedList oTerminateQueue = LINKED_LIST_INITIALIZER;
    int TQsize = 0;

    for(int i = 0; i < NUMBER_OF_PROCESSES; i++) {
        //create process
        p = generateProcess(i);
        printf("GENERATOR - CREATED: [PID = %d, Priority = %d, InitialBurstTime = %d, RemainingBurstTime = %d]\n",
                p->iPID, p->iPriority, p->iBurstTime, p->iRemainingBurstTime);
        //add to ready queue
        addLast((void *)p, &oReadyQueue);
        printf("QUEUE - ADDED: [Queue = READY, Size = %d, PID = %d, Priority = %d]\n",
                ++PQsize, p->iPID, p->iPriority);
        //admit
        printf("GENERATOR - ADMITTED: [PID = %d, Priority = %d, InitialBurstTime = %d, RemainingBurstTime = %d]\n",
                p->iPID, p->iPriority, p->iBurstTime, p->iRemainingBurstTime);
    }

    printf("GENERATOR: Finished\n");

    //simulate processing
    while(PQsize>0) {
        //remove from ready queue
        p = (Process *) removeFirst(&oReadyQueue);
        printf("QUEUE - REMOVED: [Queue = READY, Size = %d, PID = %d, Priority = %d]\n",
                --PQsize, p->iPID, p->iPriority);
        //simulate
        runPreemptiveProcess(p, false);
        printf("SIMULATOR - CPU 0: RR [PID = %d, Priority = %d, InitialBurstTime = %d, RemainingBurstTime = %d]\n",
                p->iPID, p->iPriority, p->iBurstTime, p->iRemainingBurstTime);
        
        //check state
        if(p->iState == READY) {
            //add back to ready queue
            addLast((void *)p, &oReadyQueue);
            printf("QUEUE - ADDED: [Queue = READY, Size = %d, PID = %d, Priority = %d]\n",
                    ++PQsize, p->iPID, p->iPriority);
            printf("SIMULATOR - CPU 0 - READY: [PID = %d, Priority = %d]\n", p->iPID, p->iPriority);
        } else if(p->iState == TERMINATED) {
            //calculate response time and turn around time
            long int responseTime = getDifferenceInMilliSeconds(p->oTimeCreated, p->oFirstTimeRunning);
            totalResponseTime += responseTime;
            long int turnAroundTime = getDifferenceInMilliSeconds(p->oTimeCreated, p->oLastTimeRunning);
            totalTurnaroundTime += turnAroundTime;
            printf("SIMULATOR - CPU 0 - TERMINATED: [PID = %d, ResponseTime = %d, TurnAroundTime = %d]\n",
                    p->iPID, responseTime, turnAroundTime);
            //add to terminate queue
            addLast((void *)p, &oTerminateQueue);
            printf("QUEUE - ADDED: [Queue = TERMINATED, Size = %d, PID = %d, Priority = %d]\n",
                    ++TQsize, p->iPID, p->iPriority);
        }
    }

    printf("SIMULATOR: Finished\n");

    int iteraion = 0;

    while(TQsize > 0) {
        //remove from terminate queue
        p = (Process *) removeFirst(&oTerminateQueue);
        printf("QUEUE - REMOVED: [Queue = TERMINATED, Size = %d, PID = %d, Priority = %d]\n",
                --TQsize, p->iPID, p->iPriority);
        //clean up memory
        printf("TERMINATION DAEMON - CLEARED: [#iTerminated = %d, PID = %d, Priority = %d]\n",
                ++iteraion, p->iPID, p->iPriority);
        destroyProcess(p);
    }

    printf("TERMINATION DAEMON: Finished\n");
    
    double averageResponseTime = (double) (totalResponseTime / NUMBER_OF_PROCESSES);
    double averageTurnaroundTime = (double) (totalTurnaroundTime / NUMBER_OF_PROCESSES);
    printf("TERMINATION DAEMON: [Average Response Time = %f, Average Turn Around Time = %f]\n",
            averageResponseTime, averageTurnaroundTime);

    return 0;
}