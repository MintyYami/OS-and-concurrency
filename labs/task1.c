#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#define NUMBER_OF_PROCESSES 5

int main() {
    pid_t pid = 0;
    int i = 0;

    while (i < NUMBER_OF_PROCESSES) {
        pid = fork();

        if (pid < 0) {
            printf("Failed to create process: %d\n", i);
            exit(1);
        } else if (pid == 0) {
            sleep(1);
            printf("Hello from the child %d with pid: %d\n", i, getpid());
            exit(0);
        }
        i++;
    }
    
  return 0;
}