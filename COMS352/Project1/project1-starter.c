/*******************************************
This program will emulate a UNIX shell. 
Pipes ('|'), file redirects ('<' & '>'),
background commands ('&') and jobs are 
implemented in this code. You can run all 
UNIX system commands. 
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>

#define MAX_LINE 80
#define MAX_ARGS (MAX_LINE/2 + 1)
#define REDIRECT_OUT_OP '>'
#define REDIRECT_IN_OP '<'
#define PIPE_OP '|'
#define BG_OP '&'

/* Holds a single command. */
typedef struct Cmd {
	/* The command as input by the user. */
	char line[MAX_LINE + 1];
	/* The command as null terminated tokens. */
	char tokenLine[MAX_LINE + 1];
	/* Pointers to each argument in tokenLine, non-arguments are NULL. */
	char* args[MAX_ARGS];
	/* Pointers to each symbol in tokenLine, non-symbols are NULL. */
	char* symbols[MAX_ARGS];
	/* The process id of the executing command. */
	pid_t pid;
	/* TODO: Additional fields may be helpful. */
	char* beforePipe[MAX_ARGS / 2];
	char* afterPipe[MAX_ARGS / 2];
} Cmd;

/* The process of the currently executing foreground command, or 0. */
pid_t foregroundPid = 0;

/* Parses the command string contained in cmd->line.
 * * Assumes all fields in cmd (except cmd->line) are initailized to zero.
 * * On return, all fields of cmd are appropriatly populated. */
void parseCmd(Cmd* cmd) {
	int pipeSwitch = 1;
	char* token;
	int i=0;
	int j = 0;
	strcpy(cmd->tokenLine, cmd->line);
	strtok(cmd->tokenLine, "\n");
	token = strtok(cmd->tokenLine, " ");
	while (token != NULL) {
		if (*token == '\n') {
			cmd->args[i] = NULL;
		} else if (*token == REDIRECT_OUT_OP || *token == REDIRECT_IN_OP
				|| *token == PIPE_OP || *token == BG_OP) {
			if(*token == PIPE_OP){
					pipeSwitch = 0;		
			}
			cmd->symbols[i] = token;
			cmd->args[i] = NULL;		
		} else {
			cmd->args[i] = token;
			if(pipeSwitch)
				cmd->beforePipe[i] = token;
			else
				cmd->afterPipe[j++] = token;
		}
		token = strtok(NULL, " ");
		i++;
	}
	cmd->args[i] = NULL;
}

/* Finds the index of the first occurance of symbol in cmd->symbols.
 * * Returns -1 if not found. */
int findSymbol(Cmd* cmd, char symbol) {
	for (int i = 0; i < MAX_ARGS; i++) {
		if (cmd->symbols[i] && *cmd->symbols[i] == symbol) {
			return i;
		}
	}
	return -1;
}
int pidArrayIndex = 0;
int pidArray[100];
/* Signal handler for SIGTSTP (SIGnal - Terminal SToP),
 * which is caused by the user pressing control+z. */
void sigtstpHandler(int sig_num) {

	/* Reset handler to catch next SIGTSTP. */
	signal(SIGTSTP, sigtstpHandler);
	
	if (foregroundPid > 0) {
		/* Foward SIGTSTP to the currently running foreground process. */
		
		kill(foregroundPid, SIGTSTP);
		pidArray[pidArrayIndex] = 0;
	}
}

int processNum = 1;
int runBackgroundCommand(Cmd *cmd){
	int returnPid = 0;
	int exitCode = 0;
	cmd->pid = fork();
	if(cmd->pid == 0){	
		pid_t pid2 = fork();
		if(pid2 == 0){
			/* sets exit code for the execution */
			/* execvp() does not require a PATH 
			it just needs the command and then an array
			of arguments */
			exitCode = execvp(cmd->args[0], cmd->args);
		}
		else{
			
			wait(NULL);//waits for child
			if(exitCode == 0)
				/* command exits as expected */
				printf("\n[%d] Done %s", processNum, cmd->line);
			else{
				/* command exits unexpectedly */
				printf("\n[%d] Exit %d %s", processNum, exitCode, cmd->line);
			}
            kill(getpid(), SIGKILL); //kills parent process once child is done
		}
	}
	else{
		/* this process goes allows the command loop to continue */
		returnPid = cmd->pid;
		printf("[%d] %d\n", processNum, cmd->pid);
		//fflush(stdout);	
		int status;
		waitpid(cmd->pid, &status, WNOHANG); //grabs child status without waiting for it
	}
	processNum++;
	return returnPid;
}

char* commandArray[80];
int commandArrayIndex = 0;
int main(void) {
	/* Listen for control+z (suspend process). */
	waitpid(foregroundPid, NULL, WUNTRACED);
	signal(SIGTSTP, sigtstpHandler);
	
	while (1) {
		int jobs = 0;
		printf("352> ");
		fflush(stdout);
		Cmd *cmd = (Cmd*) calloc(1, sizeof(Cmd));
		fgets(cmd->line, MAX_LINE, stdin);
		
		parseCmd(cmd);
		if (!cmd->args[0]) {
			free(cmd);
		} else if (strcmp(cmd->args[0], "exit") == 0) {
			free(cmd);
			exit(0);

		} else if (strcmp(cmd->args[0], "jobs") == 0) {
			jobs = 1; //used to make sure 'jobs' isnt in jobs list
			for (int i = 0; i < commandArrayIndex; i++)
			{
				int status;
				int jobNum = i+1;
				/* grabs the child pid status, if it is -1 then an error occured
				if it is 0 then the process is running, if it returns 
				the pid than the process has stopped. */
				pid_t return_pid = waitpid(pidArray[i], &status, WNOHANG); 
				if (return_pid == -1) {
					/* error */
				} else if (return_pid == 0) {
					printf("[%d]  Running    %s", jobNum, commandArray[i]);
				} else if (return_pid == pidArray[i]) {
					printf("[%d]  Stopped    %s", jobNum, commandArray[i]);
				}
			} 

		} else if (strcmp(cmd->args[0], "bg") == 0) {	
			int id = atoi(cmd->args[1]);
			int status;
			pid_t return_pid = waitpid(pidArray[id], &status, WNOHANG);
			if (return_pid == 0)
				printf("[%d] Running    %s", id, commandArray[id-1]);
			else
				printf("[%d] Stopped    %s", id, commandArray[id-1]);

		} else {
			if (findSymbol(cmd, BG_OP) != -1) {
				runBackgroundCommand(cmd);

			} else if(findSymbol(cmd, REDIRECT_OUT_OP) != -1) {
				cmd->pid = fork();
				foregroundPid = cmd->pid;
				if(cmd->pid == 0){
					int fd = creat(cmd->args[2], 0644); //creates file if needed
        			dup2(fd, STDOUT_FILENO);
       				close(fd);
					execvp(cmd->args[0], cmd->args);
				}	
				else{
					wait(NULL);
				}

			} else if(findSymbol(cmd, REDIRECT_IN_OP) != -1) {
				cmd->pid = fork();
				foregroundPid = cmd->pid;

				if(cmd->pid == 0){
					int fd = open(cmd->args[2], O_RDONLY); //read only
        			dup2(fd, STDIN_FILENO);
        			close(fd);
					execvp(cmd->args[0], cmd->args);
				}	
				else{
					wait(NULL);
				}
			} else if(findSymbol(cmd, PIPE_OP) != -1) {		
				int fd[2];
				pipe(fd);
				cmd->pid = fork();
				foregroundPid = cmd->pid;
				if(cmd->pid == 0)
				{
					//descriptor logic
					dup2(fd[0], 0);
					close(fd[0]);
					close(fd[1]);
					execvp(cmd->afterPipe[0], cmd->afterPipe);

				}else{
					cmd->pid = fork();
					if(cmd->pid == 0){
						dup2(fd[1], 1);
						close(fd[1]);
						close(fd[0]);
						execvp(cmd->beforePipe[0], cmd->beforePipe);
					}else{
						wait(NULL);
					}
				}
				close(fd[1]);
				close(fd[0]);
				wait(NULL);		
			} else {

				cmd->pid = fork();
				foregroundPid = cmd->pid;
				if(cmd->pid == 0){
					execvp(cmd->args[0], cmd->args);
				}	
				else{
					wait(NULL);
				}
				/* TODO: Run command in foreground. */
			}
		}	
		/* makes sure not to add the 'jobs' command to the 
		jobs list */
		if(jobs == 0){
			foregroundPid = cmd->pid;
			pidArray[pidArrayIndex++] = foregroundPid; //PIDs for the jobs array
			commandArray[commandArrayIndex++] = cmd->line;
		}
	}
	return 0;
}