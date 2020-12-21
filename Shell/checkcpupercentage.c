
void checkcpupercentage(char *argv[])
{

	if(argv[1]==NULL || argv[2]!=NULL)
	{
		printf("Illegal command or arguments");
		return ;
	}
	int status,rc;
	int ch=fork();
	
	if(ch==0)
	{
		char path1[100]="/proc/";
		char *temp=path1;
	
		while(*temp!='\0')
			temp++;
		strncpy(temp,argv[1],sizeof(argv[1]));
		
		while(*temp!='\0')
			temp++;
		strncpy(temp,"/stat",6);
		
		FILE* fp;
		
		fp=fopen(path1,"r");
		if(fp==NULL){
			printf("Illegal command or arguments");
			return ;
		}
		
		int spaces=13;
		int c=fgetc(fp);
		
		//since we require 14th nd 15th entry so we count for 13 spaces
		while(spaces)
		{
			int c=fgetc(fp);
			if(c==' ')
				spaces--;
		}
		//calculating user time and cpu time at start
		float utime_start=0,stime_start=0;
		
		while((c=fgetc(fp))!=' ')
			utime_start=utime_start*10+(c-'0');
		
		while((c=fgetc(fp))!=' ')
			stime_start=stime_start*10+( c-'0');

		
		fp=fopen("/proc/stat","r");
		if(fp==NULL)
		{
			printf("Illegal command or arguments");
			return ;
		}
		//calculating total cpu time at start
		while((c=fgetc(fp))!=' ');
		float ttime_start=0;
		
		while(c!='\n')
		{
			int cur=0;
			while((c=fgetc(fp))!=' ' && c!='\n')
				cur=cur*10+(c-'0');
			ttime_start+=cur;
		}

		
		while(sleep(1)); ///delay for 1 sec an then calcualting again
		
		fp=fopen(path1,"r");
		if(fp==NULL)
		{
			printf("Illegal command or arguments");
			return ;
		}
		
		 spaces=13;
		 c=fgetc(fp);
		
		while(spaces)
		{
			int c=fgetc(fp);
			if(c==' ')
				spaces--;
		}
		float utime_end=0,stime_end=0;
		// calculating user time  and system time at end
		while((c=fgetc(fp))!=' ')
			utime_end=utime_end*10+(c-'0');
		
		while((c=fgetc(fp))!=' ')
			stime_end=stime_end*10+( c-'0');
		
		fp=fopen("/proc/stat","r");
		if(fp==NULL)
		{
			printf("Illegal command or arguments");
			return ;
		}
		// calculating total cpu time end
		while((c=fgetc(fp))!=' ');
		float ttime_end=0;
		
		while(c!='\n')
		{
			int cur=0;
			while((c=fgetc(fp))!=' ' && c!='\n')
				cur=cur*10+(c-'0');
			ttime_end+=cur;
		}
		// calculation of user and system cpu usage
		float user=((utime_end-utime_start)*100)/(ttime_end-ttime_start);
		float system=((stime_end-stime_start)*100)/(ttime_end-ttime_start);
		
		printf("user mode cpu percentage: %f%%\n",user);
		printf("system mode cpu percentage: %f%%\n",system);
		fclose(fp);
		exit(0);
	}
	else
	{
		rc=waitpid(ch,&status,0);
	}
	return ;
}	
