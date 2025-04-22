#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define NUMBER_OF_PROCESSES 4
long int getDifferenceInMilliSeconds(struct timeval start, struct timeval end);
long int getDifferenceInMicroSeconds(struct timeval start, struct timeval end);

int main() {
    int i, status;
    pid_t pid;

    struct timeval startTime, runTime;
    gettimeofday(&startTime, NULL);

    for(i = 0; i < NUMBER_OF_PROCESSES; i++) {
        pid = fork();

        if(pid < 0) {
            printf("Failed to create process: %d\n", i);
            exit(1);
        } else if(pid == 0) {
            gettimeofday(&runTime, NULL);
            sleep(1);
            printf("Hello from the child %d with pid: %d at time %d\n", i, getpid(),
                getDifferenceInMicroSeconds(startTime, runTime));
            return;
        }
    }
    
    waitpid(pid, &status, WUNTRACED);
    printf("Bye from the parent!\n");

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