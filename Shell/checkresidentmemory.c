void checkresidentmemory(char *argv[])
{
	if(argv[1]==NULL || argv[2]!=NULL)
	{
		printf("Illegal command or arguments");
		return ;
	}

	char* pid=argv[1];
	
	// command is - "ps --no-headers --pid [pid] -o rsssize"
	char *arg[7];
	arg[0]="ps";
	arg[1]="--no-headers";
	arg[2]="--pid";
	arg[3]=pid;
	arg[4]="-o";
	arg[5]="rssize";
	arg[6]=NULL;
	int status,ch;
	int rc=fork();
	int fd,org;
	fd=open("tempshubham.txt",O_CREAT | O_WRONLY | O_TRUNC , S_IRUSR | S_IWUSR | S_IRGRP);
	
	if(rc<0)
	{
		printf("Illegal command or arguments");
		return ;
	}
	else if(rc==0)
	{
		org=dup(STDOUT_FILENO);
		dup2(fd,STDOUT_FILENO);
		if(execvp("ps",arg)<0)
		{
			printf("Illegal command or arguments");
			exit(0);
		}
	  
	}
	else
	{
		ch=waitpid(rc,&status,0);
		if(!WIFEXITED(status))
				printf("child not terminated");
		dup2(org,STDOUT_FILENO);	
		
		FILE *fp=fopen("tempshubham.txt","r");
		char ans[25];
		fgets(ans,25,fp);
		for(int i=1;ans[i]!='\n';i++)
			printf("%c",ans[i]);	
		printf("\n");
		remove("tempshubham.txt");
	}
	return ;
}
