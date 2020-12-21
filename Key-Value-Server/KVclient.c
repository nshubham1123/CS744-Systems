#include "common.h"
#include "KVClientLibrary.h"

int PORT_NO;

void error(const char *msg)
{
	perror(msg);
	exit(1);
}

void read_config_file()
{
	FILE* fp=fopen("config.csv","r+");
	char buffer[100];
	
	PORT_NO = atoi(fgets(buffer,100,fp));
}


int main(int argc,char *argv[])
{
	read_config_file();
	int sockfd , n;
	char* buffer=(char*)malloc(520);
	char* KVmsg=(char*)malloc(513);
	
	struct sockaddr_in serv_addr ;  // sockaddr_in gives us internet address , included in netinet/in.h
	struct hostent *server;	// netdb.h se
//	printf("here");
	sockfd = socket(AF_INET , SOCK_STREAM , 0);
	if(sockfd < 0)
	{
		error("error opening socket");
	}
	
	
	//printf("here");
	server = gethostbyname("127.0.0.1");
	if(server == NULL)
	{
		fprintf(stderr ,"error , no such host");
		exit(0);
	}
	//printf("here");
	bzero((char *)&serv_addr , sizeof(serv_addr) );
	
	serv_addr.sin_family = AF_INET;
	bcopy((char*) server->h_addr , (char*)& serv_addr.sin_addr.s_addr , server->h_length);
	serv_addr.sin_port = htons(PORT_NO);
//	printf("here");
	if(connect(sockfd , (struct sockaddr*)& serv_addr , sizeof(serv_addr)) < 0)
		error("connection failed");
	
	//char line[520];
	printf("Hello , Welcome to KVServer\n");
	printf("USAGE details : \n");
	printf("PUT key value\nGET key\nDEL key\n");
	
	
	
	while(1)
	{
		bzero(buffer , 520);
		bzero(KVmsg,513);
		fgets(buffer , 520 ,stdin);
	//	printf("client read is %s\n",line);
		// copy from buffer to KVmsg with specified format
		encode_client_request(buffer,KVmsg);
		
		n = write(sockfd , KVmsg ,513);
		if(n < 0)
			error("error on write");
			
		bzero(buffer , 520);
		bzero(KVmsg , 513);
		n = read(sockfd , KVmsg ,513);
		if(n < 0)
			error("error on read");
		//copy from KVmsg to buffer , user understandable format
		buffer = decode_server_response(KVmsg);
		
		//fputs(buffer,fp_out);	
		//fputs("\n",fp_out);
		printf("Server : %s\n",buffer);
		
		int i = strncmp("Bye" , buffer ,3);
		bzero(buffer , 520);
		//bzero(line,520);
		if(i==0)
			break;
	}
	//fclose(fp_in);
	//fclose(fp_out);
	close(sockfd);
	return 0; 
}
