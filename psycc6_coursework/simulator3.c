#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include "linkedlist.h"
#include "coursework.h"

//ready queue
LinkedList oReadyQueue = LINKED_LIST_INITIALIZER;
int RQsize = 0;
//terminate queue
LinkedList oTerminateQueue = LINKED_LIST_INITIALIZER;
int TQsize = 0;

//threads
pthread_t generatorThread, simulatorThread, terminatorThread;

//semephores
sem_t genSem; //generation
sem_t simSem; //simulation
sem_t terSem; //terminaion

//mutex
pthread_mutex_t lock;

//variables
double totalResponseTime = 0;
double totalTurnaroundTime = 0;
bool genDone = false;

//components
void * generator();
void * simulator();
void * terminator();

int main() {
    //initialise semephores
    sem_init(&genSem, 0, MAX_CONCURRENT_PROCESSES);
    sem_init(&simSem, 0, 0);
    sem_init(&terSem, 0, 0);
    //initialise metux
    pthread_mutex_init(&lock,NULL);

    //create threads
    if(pthread_create((&generatorThread), NULL, generator, NULL)) {
        printf("Error: Cannot create generator thread\n");
    }
    if(pthread_create((&simulatorThread), NULL, simulator, NULL)) {
        printf("Error: Cannot create simulator thread\n");
    }
    if(pthread_create((&terminatorThread), NULL, terminator, NULL)) {
        printf("Error: Cannot create terminator thread\n");
    }

    //wait for threads to finish
    pthread_join(generatorThread, NULL);
    pthread_join(simulatorThread, NULL);
    pthread_join(terminatorThread, NULL);

    //free
    //semaphores
    sem_destroy(&genSem);
    sem_destroy(&simSem);
    sem_destroy(&terSem);
    //mutex
    pthread_mutex_destroy(&lock);

    return 0;
}

void * generator() {
    for(int i = 0; i < NUMBER_OF_PROCESSES; i++) {
        sem_wait(&genSem);
        pthread_mutex_lock(&lock);

        //create process
        Process * p = generateProcess(i);
        printf("GENERATOR - CREATED: [PID = %d, Priority = %d, InitialBurstTime = %d, RemainingBurstTime = %d]\n",
                p->iPID, p->iPriority, p->iBurstTime, p->iRemainingBurstTime);
        //add to ready queue
        addLast((void *)p, &oReadyQueue);
        printf("QUEUE - ADDED: [Queue = READY, Size = %d, PID = %d, Priority = %d]\n",
                ++RQsize, p->iPID, p->iPriority);
        //admit
        printf("GENERATOR - ADMITTED: [PID = %d, Priority = %d, InitialBurstTime = %d, RemainingBurstTime = %d]\n",
                p->iPID, p->iPriority, p->iBurstTime, p->iRemainingBurstTime);

        pthread_mutex_unlock(&lock);
        if((i >= (MAX_CONCURRENT_PROCESSES-2)) && (i < (NUMBER_OF_PROCESSES-1))) {
            sem_post(&simSem);
        }
    }
    pthread_mutex_lock(&lock);
    genDone = true;
    printf("GENERATOR: Finished\n");
    pthread_mutex_unlock(&lock);
    sem_post(&simSem);
}

void * simulator() {
    sem_wait(&simSem);
    while(RQsize > 0) {
        sem_wait(&simSem);
        //remove from ready
        pthread_mutex_lock(&lock);
        //remove from ready queue
        Process * p = (Process *) removeFirst(&oReadyQueue);
        printf("QUEUE - REMOVED: [Queue = READY, Size = %d, PID = %d, Priority = %d]\n",
                --RQsize, p->iPID, p->iPriority);
        pthread_mutex_unlock(&lock);

        //if terminator required
        if(TQsize > 0) {
            sem_post(&terSem);
            sem_wait(&simSem);
        }
                
        pthread_mutex_lock(&lock);
        //simulate
        runPreemptiveProcess(p, false);
        printf("SIMULATOR - CPU 0: RR [PID = %d, Priority = %d, InitialBurstTime = %d, RemainingBurstTime = %d]\n",
                p->iPID, p->iPriority, p->iBurstTime, p->iRemainingBurstTime);

        //check state
         if(p->iState == READY) {
            //add back to ready queue
            addLast((void *)p, &oReadyQueue);
            printf("QUEUE - ADDED: [Queue = READY, Size = %d, PID = %d, Priority = %d]\n",
                    ++RQsize, p->iPID, p->iPriority);
            printf("SIMULATOR - CPU 0 - READY: [PID = %d, Priority = %d]\n", p->iPID, p->iPriority);
        } else {
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
        pthread_mutex_unlock(&lock);
        sem_post(&simSem);
    }

    sem_post(&terSem);
    sem_wait(&simSem);
    sem_wait(&simSem);
    
    printf("SIMULATOR: Finished\n");
}

void * terminator() {
    for(int i = 0; i < NUMBER_OF_PROCESSES; i++) {
        sem_wait(&terSem);
        pthread_mutex_lock(&lock);

        //remove from terminate queue
        Process * p = (Process *) removeFirst(&oTerminateQueue);
        printf("QUEUE - REMOVED: [Queue = TERMINATED, Size = %d, PID = %d, Priority = %d]\n",
                --TQsize, p->iPID, p->iPriority);
        //clean up memory
        printf("TERMINATION DAEMON - CLEARED: [#iTerminated = %d, PID = %d, Priority = %d]\n",
                (i+1), p->iPID, p->iPriority);
        destroyProcess(p);

        pthread_mutex_unlock(&lock);
        if(!genDone) {
            sem_post(&genSem);
        } else if(i < (NUMBER_OF_PROCESSES-1)) {
            sem_post(&simSem);
        }
    }
    pthread_mutex_lock(&lock);
    printf("TERMINATION DAEMON: Finished\n");
    
    double averageResponseTime = (double) (totalResponseTime / NUMBER_OF_PROCESSES);
    double averageTurnaroundTime = (double) (totalTurnaroundTime / NUMBER_OF_PROCESSES);
    printf("TERMINATION DAEMON: [Average Response Time = %f, Average Turn Around Time = %f]\n",
            averageResponseTime, averageTurnaroundTime);
    pthread_mutex_unlock(&lock);

    sem_post(&simSem);
}