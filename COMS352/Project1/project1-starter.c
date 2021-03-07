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

/* Signal handler for SIGTSTP (SIGnal - Terminal SToP),
 * which is caused by the user pressing control+z. */
void sigtstpHandler(int sig_num) {
	/* Reset handler to catch next SIGTSTP. */
	signal(SIGTSTP, sigtstpHandler);
	if (foregroundPid > 0) {
		/* Foward SIGTSTP to the currently running foreground process. */
		kill(foregroundPid, SIGTSTP);
		/* TODO: Add foreground command to the list of jobs. */
	}
}

int processNum = 1;
int runBackgroundCommand(Cmd *cmd){
	int returnPid = 0;
	cmd->pid = fork();
	if(cmd->pid == 0){	
		pid_t pid2 = fork();
		if(pid2 == 0){
			printf("[%d] %d", processNum, cmd->pid);
			execvp(cmd->args[0], cmd->args);
		}
		else{
			
			wait(NULL);
			printf("352>");
			printf("\n[%d] Done %s", processNum, cmd->line);
            kill(getpid(), SIGKILL);
		}
	}
	else{
		returnPid = cmd->pid;
		printf("[%d]", processNum);
		printf(" %d\n", cmd->pid);
		fflush(stdout);	
		int status;
		waitpid(cmd->pid, &status, WNOHANG);
	}
	processNum++;
	return returnPid;
}

char* jobsList[80];
int jobArrayIndex = 0;
int jobArraySize = 0;
int pidArray[80];
char* commandArray[80];
int main(void) {
	/* Listen for control+z (suspend process). */
	signal(SIGTSTP, sigtstpHandler);

	while (1) {
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
			for (int i = 0; i < jobArraySize; i++)
			{
				int status;
				int jobNum = i+1;
				pid_t return_pid = waitpid(pidArray[i], &status, WNOHANG); /* WNOHANG def'd in wait.h */
				if (return_pid == -1) {
					/* error */
				} else if (return_pid == 0) {
					printf("[%d]  Running    %s", jobNum, commandArray[i]);
				} else if (return_pid == pidArray[i]) {
					printf("[%d]  Stopped    %s", jobNum, commandArray[i]);
				}
			} 

		} else if (strcmp(cmd->args[0], "bg") == 0) {			

		} else {
			if (findSymbol(cmd, BG_OP) != -1) {
				/* TODO: Run command in background. */
				runBackgroundCommand(cmd);

			} else if(findSymbol(cmd, REDIRECT_OUT_OP) != -1) {
				cmd->pid = fork();
				if(cmd->pid == 0){
					int fd = creat(cmd->args[2] , 0644) ;
        			dup2(fd, STDOUT_FILENO);
       				close(fd);
					execvp(cmd->args[0], cmd->args);
				}	
				else{
					wait(NULL);
				}

			} else if(findSymbol(cmd, REDIRECT_IN_OP) != -1) {
				cmd->pid = fork();
				if(cmd->pid == 0){
					int fd = open(cmd->args[2], O_RDONLY);
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
				if(cmd->pid == 0)
				{
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
				if(cmd->pid == 0){
					execvp(cmd->args[0], cmd->args);
				}	
				else{
					wait(NULL);
				}
				/* TODO: Run command in foreground. */
			}
		}	
		/* TODO: Check on status of background processes. */

	}
	return 0;
}