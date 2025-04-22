#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define NUMBER_OF_PROCESSES 4

int main() {
    int i, status;
    pid_t pid;

    printf("Hello from the parent process\n");

    for(i = 0; i < NUMBER_OF_PROCESSES; i++) {
        pid = fork();

        if(pid < 0) {
            printf("Failed to create process: %d\n", i);
            exit(1);
        } else if(pid == 0) {
            sleep(1);
            printf("Hello from the child process with pid %d\n", getpid());
            return;
        }
    }
    
    waitpid(pid, &status, WUNTRACED);
    printf("Bye from the parent!\n");

    return 0;
}