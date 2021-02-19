#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
int id[4];
/* the thread */
void *runner(void *param) 
{
    int *id = (int *)param;
    printf("THREAD %d FINISHED\n", *id);
    pthread_exit(0);
}
int main(int argc, char *argv[]) 
{
    pid_t pid;
    pthread_t tid[4];
    pthread_attr_t attr;
    pid = fork();
    pthread_kill(tid[1], 3);

    if (pid == 0) 
    {
        /* child process */
        pthread_attr_init(&attr);
        for (int i=0; i<4; i++) 
        {
            id[i] = i;
            pthread_create(&tid[i], &attr, runner, &id[i]);
        }
        for (int i = 0; i < 4; i++)
            pthread_join(tid[i], NULL);

        printf("CHILD PROCESS FINISHED\n");
    } 
    else if (pid > 0) 
    {
        /* parent process*/
        wait(NULL);
        printf("PARENT PROCESS FINISHED\n");
    }
    return 0;
}