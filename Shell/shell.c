#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/sysinfo.h>
#include "listFiles.c"
#include "sortFiles.c"
#include "checkcpupercentage.c"
#include "checkresidentmemory.c"

// global variables for input command and both arguments array
char inputCommand[1000],*parseArg[100], *parseArgPipe[100]; 

//prototype declaration
void run_command(char **);
void run_command_pipe(char **,char **);
void parseCommand(char *,char **);
void checkresidentmemory(char **);
void checkcpupercentage(char **);
void listFiles(char **);
void sortFiles(char **);
void executeCommands(char **);
void shell();

//for io redirection - both input and output ......this function is called only when atleast 1 io redirection is there
void io_redirection(char *cmd,char *arg[])
{
	char *in=strchr(cmd,'<'); //checking for input redirection
	char *out=strchr(cmd,'>'); // checking for output file 
	char *infile[1],*outfile[1]; // char* array to store name of input and output file if any
	int fd_in,fd_out; //file descriptor for input and output
	int appnd=0; // whether to write output in  append mode or truncated mode
	if(out!=NULL && *(out+1)=='>') // checking for >>
		appnd=1;
	
	//if input file found , then parsing for input file 
	if(in!=NULL) 
	{
		*in='\n';
		parseCommand(cmd,arg);
		if(out!=NULL) // if output file found , parsing for output file
		{
			*out='\n';
			if(appnd)
				parseCommand(out+2,outfile);
			else
				parseCommand(out+1,outfile);
		}
		parseCommand(in+1,infile);	
	}
	else{     // parsing output file
		*out='\n';
		parseCommand(cmd,arg);
		
		if(appnd)
			parseCommand(out+2,outfile);
		else
			parseCommand(out+1,outfile);
	}
	
	// saving original file descriptors so that to use them after command gets complete
	int org_in=dup(STDIN_FILENO);
	int org_out=dup(STDOUT_FILENO);
	
	//setting input to fd_in i.e. input file
	if(in!=NULL)
	{
		fd_in=open(infile[0],O_RDONLY, S_IRUSR | S_IWUSR | S_IRGRP);
		if(fd_in<0)
		{
			printf("Illegal command or arguments\n");
			return;
		}
		close(STDIN_FILENO);
		dup(fd_in);
		
	}
	if(out!=NULL) // setting output to fd_out i.e. output file
	{
		if(appnd)
			fd_out=open(outfile[0],O_CREAT | O_WRONLY | O_APPEND , S_IRUSR | S_IWUSR | S_IRGRP);
		else
			fd_out=open(outfile[0],O_CREAT | O_WRONLY | O_TRUNC , S_IRUSR | S_IWUSR | S_IRGRP);
		if(fd_out<0)
		{
			printf("Illegal command or arguments\n");
			return;
		}
		dup2(fd_out,STDOUT_FILENO);
		
	}
	
	//checking for first argument and executing as it is
	if(arg[0]==NULL)
	{
		printf("Illegal command or arguments\n");
		return;
	}
	if(!strcmp(arg[0],"checkcpupercentage"))
		checkcpupercentage(arg);
	else if(!strcmp(arg[0],"checkresidentmemory"))
		checkresidentmemory(arg);
	else if(!strcmp(arg[0],"listFiles"))
		listFiles(arg);
	else if(!strcmp(arg[0],"sortFiles"))
		sortFiles(arg);
	else if(!strcmp(arg[0],"executeCommands"))
		executeCommands(arg);
	else
	{ // when any buitin command found then fork a child and run it
		int p1,status;
		p1=fork();
		if(p1==0){
			if(execvp(arg[0],arg)<0)
			{
				printf("Illegal command or arguments\n");
				exit(0);
			}
		}
		else{
			int rc=waitpid(p1,&status,0);
			if(!WIFEXITED(status))
				printf("child terminated");
			dup2(org_in,STDIN_FILENO);
			dup2(org_out,STDOUT_FILENO);
		}
	}	
}

// running piped command
void piped_command(char *cmd,char *arg1[], char*arg2[])
{
	// here also we check for input and output redirection
	char *in=strchr(cmd,'<');
	char *out=strchr(cmd,'>');
	char *infile[1],*outfile[1];
	int fd_in,fd_out;
	int appnd=0;
	if(out!=NULL && *(out+1)=='>')
		appnd=1;
	
	char *piped;
	piped=strchr(cmd,'|'); // checking for pipe
	*piped='\n';
	
	//parsing input file name in infile array
	if(in!=NULL)
	{
		*in='\n';
		parseCommand(in+1,infile);	
	}
	//parsing output file name
	if(out!=NULL){
		*out='\n';
		if(appnd)
			parseCommand(out+2,outfile);
		else
			parseCommand(out+1,outfile);
	}
	
	// parsing both commands arguments in array arg1 and arg2 respectively
	parseCommand(cmd,arg1);
	parseCommand(piped+1,arg2);
	
	
	// saving original file descriptors
	int org_in=dup(STDIN_FILENO);
	int org_out=dup(STDOUT_FILENO);
	
	int fd[2];
	// if pipe give error
	if(pipe(fd)==-1)
	{
		printf("pipe error");
		return;
	}
	
	int p1,p2;
	int status1,status2,rc1,rc2;
	
	//parent fork a child and then after another child and waits for both , also if there is io redirection then it sets them accordingly
	p1=fork();
	if(p1!=0)
	{
			if(p2=fork())
			{
				close(fd[1]);
				close(fd[0]);
				rc1=waitpid(p1,&status1,0);
				rc2=waitpid(p2,&status2,0);
				dup2(org_in,STDIN_FILENO);
				dup2(org_out,STDOUT_FILENO);
				return;
			}
			else
			{	
				if(out!=NULL) // setting output redirection
				{
					if(appnd)
						fd_out=open(outfile[0],O_CREAT | O_WRONLY | O_APPEND , S_IRUSR | S_IWUSR | S_IRGRP);
					else
						fd_out=open(outfile[0],O_CREAT | O_WRONLY | O_TRUNC , S_IRUSR | S_IWUSR | S_IRGRP);
					if(fd_in<0)
					{
						printf("Illegal command or arguments\n");
						exit(0);
					}
					dup2(fd_out,STDOUT_FILENO);
		
				}		
				dup2(fd[0],STDIN_FILENO);
				close(fd[1]);
				close(fd[0]);
				if(execvp(arg2[0],arg2)<0)
				{
					printf("Illegal command or arguments\n");
					exit(0);
				}
			}	
	}
	else
	{
		if(in!=NULL) // setting input redirection
		{
			fd_in=open(infile[0],O_RDONLY, S_IRUSR | S_IWUSR | S_IRGRP);
			if(fd_in<0)
			{
				printf("Illegal command or arguments\n");
				exit(0);
			}
			close(STDIN_FILENO);
			dup(fd_in);
					
		}
		dup2(fd[1],STDOUT_FILENO);
		close(fd[0]);
		close(fd[1]);
		
		if(!strcmp(arg1[0],"checkcpupercentage"))
			checkcpupercentage(arg1);
		else if(!strcmp(arg1[0],"checkresidentmemory"))
			checkresidentmemory(arg1);
		else if(!strcmp(arg1[0],"listFiles"))
			listFiles(arg1);
		else if(!strcmp(arg1[0],"sortFiles"))
			sortFiles(arg1);
		else if(!strcmp(arg1[0],"executeCommands"))
			executeCommands(arg1);
		else
		{ // when any buitin command found then fork a child and run it
			if(execvp(arg1[0],arg1)<0)
			{
				printf("Illegal command or arguments\n");
				exit(0);
			}
		}
			
	}
}


// for executing commands from a file
void executeCommands(char *args[])
{
		if(args[1]==NULL || args[2]!=NULL)
		{
			printf("Illegal command or arguments\n");
			return;
		}
		
		FILE *fp;// file pointer pointing to current location in file
		fp=fopen(args[1],"r");
		if(fp==NULL)
		{
			printf("Illegal command or arguments\n");
			return;
		}
		
		char cmd1[1000]; // input array for each command
		char *line;
			
		while(fgets(cmd1,1000,fp)!=NULL) // fgets reads till EOF or \n
		{
		  
			
			char *parallel,*pipe,*in,*out;
			char *arg1[100],*arg2[100];
			
			line=cmd1;
			int cmd=1;
				
			if(*line=='\n')
			{
				printf("Illegal command or arguments\n");
				continue;
			}
				
			//cchecking for pipe,prallel and io redirection
			parallel=strchr(line,';');
			pipe=strchr(line,'|');
			in =strchr(line,'<');
			out=strchr(line,'>');
			
			if(pipe!=NULL)
			{
				//printf("----------going for pipe-------------");
				piped_command(cmd1,arg1,arg2);
				continue;
			}
			
			if(in!=NULL || out!=NULL)
			{
				// --------------------going for io redirection-----------------------------
				io_redirection(line,arg1);
				continue;
			}
		
			if(parallel !=NULL){
			//--------------------------going for parallel execution-------------------
				cmd++;
				*parallel='\n';
				char *temp=parallel+1;
				while(*temp!='\n')
					temp++;
				*temp='\n';
				parseCommand(parallel+1,arg2);
			}
		
			parseCommand(line,arg1);
			
		
			if(cmd==1)
			{
				int i=0;
				run_command(arg1);
			}
			
			if(cmd==2) // if parallel command then run_command_pipe will take care of both
			{
				
				run_command_pipe(arg1,arg2);
			}
			
		}
		fclose(fp);
}

// to run parallel commands - i.e. seperated by ;
void run_command_pipe(char *arg[],char *argPipe[])
{
	int p1,p2;
	int status1,status2,rc1,rc2;
	if((p1=fork())!=0) // first child forked
	{
		if(p2=fork())  // second chiled forked
		{
			//waiting for the childs
			rc1=waitpid(p1,&status1,0);
			rc2=waitpid(p2,&status2,0);
			if(!WIFEXITED(status1))
				printf("child not terminated");
			if(!WIFEXITED(status2))
				printf("child not terminated");
		}
		else{
			run_command(argPipe);
			exit(0);
			}
	}
	else
	{
	
		run_command(arg);
		exit(0);
	}
	
		
}

// run commands by checking command name
void run_command(char *args[])
{

	if(args[0]==NULL)
	{
		printf("Illegal command or arguments\n");
		return;
	}

	if(!strcmp(args[0],"checkcpupercentage"))
		checkcpupercentage(args);
	else if(!strcmp(args[0],"checkresidentmemory"))
		checkresidentmemory(args);
	else if(!strcmp(args[0],"listFiles"))
		listFiles(args);
	else if(!strcmp(args[0],"sortFiles"))
		sortFiles(args);
	else if(!strcmp(args[0],"executeCommands"))
		executeCommands(args);
	else if(!strcmp(args[0],"exit"))
	{
		while(wait(NULL));
		exit(0);
	}
	else if(!strcmp(args[0],"kill"))
	{
		exit(0);
		//kill_process(args);
	}
	else
	{
		printf("Illegal command or arguments\n");
		return;
	}

}

//parsing commands and storing each argument in a array
void parseCommand(char *input,char *args[])
{
	
	char *temp=input;
	int i=0;
	int flag=0;
	while(*temp!='\0' && *temp!='\n')
	{
		while(*temp==' ')
			temp++;
		input=temp;
		while(*temp!=' ' && *temp!='\n' && *temp!='\0')
			temp++;
		if(*temp =='\0' || *temp=='\n')
			flag=1;
		*temp='\0';
		
	
		if(temp!=input)	
			args[i]=input;
		if(flag)
			break;
		temp++;
		i++;
	}
}

//signal handler for ^C or sigint
void handler1()
{
	write(STDOUT_FILENO , "the program is interrupted, do you want to exit [Y/N]\n",55);
	char c;
	read(0,&c,1);
	if(c=='Y')
		exit(0);
}

//signal handler for sigterm
void handler2()
{
	printf("got SIGTERM-Leaving\n");
	exit(0);
	
}

// out main shell , while loop runs - each time it takes input commandd from user , check for io redirection , pipe , parallel 
//then parse the command arguments to global array and then run then using run_command .. if io redirection found , it directly class that function
// similarly for piped command also it call that function and continues
void shell(char *inputCommand, char *parseArg[] ,char *parseArgPipe[])
{
	
	while(1)
	{
		
		printf("\n%s","myShell>");
		
		//fgets take command from input
		if(fgets(inputCommand,1000,stdin)==NULL || *inputCommand=='\n')
		{
			if(*inputCommand=='\n')
				continue;
	
			printf("Illegal command or arguments\n");
			memset(inputCommand,0,1000);	
			continue;	
		}
		
		
			
		char *parallel,*pipe,*in,*out;
		char *line=inputCommand;
		int cmd=1;
		
		parallel=strchr(line,';');
		pipe=strchr(line,'|');
		in =strchr(line,'<');
		out=strchr(line,'>');
		
		// if pipe then go to function piped_command()
		if(pipe!=NULL)
		{
			piped_command(line,parseArg,parseArgPipe);	
			continue;
		}
		
		// if io redirction , then go to io_redireciton()
		if(in!=NULL || out!=NULL)
		{
			io_redirection(line,parseArg);
			continue;
		}
		
		// if parallel commands
		if(parallel !=NULL){
			cmd++;
			*parallel='\n';
			char *temp=parallel+1;
			while(*temp!='\n')
				temp++;
			*temp='\n';
			
			parseCommand(parallel+1,parseArgPipe);
		}
		
		parseCommand(line,parseArg);
		
		
		if(cmd==1)
		{
			run_command(parseArg);
		}
		else if(cmd==2)
		{
			run_command_pipe(parseArg,parseArgPipe);
		}
		
		// emptying all global variables 
		memset(inputCommand,0,1000);
		memset(parseArg,0,800);
		memset(parseArgPipe,0,800);
	}


}


int main()
{
	
	signal(SIGINT,handler1);
	signal(SIGTERM,handler2);
	shell(inputCommand , parseArg ,parseArgPipe);
	return 0;
	
}
