#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>

int main()
{
    int i;
    pid_t pid, pid1;
    for (i = 0; i <2; i++){
        pid = fork();
        pid = fork();
    }
    sleep(10000);
    //run pstree in another terminal after executing
    return 0;
}