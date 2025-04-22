#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <semaphore.h>
#include <fcntl.h>

#define NUMBER_OF_PROCESSES 4
#define MAX_EXPERIMENT_DURATION 20

long int difference;
sem_t sem;

long int getDifferenceInMilliSeconds(struct timeval start, struct timeval end);

//./task8 | head -n 10000 > output8.csv
int main() {
    int i, status;
    pid_t pid;
    struct timeval startTime, runTime;

    sem = sem_open("/sem", O_CREAT | O_EXCL, 0644, 1);
    if (sem == SEM_FAILED) {
        perror("sem_open() failed ");
    }
    // sem_init(sem,1,1);

    gettimeofday(&startTime, NULL);

    for (i = 0; i < NUMBER_OF_PROCESSES; i++) {
        pid = fork();

        if (pid < 0) {
            printf("Failed to create process: %d\n", i);
            exit(EXIT_FAILURE);
        }
        else if (pid == 0) {
            difference = 0;
            while (difference < MAX_EXPERIMENT_DURATION) {
                sem_wait(&sem);
                gettimeofday(&runTime, NULL);
                difference = getDifferenceInMilliSeconds(startTime, runTime);
                printf("%ld, %d\n", difference, i);
                sem_post(&sem);
            }
            return 0;
        }
    }
    
    if(pid>0) {
        for(i = 0; i < NUMBER_OF_PROCESSES; i++) {
            waitpid(-1, &status, WUNTRACED);
        }
    }
    
    sem_unlink("/sem");
    sem_close(&sem);

    return 0;
}

long int getDifferenceInMilliSeconds(struct timeval start, struct timeval end) {
    int seconds = end.tv_sec - start.tv_sec;
    int useconds = end.tv_usec - start.tv_usec;
    int mtime = (seconds * 1000 + useconds / 1000.0); return mtime;
}