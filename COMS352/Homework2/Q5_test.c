#include<stdio.h> 
#include<stdlib.h> 
#include<unistd.h> 
#include<sys/types.h> 
#include<string.h> 
#include<sys/wait.h> 
int main() { 
    char buffer[100];
    int fd1[2];
    if (pipe(fd1) == -1) 
    {   
        return 1; 
    } 
    int fd2[2];
    if (pipe(fd2) == -1) 
    { 
        return 1; 
    } 
    pid_t pid = fork(); 
    if (pid < 0) 
    { 
        return 1; 
    } 
    if (pid == 0) 
    { 
        close(fd1[0]);
        close(fd2[1]);
        dup2(fd1[1], 1);
        dup2(fd2[0], 0);
        execlp("cat", "cat", NULL);
    } 

    else 
    {
        close(fd1[1]);
        close(fd2[0]);
        write(fd2[1], "Hello, Child!\n", 14);
        read(fd1[0], buffer, 100); 
        close(fd1[0]);
        close(fd2[1]);
        printf("%s", buffer);
    }
    sleep(10000);
    return 0;
}