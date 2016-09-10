/* This is a csc60mshell.c
 * This program serves as a skeleton for starting for lab 5.
 * Student is required to use this program to build a mini shell 
 * using the specification as documented in lab 5 in SacCT.
 * Date: 4/28/2016
 * Author: Brian Junio
*/
#include <signal.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#define MAXLINE 80
#define MAXARGS 20
#define HISTORY_COUNT 20
#define JOB_COUNT 20

struct job_array
{
	int process_id;
	char command[80]; 
	int job_number; 
};

struct job_array jobs[20];

int bg = 0;
int numOfProcesses = 0;


void process_input(int argc, char **argv) {
  /* Step 1: perform system call execvp to execute command     */ 
  /*            No special operator(s) detected                   */
  /* Hint: Please be sure to review execvp.c sample program       */
  /* if (........ == -1) {                                        */  
  /*  perror("Shell Program");                                    */
  /*  _exit(-1);                                                  */
  /* }                                                            */
  /* Step 2: Handle redirection operators: < , or  >, or both  */
	int i;
	
	if(strcmp(argv[0],">")==0||strcmp(argv[0],"<")==0){
		printf("Error: no command \n");
	}
	
	else if(strcmp(argv[argc-1],">")==0||strcmp(argv[argc-1],"<")==0){
		printf("Error: no redirection file specified \n");
	}
	

	for(i = 0; i< argc; i++){
	
		if(strcmp(argv[i], ">") == 0){
			if(strcmp(argv[i+1], ">")==0 || strcmp(argv[i+1], "<")==0){
					printf("Error: Cannot redirect twice.\n");
				}
		}

		else if((strcmp(argv[i], "<")==0)){
			if(strcmp(argv[i+1], "<")==0 || strcmp(argv[i+1], ">")==0){
				printf("Error: Cannot redirect twice\n");
			}
		}
	}
	int pos =0;
	for(i = 0; i<argc; i++){
		if(strcmp(argv[i],">") == 0){
			pos = i;
			argv[i] = NULL;
			break;
		}
  	}
	int fileId;
	if(pos != 0)
		fileId = open(argv[pos + 1], O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);

 	if(fileId<0)
		printf("Error creating file\n");
	dup2(fileId, 1);
	close(fileId);  
	if(execvp(*argv, argv) == -1){
		perror("Process input error");
		_exit(-1);
	}

}
void cd(int argc, char **argv){
	char tempbuf[256];
	if(argc==2){
		if(chdir(argv[1])==0)
			printf("Directory changed to %s \n", argv[1]);
		else
			perror(argv[1]);
	}
	else if(argc==1){
		chdir(getenv("HOME"));
		printf("Directory changed to home\n");
	}
	else{
		getcwd(tempbuf, 256);
		setenv("PWD", tempbuf, 1);
	}
}
	

void pwd(){
	char *cwd;
	if((cwd = getcwd(NULL, 64))==NULL){
		perror("pwd");
		exit(2);
	}
	(void)printf("%s\n", cwd);
	free(cwd);
}


void childhandler(int sig) {
	pid_t pid;
        int status;
	int counter = 0;
	while((pid=waitpid(-1, &status, WNOHANG)) > 0)
	{
		for(counter = 0; counter < numOfProcesses; counter++){
			if(jobs[counter].process_id == pid){
				printf("\n[%d] Done \t %s\n", jobs[counter].job_number, jobs[counter].command);
				numOfProcesses--;
				if(numOfProcesses == 0)
					bg =0;
			}
		}
	}
} 
				
int parseline(char *cmdline, char **argv)
{
  int count = 0;
  char *separator = " \n\t";
  argv[count] = strtok(cmdline, separator);
  while ((argv[count] != NULL) && (count+1 < MAXARGS)) 
   argv[++count] = strtok((char *) 0, separator);
  
  return count;
}
/* ----------------------------------------------------------------- */
/*                  The main program starts here                     */
/* ----------------------------------------------------------------- */
int main(void)
{
 char cmdline[MAXLINE];
 char *argv[MAXARGS];
 int i, current = 0;
 int argc;
 int status;
 pid_t pid;
 int bg = 0;

 struct sigaction handler; 
 handler.sa_handler = SIG_IGN; 
 sigemptyset(&handler.sa_mask);
 handler.sa_flags = 0;
 sigaction(SIGINT, &handler, NULL);

 struct sigaction childaction;
 childaction.sa_handler = childhandler;
 sigemptyset(&childaction.sa_mask);
 childaction.sa_flags = 0;
 sigaction(SIGCHLD, &childaction, NULL);

 /*Initialize Jobs array*/

 for(i = 0; i< JOB_COUNT; i++)
	jobs[i].process_id = 0;

 /*Shell begins here*/

 for(i=0; i < 10; i++){
  printf("csc60mshell>  ");
  fgets(cmdline, MAXLINE, stdin);
  argc = parseline(cmdline, argv);
  current = (current + 1) % HISTORY_COUNT;
  
  char command[80];
  strncpy(command, cmdline, 80);
  if (strcmp(argv[argc-1],"&") == 0) {
    bg = 1;
    argv[argc-1] = NULL;
    argc -= 1;
    strncpy(jobs[numOfProcesses].command, command, 80);
    jobs[numOfProcesses].job_number = numOfProcesses; 
  }

  if(!(strcmp(argv[0], "jobs"))){
	int i;
	for(i=0; i < numOfProcesses; i++)
		printf("[%i] [%i]  Running\t %s\n", jobs[i].job_number,jobs[i].process_id, jobs[i].command);
	continue;
  }
  if(argc ==0){
    continue;
  }
  if (strcmp(argv[0], "cd")==0){
    cd(argc, argv);
  }
  if(argc ==1 && strcmp(argv[0], "pwd") == 0){
    pwd();
  }
  if(argc ==1 && strcmp(argv[0], "exit") == 0){
    if(numOfProcesses != 0){
        printf("Kill background processes before you exit\n");
        continue;
    }
    else
        exit(0);
  }
  
 
    pid = fork();
    if(pid == -1){
	perror("Shell Program fork error");
	exit(1);
    }
    else if(pid == 0){
	struct sigaction siginthandler;
        siginthandler.sa_handler = SIG_DFL;
        sigemptyset(&siginthandler.sa_mask);
        siginthandler.sa_flags = 0;
        sigaction(SIGINT, &siginthandler, NULL);
		if(bg)
			setpgid(0, 0);
        process_input(argc, argv);
    }
    else{
	if(bg){
		jobs[numOfProcesses].process_id = pid;
		numOfProcesses++;
	}
	else{  
		if(wait(&status) == -1){
			perror("Shell Program error");
		}
		else{
			printf("Child returned status: %d\n", status);
		}
        }
    }		
  }
 } 
 
