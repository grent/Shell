/*
 * project 1 (shell) main.c template 
 *
 * project partner names and other information up here, please

Seth Gamble
Pearce Decker
 *
 */

/* you probably won't need any other header files for this project */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <poll.h>
#include <signal.h>
#include <errno.h>
#include <assert.h>


char **tokenify(char *s, char *delims){
	char **toker = malloc(sizeof(char*)*(strlen(s)));
	toker[0] = strtok(s, delims);
	int i = 0;
	while (toker[i] != NULL){
		i+=1;
		toker[i] = strtok(NULL, delims);
	}
	return toker;
}

void removewhitespace(char *s)
{
	if (s != NULL)
	{
		char *newstr = malloc((strlen(s)+1) * (sizeof(char)));
		int newindex = 0;
		int i = 0;
		for (;i<strlen(s);i++)
		{
			if (isspace(s[i]) == 0)
			{
				newstr[newindex]=s[i];
				newindex +=1;
			}
		}
		newstr[newindex] = '\0';
		memcpy (s, newstr, sizeof(char) * (newindex+1));
		free(newstr);
	}
}


int 
main(int argc, char **argv) 
{
	int md = 0;			//mode bit 
	int processcount = 0;		//count number of processes forked for parallel mode
	char *prompt = "boop> ";
	printf("%s", prompt);
	fflush(stdout);
	    
	char buffer[1024];
	while (fgets(buffer, 1024, stdin) != NULL) 
	{
		/* process current command line in buffer */	
		int i = 1;
		int j = 0;
		//count number of semicolons to determine amount of commands (#commands=#semicolons + 1)
		int counter = 1;
		for(;i<strlen(buffer);i++)
		{
			//don't count a semicolon if it follows another semicolon because there will be no command to process
			if (buffer[i] == ';' && buffer[i-1] != ';')	
				counter++;
			if (buffer[i] == '#'){	//ignore comments marked by # character
				buffer[i] = '\0';
				i = strlen(buffer);
			}
		}
		char *wsdelim = " \t\n";
		char *scdelim = ";";
		//first we tokenify the input string by semicolons to seperate commands
		char **cmds = tokenify(buffer, scdelim);
		i = 0;
		for (;i<counter;i++)
		{	
			if (cmds[i] != NULL)
			{
				char *wsr = strdup(cmds[i]);
				removewhitespace(wsr);
				if (wsr != NULL && wsr[0] != '\0')//if a string of whitespace or null, skip
				{
					char *currentcommand = strdup(cmds[i]);
					//now tokenify each individual command by whitespace to separate fields
					char **cmd = tokenify(currentcommand, wsdelim);
					/////CHECK FOR EXIT AND MODE/////
					if (strcmp(currentcommand, "exit") == 0)
					{
						//print usage before exiting
						struct rusage usage;
						getrusage(RUSAGE_SELF, &usage);
						printf("I spent %ld.%06ld seconds in kernel mode \n", usage.ru_stime.tv_sec, usage.ru_stime.tv_usec);
						//say goodbye and exit
						printf("goodbye\n");
						free(cmd);
						free(cmds);
						free(wsr);
						free(currentcommand);
						exit(0);
					}
					else if (strcmp(cmd[0], "mode") == 0)
					{

						if (strcmp("mode", wsr) == 0)    //if user simply types "mode", the shell will switch modes
						{
							if (md == 1)//if mode is switched from parallel to sequential, wait for all processes to finish
							{
								j = 0;
								for (;j<processcount;j++)
								{
									int rstatus = 0;
									pid_t childp = wait(&rstatus);
									printf("Parent got carcass of child process %d, return val %d\n", childp, 										WEXITSTATUS(rstatus));
								}
								processcount = 0;
								md = 0;
								printf("Mode switched from Parallel to Sequential :)\n");
							}
							else if (md == 0)
							{
								md = 1;
								printf("Mode switched from Sequential to Parallel ;)\n");
							}
						}
						//we use strncmp with strlen(cmd[1]) so that any abbrieviations of sequential or parallel
						//will work. ie s,se,seq,sequ will all do the same thing. 
						else if (strncmp(wsr,"modesequential",strlen(wsr)) == 0)
						{
							if (md == 1)
							{
								j = 0;
								for (;j<processcount;j++)//if mode is switched from par to seq wait for processes
								{
									int rstatus = 0;
									pid_t childp = wait(&rstatus);
									printf("Parent got carcass of child process %d, return val %d\n", childp, 										WEXITSTATUS(rstatus));
								}
								processcount = 0;
							}
							md = 0;
							printf("Sequential Mode activated\n");
						}
						else if (strncmp(wsr,"modeparallel",strlen(wsr)) == 0)
						{
							md = 1;
							printf("Parallel Mode activated\n");
						}
						else			//if given an unrecognized mode, print an error and the current mode
						{
							printf("I don't do that mode. ");
							if (md == 0)
							{
								printf("Currently in Sequential Mode\n");
							}
							if (md == 1)
							{
								printf("Currently in Parallel Mode\n");
							}
						}
					}
					else
					/////ALL OTHER KINDS OF COMMANDS/////
					{
						if (md == 1)
						{
							processcount ++;		//processcount increments everytime there is a fork
						}
						pid_t p = fork();
						if (p == 0)
						{
							/* IN CHILD PROCESS */

							struct stat statresult;
							int rv = stat(cmd[0], &statresult);
							if (rv >= 0)//if given path exists do it
							{
								if (execv(cmd[0], cmd) < 0)
								{
									fprintf(stderr, "execv failed: %s\n", strerror(errno));
									return 101;
								}
							}
							else
							{
								//fopen the shell-config
								//stat each path 
								//if you find one that works execv it
								//otherwise print that execv failed 
								FILE *configs = fopen("shell-config", "r");
								if (configs == NULL)        //if the shell-config file isn't around 
								{
									printf("No shell-config file found :(\n");
									return 200;
								}
								char *buffer2 = malloc(200*sizeof(char));
								
								while(fgets(buffer2, 200, configs))
								{
									removewhitespace(buffer2);
									buffer2 = strcat(buffer2, "/");
									char *pathedcommand = malloc((1024 + strlen(buffer2))*sizeof(char));
									pathedcommand = strcat (buffer2, cmd[0]);
									rv = stat(pathedcommand, &statresult);
									if (rv >= 0)
									{
										cmd[0]=pathedcommand;
										if (execv(cmd[0], cmd) < 0)
										{
											fprintf(stderr, "execv failed: %s\n", strerror(errno));
											return 102;
										}
									}
								}
								if (execv(cmd[0], cmd) < 0)
								{
									fprintf(stderr, "execv failed: %s\n", strerror(errno));
									return 103;
								}

							}
						}
						else if (p > 0) 
						{
							   	/* in parent */
						
							if (md == 0)
							{
								int rstatus = 0;
								pid_t childp = wait(&rstatus);
								assert(p == childp);
								printf("Parent got carcass of child process %d, return val %d\n", childp, WEXITSTATUS(rstatus));
							}
						}
						else
						{
						   	/* fork had an error; bail out */
						   	fprintf(stderr, "fork failed: %s\n", strerror(errno));
						}
					}
					free(cmd);
					free(currentcommand);
				}
				free(wsr);
			}
		}
		if (md == 1)
		{
			i = 0;
			for (;i<processcount;i++)
			{
				int rstatus = 0;
				pid_t childp = wait(&rstatus);
				printf("Parent got carcass of child process %d, return val %d\n", childp, WEXITSTATUS(rstatus));
			}
		}
		processcount = 0;
		free(cmds);
		printf("%s", prompt);
		fflush(stdout);
	}
	return 0;
}
