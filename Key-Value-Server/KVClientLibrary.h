#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include <string.h>

void encode_client_request(char* buf,char* KVmsg)
{
	char command[4];
	char key[256];
	char value[256];
	bzero(key,256);
	bzero(value,256);
	bzero(command,4);
	for(int i=0;i<520;i++)
	{
		if(buf[i]=='\n'|| buf[i]==',')
		{
			buf[i]=' ';

		}
	
	}
	sscanf(buf,"%s %s %s", command ,key,value );
	//printf("command - %s,%d,key is- %s,%d ,value is- %s,%d\n",command,strlen(command),key,strlen(key),value,strlen(value));
		
	//char* KVmsg = (char*)malloc(513);
	
	if(strlen(key)>256 || strlen(value)>256)
	{
		// key length exceed return error;
		KVmsg[0]=240;
		char* error="Key or Value must be less than 256 bytes";
		memcpy(&KVmsg[1],error,256);
		//memcpy(&KVmsg[257],&value,256);
	
	}
	else if(( strcmp(command,"PUT")==0 || strcmp(command,"GET")==0 ||strcmp(command,"DEL")==0) && strlen(key)==0)
	{
		// key length exceed return error;
		KVmsg[0]=240;
		char* error="Key not given";
		memcpy(&KVmsg[1],error,256);
		//memcpy(&KVmsg[257],&value,256);
	
	}
	else if(strlen(value)==0 && strcmp(command,"PUT")==0)
	{
		// key length exceed return error;
		KVmsg[0]=240;
		char* error="Value not given";
		memcpy(&KVmsg[1],error,256);
		//memcpy(&KVmsg[257],&value,256);
	
	}
	else if(strcmp(command,"PUT")==0)
	{
		KVmsg[0]=2;
		memcpy(&KVmsg[1],&key,256);
		memcpy(&KVmsg[257],&value,256);
	
	}
	else if(strcmp(command,"GET")==0)
	{
		KVmsg[0]=1;
		memcpy(&KVmsg[1],&key,256);
		//memcpy(&KVmsg[257],&value,256);
	}
	else if(strcmp(command,"DEL")==0)
	{
		KVmsg[0]=3;
		memcpy(&KVmsg[1],&key,256);
		//memcpy(&KVmsg[257],&value,256);
	}
	else
	{
		KVmsg[0]=240;
		char* error="Wrong Command";
		memcpy(&KVmsg[1],error,256);
	}
//	return KVmsg;
}

char* decode_server_response(char* KVresponse)
{
	for(int i=0;i<512;i++)
	{
		if(KVresponse[i]==' ' && KVresponse[i+1]==' ')
			KVresponse[i]='\0';
	}
	return &KVresponse[1];	

}

 
