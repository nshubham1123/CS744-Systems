void sortFiles(char *argv[])
{

	if(argv[1]==NULL || argv[2]!=NULL)
	{
		
		printf("Illegal command or arguments\n");
		return ;
	}
	char *file=argv[1];
	
	int status;
	int rc;
	int ch=fork();
	
	if(ch<0)
	{
		printf("Illegal command or arguments\n");
		return ;
	}
	else if(ch==0)
	{
		
		if(execlp("sort","sort",argv[1],NULL)<0)
		{
			printf("Illegal command or arguments\n");
			exit(0);
		}
	}
	else
	{
		rc=waitpid(ch,&status,0);
		if(!WIFEXITED(status))
			printf("child not terminated");
	}
	return ;
}
