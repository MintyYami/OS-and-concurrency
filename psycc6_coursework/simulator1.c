#include <stdio.h>
#include "coursework.h"

int main() {
    //Create a process
    Process *p = generateProcess(0);
    printf("GENERATOR - CREATED: [PID = %d, Priority = %d, InitialBurstTime = %d, RemainingBurstTime = %d]\n",
        p->iPID, p->iPriority, p->iBurstTime, p->iRemainingBurstTime);

    //Simulate the process running in a round-robin fashion
    while (p->iRemainingBurstTime > 0) {
        runPreemptiveProcess(p, false);
        printf("SIMULATOR - CPU 0: RR [PID = %d, Priority = %d, InitialBurstTime = %d, RemainingBurstTime = %d]\n",
            p->iPID, p->iPriority, p->iBurstTime, p->iRemainingBurstTime);
    }

    //Process terminated
    printf("TERMINATOR - TERMINATED: [PID = %d, ResponseTime = 0, TurnAroundTime = %d]\n",
        p->iPID, p->iBurstTime);

    //Clean up memory
    destroyProcess(p);

    return 0;
}
