#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define NUMBER_OF_PROCESSES 4
#define MAX_EXPERIMENT_DURATION 50

long int getDifferenceInMilliSeconds(struct timeval start, struct timeval end);
long int getDifferenceInMicroSeconds(struct timeval start, struct timeval end);

//./task5 | head -n 10000 > output5.csv
int main() {
    int i, status;
    pid_t pid;
    struct timeval startTime, runTime;
    long int difference;


    gettimeofday(&startTime, NULL);

    for(i = 0; i < NUMBER_OF_PROCESSES; i++) {
        pid = fork();

        if(pid < 0) {
            printf("Failed to create process: %d\n", i);
            exit(1);
        } else if(pid == 0) {
            difference = 0;
            while (difference < MAX_EXPERIMENT_DURATION) {
                gettimeofday(&runTime, NULL);
                difference = getDifferenceInMilliSeconds(startTime, runTime);
                printf("%ld, %d\n", difference, i);
            }
            return 0;
        }
    }
    
    if(pid>0) {
        for(i = 0; i < NUMBER_OF_PROCESSES; i++) {
            waitpid(-1, &status, WUNTRACED);
        }
    }

    return 0;
}

long int getDifferenceInMilliSeconds(struct timeval start, struct timeval end) {
    int seconds = end.tv_sec - start.tv_sec;
    int useconds = end.tv_usec - start.tv_usec;
    int mtime = (seconds * 1000 + useconds / 1000.0); return mtime;
}
long int getDifferenceInMicroSeconds(struct timeval start, struct timeval end) {
    int seconds = end.tv_sec - start.tv_sec; int useconds = end.tv_usec - start.tv_usec; int mtime = (seconds * 1000000 + useconds); return mtime;
}