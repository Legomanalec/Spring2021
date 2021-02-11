#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <unistd.h>

int main()
{
    pid_t pid, pid1;
    pid = fork();

    if (pid < 0) {
        fprintf(stderr, "Fork Failed");
        return 1;
    }

    else if (pid == 0) {
        pid1 = getpid();
        printf("child A: pid = %d\n", pid);
        printf("child B: pid1 = %d\n", pid1);
    }
    else {
        pid1 = getpid();
        printf("parent C: pid = %d\n", pid);
        printf("parent D: pid1 = %d\n", pid1);
        wait(NULL);
    }
    return 0;
}