void listFiles(char *argv[])
{
	if(argv[1]!=NULL)
	{
		printf("Illegal command or arguments\n");
		return ;
	}
	int ch,status;
	int rc=fork();
	
	if(rc<0)
	{
		printf("Illegal command or arguments\n");
		return ;
	}
	else if(rc==0)
	{
		
		close(STDOUT_FILENO);
		int fd=open("files.txt",O_CREAT | O_WRONLY | O_TRUNC , S_IRUSR | S_IWUSR | S_IRGRP);
		dup2(fd,STDOUT_FILENO);
		
		if(execlp("ls","ls",NULL)<0)
		{
			printf("Illegal command or arguments");
			exit(0);
		}
	}
	else
	{
		
		ch=waitpid(rc,&status,0);
		if(!WIFEXITED(status))
			printf("child not terminated\n");
	}
}
