#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <unistd.h>

int main()
{
    pid_t pid;
    
    pid = fork();
    printf("LINE J");

    if (pid < 0) {
        fprintf(stderr, "Fork Failed");
        return 1;
    }
    else if (pid == 0) {
        printf("LINE J");
        sleep(3);
        
        execlp("/bin/ls", "ls", NULL);
        printf("LINE J");
    }
    else {
        wait(NULL);
        printf("Child Complete");
    }
    return 0;
}