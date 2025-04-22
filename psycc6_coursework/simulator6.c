#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include "linkedlist.h"
#include "coursework.h"

//array for ready queues
LinkedList readyQueueSet[NUMBER_OF_PRIORITY_LEVELS];
//array for ready queue sizes
int readyQueueSize[NUMBER_OF_PRIORITY_LEVELS];

//ready queue
LinkedList oReadyQueue = LINKED_LIST_INITIALIZER;
int RQsize = 0;

//terminate queue
LinkedList oTerminateQueue = LINKED_LIST_INITIALIZER;
int TQsize = 0;

//process table
Process * processTable[SIZE_OF_PROCESS_TABLE];
//pool of PID
LinkedList pool = LINKED_LIST_INITIALIZER;

//threads
pthread_t generatorThread, simulatorThread, boosterThread, terminatorThread;

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
bool simDone = false;

//components
void * generator();
void * simulator();
void * boosterDaemon();
void * terminator();

int main() {
    //initialise PID pool
    for (int i = 0; i < SIZE_OF_PROCESS_TABLE; i++) {
        int *pid = (int *)malloc(sizeof(int));
        *pid = i;
        addLast(pid, &pool);
    }

    //initialise ready queues and sizes
    for(int i = 0; i < NUMBER_OF_PRIORITY_LEVELS; i++) {
        LinkedList newQueue = LINKED_LIST_INITIALIZER;
        readyQueueSet[i] = newQueue;
        readyQueueSize[i] = 0;
    }

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
    if(pthread_create((&boosterThread), NULL, boosterDaemon, NULL)) {
        printf("Error: Cannot create booaster daemon thread\n");
    }
    if(pthread_create((&terminatorThread), NULL, terminator, NULL)) {
        printf("Error: Cannot create terminator thread\n");
    }

    //wait for threads to finish
    pthread_join(generatorThread, NULL);
    pthread_join(simulatorThread, NULL);
    pthread_join(boosterThread, NULL);
    pthread_join(terminatorThread, NULL);

    //free
    //semaphores
    sem_destroy(&genSem);
    sem_destroy(&simSem);
    sem_destroy(&terSem);
    //mutex
    pthread_mutex_destroy(&lock);
    //pool
    int *pid;
    while ((pid = (int *)removeFirst(&pool)) != NULL) {
        free(pid);
    }

    return 0;
}

void * generator() {
    for(int i = 0; i < NUMBER_OF_PROCESSES; i++) {
        sem_wait(&genSem);

        //get available pid from pool
        pthread_mutex_lock(&lock);
        int * pid = (int *)removeFirst(&pool);
        pthread_mutex_unlock(&lock);
        //create process
        pthread_mutex_lock(&lock);
        Process * p = generateProcess(*pid);
        printf("GENERATOR - CREATED: [PID = %d, Priority = %d, InitialBurstTime = %d, RemainingBurstTime = %d]\n",
                p->iPID, p->iPriority, p->iBurstTime, p->iRemainingBurstTime);
        pthread_mutex_unlock(&lock);
        //add to process table
        pthread_mutex_lock(&lock);
        processTable[*pid] = p;
        free(pid);
        printf("GENERATOR - ADDED TO TABLE: [PID = %d, Priority = %d, Initial BurstTime = %d, Remaining BurstTime = %d]\n",
                p->iPID, p->iPriority, p->iBurstTime, p->iRemainingBurstTime);
        pthread_mutex_unlock(&lock);
        //add to ready queue
        pthread_mutex_lock(&lock);
        addLast((void *) p, &readyQueueSet[p->iPriority]);
        printf("QUEUE - ADDED: [Queue = READY %d, Size = %d, PID = %d, Priority = %d]\n",
                p->iPriority, ++readyQueueSize[p->iPriority], p->iPID, p->iPriority);
        pthread_mutex_unlock(&lock);
        //admit
        pthread_mutex_lock(&lock);
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
    //check more process + get highest priority
    int priority = 0;
    while((priority < NUMBER_OF_PRIORITY_LEVELS) && (readyQueueSize[priority] <= 0)) {
        priority++;
    }
    while(priority < NUMBER_OF_PRIORITY_LEVELS) {
        sem_wait(&simSem);
        //remove from ready queue
        pthread_mutex_lock(&lock);
        Process * p = (Process *) removeFirst(&readyQueueSet[priority]);
        printf("QUEUE - REMOVED: [Queue = READY %d, Size = %d, PID = %d, Priority = %d]\n",
                priority, --readyQueueSize[priority], p->iPID, p->iPriority);
        pthread_mutex_unlock(&lock);

        //if terminator required
        if(TQsize > 0) {
            sem_post(&terSem);
            sem_wait(&simSem);
        }
        
        //simulate
        pthread_mutex_lock(&lock);
        if(priority < (NUMBER_OF_PRIORITY_LEVELS/2)) {
            runNonPreemptiveProcess(p, false);
            printf("SIMULATOR - CPU 0: FCFS [PID = %d, Priority = %d, InitialBurstTime = %d, RemainingBurstTime = %d]\n",
                    p->iPID, p->iPriority, p->iBurstTime, p->iRemainingBurstTime);
        } else {
            runPreemptiveProcess(p, false);
            printf("SIMULATOR - CPU 0: RR [PID = %d, Priority = %d, InitialBurstTime = %d, RemainingBurstTime = %d]\n",
                    p->iPID, p->iPriority, p->iBurstTime, p->iRemainingBurstTime);
        }
        pthread_mutex_unlock(&lock);

        //check state
        if(p->iState == READY) {
            //add back to ready queue
            pthread_mutex_lock(&lock);
            addLast((void *)p, &readyQueueSet[p->iPriority]);
            printf("QUEUE - ADDED: [Queue = READY %d, Size = %d, PID = %d, Priority = %d]\n",
                    priority, ++readyQueueSize[p->iPriority], p->iPID, p->iPriority);
            printf("SIMULATOR - CPU 0 - READY: [PID = %d, Priority = %d]\n", p->iPID, p->iPriority);
            pthread_mutex_unlock(&lock);
        } else {
            pthread_mutex_lock(&lock);
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
            pthread_mutex_unlock(&lock);
        }

        //reset priority
        pthread_mutex_lock(&lock);
        priority = 0;
        while((priority < NUMBER_OF_PRIORITY_LEVELS) && (readyQueueSize[priority] <= 0)) {
            priority++;
        }
        pthread_mutex_unlock(&lock);
        sem_post(&simSem);
    }

    sem_post(&terSem);
    sem_wait(&simSem);
    sem_wait(&simSem);
    
    pthread_mutex_lock(&lock);
    simDone = true;
    printf("SIMULATOR: Finished\n");
    pthread_mutex_unlock(&lock);
}

void * boosterDaemon() {
    int priority;

    pthread_mutex_lock(&lock);
    printf("BOOSTER DAEMON: Created\n");
    pthread_mutex_unlock(&lock);

    while(!simDone) {
        //wait in miliseconds
        usleep(BOOST_INTERVAL * 1000);
        //get second half of priority queues
        priority = (NUMBER_OF_PRIORITY_LEVELS / 2) + 1;
        //move all priority to highest in second half
        for(int priority = ((NUMBER_OF_PRIORITY_LEVELS/2)+1); priority < NUMBER_OF_PRIORITY_LEVELS; priority++) {
            pthread_mutex_lock(&lock);
            while(readyQueueSize[priority] > 0) {
                //remove from ready queue
                Process * p = (Process *) removeFirst(&readyQueueSet[priority]);
                printf("QUEUE - REMOVED: [Queue = READY %d, Size = %d, PID = %d, Priority = %d]\n",
                        priority, --readyQueueSize[priority], p->iPID, p->iPriority);
                //boost priority
                int topPriority = (NUMBER_OF_PRIORITY_LEVELS / 2);
                printf("BOOSTER DAEMON: [PID = %d, Priority = %d, InitialBurstTime = %d, RemainingBurstTime = %d] => Boosted to Level %d\n",
                        p->iPID, p->iPriority, p->iBurstTime, p->iRemainingBurstTime, topPriority);
                //add to highest priority in second half
                addLast((void *)p, &readyQueueSet[topPriority]);
                printf("QUEUE - ADDED: [Queue = READY %d, Size = %d, PID = %d, Priority = %d]\n",
                        topPriority, ++readyQueueSize[topPriority], p->iPID, p->iPriority);
            }
            pthread_mutex_unlock(&lock);
        }
    }

    pthread_mutex_lock(&lock);
    printf("BOOSTER DAEMON: Finished\n");
    pthread_mutex_unlock(&lock);
}

void * terminator() {
    for(int i = 0; i < NUMBER_OF_PROCESSES; i++) {
        sem_wait(&terSem);

        //remove from terminate queue
        pthread_mutex_lock(&lock);
        Process * p = (Process *) removeFirst(&oTerminateQueue);
        printf("QUEUE - REMOVED: [Queue = TERMINATED, Size = %d, PID = %d, Priority = %d]\n",
                --TQsize, p->iPID, p->iPriority);
        pthread_mutex_unlock(&lock);
        //remove from process table
        pthread_mutex_lock(&lock);
        processTable[p->iPID] = NULL;
        pthread_mutex_unlock(&lock);
        //add pid back to pool
        pthread_mutex_lock(&lock);
        int * pid = (int *)malloc(sizeof(int));
        *pid = p->iPID;
        addLast((void *) pid, &pool);
        pthread_mutex_unlock(&lock);
        //clean up memory
        pthread_mutex_lock(&lock);
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
    pthread_mutex_unlock(&lock);

    double averageResponseTime = (double) (totalResponseTime / NUMBER_OF_PROCESSES);
    double averageTurnaroundTime = (double) (totalTurnaroundTime / NUMBER_OF_PROCESSES);
    pthread_mutex_lock(&lock);
    printf("TERMINATION DAEMON: [Average Response Time = %f, Average Turn Around Time = %f]\n",
            averageResponseTime, averageTurnaroundTime);
    pthread_mutex_unlock(&lock);

    sem_post(&simSem);
}