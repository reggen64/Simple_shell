#include  <stdio.h>
#include  <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <signal.h>

#define MAX_INPUT_SIZE 1024
#define MAX_TOKEN_SIZE 64
#define MAX_NUM_TOKENS 64

int bg_pid[]={[0 ... 99] INT_MAX};
int bg_ctr=0;
int fg_pid=INT_MIN;
char  **tokens;              
/* Splits the string by space and returns the array of tokens
*
*/
char **tokenize(char *line)
{
  char **tokens = (char **)malloc(MAX_NUM_TOKENS * sizeof(char *));
  char *token = (char *)malloc(MAX_TOKEN_SIZE * sizeof(char));
  int i, tokenIndex = 0, tokenNo = 0;

  for(i =0; i < strlen(line); i++){

    char readChar = line[i];

    if (readChar == ' ' || readChar == '\n' || readChar == '\t'){
      token[tokenIndex] = '\0';
      if (tokenIndex != 0){
	tokens[tokenNo] = (char*)malloc(MAX_TOKEN_SIZE*sizeof(char));
	strcpy(tokens[tokenNo++], token);
	tokenIndex = 0; 
      }
    } else {
      token[tokenIndex++] = readChar;
    }
  }
 
  free(token);
  tokens[tokenNo] = NULL ;
  return tokens;
}
/*
void bg_finish(){puts("Shell: Background Process Finished");}
void reaper(){
	struct sigaction sa;
	memset(&sa, 0, sizeof(sigaction));
	sa.sa_handler = &bg_finish;
	sa.sa_flags = SA_NOCLDWAIT;
	
	sigaction(SIGCHLD, &sa, NULL);
}
*/
void free_tokens(){
	for(int i=0; tokens[i]!=NULL; i++) free(tokens[i]);
	free(tokens);
}

void sigint_handl(int sig){
	if(kill(fg_pid, 0)==0){
	/*
	for(int i=0; i<100; i++){
		if(bg_pid[i]==INT_MAX) continue;
		else{
			if(kill(bg_pid[i], SIGINT)!=0){perror("BG process SIGINT kill error");}
		}
	}*/
	kill(fg_pid, SIGINT);
	} else {
		printf("\nNo FG process Running\n");
	}
}

int main(int argc, char* argv[]) {
	char  line[MAX_INPUT_SIZE];            

	while(1) {			
		signal(SIGINT, &sigint_handl);
		/* BEGIN: TAKING INPUT */
		bzero(line, sizeof(line));
		printf("$ ");
		scanf("%[^\n]", line);
		getchar();

		//printf("Command entered: %s (remove this debug output later)\n", line);
		/* END: TAKING INPUT */

		//checking and reaping deadchildren
		for(int i=0; i<100; i++){
			if(bg_pid[i]==INT_MAX) continue;

			if(waitpid(bg_pid[i],NULL,WNOHANG)==bg_pid[i]){
				printf("Reaped BG process pid: %d\n", bg_pid[i]);
				bg_pid[i]=INT_MAX;
				bg_ctr--;
			}
		}	
		//return check
		if(strlen(line)==0) continue;

		line[strlen(line)] = '\n'; //terminate with new line
		tokens = tokenize(line);
   
       //do whatever you want with the commands, here we just print them
		int tok_len=0;
		while(tokens[tok_len]!=NULL) tok_len++;
		/*
		for(i=0;tokens[i]!=NULL;i++){
			printf("found token %s (remove this debug output later)\n", tokens[i]);
		}
		*/
		if(!strcmp(tokens[0],"exit")){
			//kill all background processes
			puts("[.] Killing all Background processes ...");
			for(int i=0; i<100; i++){
				if(bg_pid[i]==INT_MAX) continue;
				kill(bg_pid[i], SIGKILL);
			}
			puts("[.] Freeing memory ...");
			//free allocated memory
			free_tokens();
			printf("\nGoodbye!\n");
			//exit
			exit(0);
		}
		//cd check
		if(!strcmp(tokens[0], "cd")){
			if(!strcmp(tokens[1], "..")){
				char cwd[1000];
				if(getcwd(cwd, sizeof(cwd)) != NULL) {
					char parent[1004];
					snprintf(parent, sizeof(parent), "%s/..", cwd);
					if(chdir(parent)==0) {
						printf("\nChanged to parent directory\n");
						free_tokens();
						continue;
					}
					else perror("chdir() error");
				}
				else {
					perror("getcwd() error");
				}
			} else {
				if(chdir(tokens[1])==0) {
					printf("\nChanged working directory to %s\n", tokens[1]);
					free_tokens();
					continue;
				}
				else perror("chdir() error");
			}

			free_tokens();
			continue;
		}
		//background request check
		if(!strcmp(tokens[tok_len-1], "&")){
			if(bg_ctr==99){
				puts("BG limit reached");
				free_tokens();
				continue;
			}
			tokens[tok_len-1]=NULL;
			int bf=fork();
			if(bf<0){
				fprintf(stderr, "\nProcess failed (fork fail)\n");
				exit(1);
			}
			else if(bf==0){
				execvp(tokens[0], tokens);
				perror("execvp() error");
				exit(1);
			}
			else{
				bg_pid[bg_ctr]=bf;
				bg_ctr++;
				free_tokens();
				continue;
			}
		}
		//foreground exec()
		fg_pid=fork();
		if(fg_pid<0){
      		fprintf(stderr, "\nProcess failed (fork fail)\n");
	        exit(1);
		}
		else if(fg_pid==0) {
			execvp(tokens[0], tokens);
			perror("execvp() error");
			exit(1);	
		}
		else {
			int rc_wait=waitpid(fg_pid, NULL, 0);
			printf("\nFinished forked process : %d\n", fg_pid);
		}
		// Freeing the allocated memory	
		free_tokens();
	}
	return 0;
}
