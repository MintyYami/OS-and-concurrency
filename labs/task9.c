#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>

#define NUMBER_OF_PROCESSES 4
#define MAX_EXPERIMENT_DURATION 50
#define SIZE_OF_MEMORY sizeof(sizeof(long int) * NUMBER_OF_PROCESSES)
#define SHARED_MEMORY_NAME "shm_time"

long int getDifferenceInMilliSeconds(struct timeval start, struct timeval end);
long int getDifferenceInMicroSeconds(struct timeval start, struct timeval end);

//./task9 | head -n 10000 > output9.csv
int main() {
    int i, status;
    pid_t pid;
    struct timeval startTime, runTime;
    long int difference;

    //open shared memory
    int shm_fd = shm_open(SHARED_MEMORY_NAME, O_RDWR | O_CREAT, 0666);
    if(shm_fd == -1) {
        printf("failed to open shared memory\n");
        exit(1);
    }
    //configure size
    if(ftruncate(shm_fd, SIZE_OF_MEMORY) == -1) {
        printf("failed to set size of memory\n");
    }
    //map shared memory object to processor's logical address
    int * i_ptr = mmap(NULL, SIZE_OF_MEMORY, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    *i_ptr = 1000;
    if (munmap(i_ptr, SIZE_OF_MEMORY) == -1) {
        perror("Error un-mmapping the file");
    }

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

    //unlink shared memory
    shm_unlink( SHARED_MEMORY_NAME );
    shmctl(shm_fd, IPC_RMID, 0);

    return 0;
}

long int getDifferenceInMilliSeconds(struct timeval start, struct timeval end) {
    int seconds = end.tv_sec - start.tv_sec;
    int useconds = end.tv_usec - start.tv_usec;
    int mtime = (seconds * 1000 + useconds / 1000.0); return mtime;
}