#include "common.h"
#include "KVcache.h"

pthread_t* thread=NULL;
int NO_THREADS;
int PORT_NO;
int CLIENTS_PER_THREAD;
int CACHE_SIZE;
char CACHE_REPLACEMENT[4];
int THREAD_POOL_SIZE_INITIAL;
int THREAD_POOL_GROWTH;


struct epoll_arg{
	pthread_mutex_t thread_mutex;
	pthread_cond_t thread_cond;
	int thread_started;
	int epollfd;
	int current_clients;
	
};
struct epoll_arg* args=NULL;
void register_client_to_epoll(struct epoll_arg* , int );
void* epoll_process(void*);
int find_worker_thread_no(struct epoll_arg*,int);
time_t start[256],end[256];

void error(const char *msg)
{
	perror(msg);
	exit(1);
}

int make_socket_non_blocking (int sfd)
{
  int flags, s;

  flags = fcntl (sfd, F_GETFL, 0);
  if (flags == -1)
    {
      perror ("fcntl");
      return -1;
    }

  flags |= O_NONBLOCK;
  s = fcntl (sfd, F_SETFL, flags);
  if (s == -1)
    {
      perror ("fcntl");
      return -1;
    }

  return 0;
}


void read_config_file()
{
	FILE* fp=fopen("config.csv","r+");
	char buffer[100];
	
	PORT_NO = atoi(fgets(buffer,100,fp));
	bzero(buffer,0);
	
	CLIENTS_PER_THREAD = atoi(fgets(buffer,100,fp));
	
	bzero(buffer,0);
	
	CACHE_SIZE = atoi(fgets(buffer,100,fp));
	bzero(buffer,0);
	
	fgets(buffer,100,fp);
	strcpy(CACHE_REPLACEMENT ,buffer);
	CACHE_REPLACEMENT[3]='\0';
	bzero(buffer,0);
	
	
	
	THREAD_POOL_SIZE_INITIAL = atoi(fgets(buffer,100,fp));
	NO_THREADS = THREAD_POOL_SIZE_INITIAL;
	bzero(buffer,0);
	
	THREAD_POOL_GROWTH = atoi(fgets(buffer,100,fp));
	bzero(buffer,0);
}





int main(int argc,char *argv[])
{
	read_config_file();
	
	int sockfd , newsockfd  , n;
	char buffer[255];
	
	struct sockaddr_in serv_addr , cli_addr;  // sockaddr_in gives us internet address , included in netinet/in.h
	socklen_t clilen;	// data type in socket.h , 32 bit
	
	sockfd = socket(AF_INET , SOCK_STREAM , 0);
	if(sockfd < 0)
	{
		error("error opening socket");
	}
	int s;
	
	bzero((char *)&serv_addr , sizeof(serv_addr) );
	
	
	
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(PORT_NO);
	
	if(bind(sockfd , (struct sockaddr*)&serv_addr , sizeof(serv_addr)) < 0)
		error("binding failed");
		
	listen(sockfd , 5);
	struct epoll_event ev, events[CLIENTS_PER_THREAD];
	
	printf("Hello , This is KVServer\n");
	printf("open to accept connections\n");
	
	
	
	
	thread =(pthread_t*)malloc(sizeof(pthread_t)*NO_THREADS);
	//int epollfd[NO_THREADS];
	 args=(struct epoll_arg*)malloc(sizeof(struct epoll_arg)*NO_THREADS);
	
	for(int i=0;i<NO_THREADS;i++)
	{
		// epollfd[i] = epoll_create1(0);
           	//if (epollfd[i] == -1) {
               //perror("epoll_create1");
               //exit(EXIT_FAILURE);
           	//}
           	
		args[i].epollfd = -1;
		args[i].thread_started = 0;
		args[i].current_clients = 0;
		
		pthread_mutex_init( &args[i].thread_mutex , NULL);
		pthread_cond_init( &args[i].thread_cond , NULL );
		pthread_create(&thread[i],NULL,&epoll_process,(void*)&args[i]);
	}
	int cur=0;
	
	initialize_cache(CACHE_SIZE);
	initialize_files();

	while(1){
	 struct sockaddr in_addr;
          socklen_t in_len;
                  int infd;
                  in_len = sizeof(in_addr);
                  infd = accept (sockfd, &in_addr, &in_len);
                  if (infd == -1)
                    {
                      printf("error on accept\n");
                      exit(1);
                    }

                  //start[infd]=clock();
                  
                  s = make_socket_non_blocking (infd);
                  if (s == -1)
                    abort ();
	    int thread_no = find_worker_thread_no(args,cur);
            register_client_to_epoll(&args[thread_no],infd);
            cur = (thread_no+1)%NO_THREADS;
            
            
	//epoll_process(epollfd,sockfd);
	}
	//close(newsockfd);
	for(int i=0;i<NO_THREADS;i++)
		pthread_join(thread[i],NULL);
	close(sockfd);
	return 0;

}

void increase_thread_pool()
{
	int cur_pool_size = NO_THREADS;
	NO_THREADS+=THREAD_POOL_GROWTH;
	struct epoll_arg* temp;
	args = realloc(args , sizeof(struct epoll_arg)*NO_THREADS);
	//args = temp;
	printf("realloc done in args\n");
	
	thread = realloc(thread , sizeof(pthread_t)*NO_THREADS);
	
	printf("realloc done in thread\n");
	
	for(int i=cur_pool_size;i<NO_THREADS;i++)
	{
		args[i].epollfd = -1;
		args[i].thread_started = 0;
		args[i].current_clients = 0;
		
		pthread_mutex_init( &args[i].thread_mutex , NULL);
		pthread_cond_init( &args[i].thread_cond , NULL );
		pthread_create(&thread[i],NULL,&epoll_process,(void*)&args[i]);
	}
	
}

int find_worker_thread_no(struct epoll_arg* args,int cur)
{
	int i=cur;
	do{
		pthread_mutex_lock(&args[i].thread_mutex);
		if(args[i].current_clients < CLIENTS_PER_THREAD){
			pthread_mutex_unlock(&args[i].thread_mutex);
			return i;
			}
		pthread_mutex_unlock(&args[i].thread_mutex);
		i = (i+1)%NO_THREADS;
	}while(i!=cur);
	int cur_pool_size = NO_THREADS;
	increase_thread_pool();
	return cur_pool_size;
}


void register_client_to_epoll(struct epoll_arg* arg,int infd)
{
	//printf("address of epoll arg is %d\n",arg);
	//printf("waiting for mjutex lock\n");
	pthread_mutex_lock(&arg->thread_mutex);
//	printf("lock acquired\n");
	if(arg->thread_started==0)
	{
		// create epoll instance 
		
		arg->epollfd = epoll_create1(0);
           	if (arg->epollfd == -1) {
               	perror("epoll_create1");
           		exit(EXIT_FAILURE);
           	}
           	arg->thread_started=1;
           	int s = pthread_cond_signal(&arg->thread_cond);
           //	if(s==0)
  //         		printf("%d thread signaled and started %d\n",arg->epollfd,arg->thread_started);
	}
	// add client
	int s;
	struct epoll_event ev;
	ev.data.fd = infd;
        ev.events = EPOLLIN;// | EPOLLET;
        s = epoll_ctl (arg->epollfd, EPOLL_CTL_ADD, infd, &ev);
        if (s == -1)
        {
        	perror ("epoll_ctl");
                exit(1);
        }
        arg->current_clients+=1;
        printf("client with fd %d is succesfully accepted on thread epoll with fd %d\n",infd,arg->epollfd);
        pthread_cond_signal(&arg->thread_cond);
        pthread_mutex_unlock(&arg->thread_mutex);

}

void handle_request_from_client(char* KVmsg,int count,int fd)
{
	char* key=(char*)malloc(257);
	char* value=(char*)malloc(257);
	bzero(key,257);
	bzero(value,257);
	memset(key,' ',257);
	memset(value,' ',257);
	key[256]='\0';
	value[256]='\0';
	
	// memcpy(destination , source , no_of_bytes )
	memcpy(key,&KVmsg[1],256);
	memcpy(value,&KVmsg[257],256);
	
	
	char KVresponse[513];
	bzero(KVresponse,513);
	
	printf("status code is %u\n",(unsigned char)KVmsg[0]);
	if(KVmsg[0]==2)
	{
		//printf("call to insert to cache\n");
		int s;
		if(strcmp(CACHE_REPLACEMENT,"LFU")==0)
			s=insert_to_cache_lfu(key,value);	
		else
			s=insert_to_cache(key,value);
		if(s==1){
			KVresponse[0]=200;
			memcpy(&KVresponse[1],"Success",7);
		}
		else{
			KVresponse[0]=240;
			memcpy(&KVresponse[1],"Does not exist",14);
		}
	}
	else if(KVmsg[0]==1)
	{
		if(strcmp(CACHE_REPLACEMENT,"LFU")==0)
			value=find_value_lfu(key);	
		else
			value = find_value(key);
		//printf("value fetched si %s\n",value);
		if(strcmp(value,"key not found")!=0){
			KVresponse[0]=200;
			memcpy(&KVresponse[1],key,strlen(key));
			KVresponse[strlen(key)+1]=',';
			memcpy(&KVresponse[strlen(key)+2],value,strlen(value));
		}
		else{
			KVresponse[0]=240;
			memcpy(&KVresponse[1],"Does not exist",14);
		}
	}
	else if(KVmsg[0]==3)
	{
		int s;
		if(strcmp(CACHE_REPLACEMENT,"LFU")==0)
			s=delete_from_cache_lfu(key);	
		else
			s=delete_from_cache(key);
		if(s==1){
			KVresponse[0]=200;
			memcpy(&KVresponse[1],"Success",7);
		}
		else{
			KVresponse[0]=240;
			memcpy(&KVresponse[1],"Does not exist",14);
		}
	}
	else
	{
		memcpy(&KVresponse,KVmsg,513);
	}
	
	
	write(fd,KVresponse,513);
	print_cache();
	
}




void* epoll_process(void* args)
{
	struct epoll_arg* arg= (struct epoll_arg*)args;
	//printf("thread created but epoll not assigned\n");
	//printf("address of epoll arg is %d\n",arg);	
	pthread_mutex_lock(&arg->thread_mutex);
	
	while(arg->thread_started==0){
		//printf("inside while loop and starte is %d\n",arg->thread_started);
		//printf("thread waiting in cond wait\n");
		pthread_cond_wait(&arg->thread_cond , &arg->thread_mutex);
		//printf("thread come out of cond wait\n");
		
	}
	
	pthread_mutex_unlock(&arg->thread_mutex);
	int epollfd = arg->epollfd;
	struct epoll_event events[CLIENTS_PER_THREAD];
	printf("%d thread started\n",epollfd);
	while(1)
	{
	
	
	int nfds;
	//fprintf(stdout , "currently active fds before \n");
	nfds = epoll_wait(epollfd , events , CLIENTS_PER_THREAD , -1);
	//fprintf(stdout , "currently active fds after \n");
	char buf[513];
	for(int i=0;i<nfds;i++)
	{
			int done = 0;
		printf("reading from previous client with fd = %d on epoll thread id = %d\n",events[i].data.fd,epollfd);
              while (1)
                {
                  ssize_t count;
                  
		bzero(buf , 513);
                  count = read (events[i].data.fd, buf, sizeof(buf));
                  if (count == -1)
                    {
                      /* If errno == EAGAIN, that means we have read all
                         data. So go back to the main loop. */
                      if (errno != EAGAIN)
                        {
                          perror ("read");
                          done = 1;
                        }
                      break;
                    }
                  else if (count == 0)
                    {
                      /* End of file. The remote has closed the
                         connection. */
                      done = 1;
                      break;
                    }

                  /* Write the buffer to standard output */
                
                    handle_request_from_client(buf,count,events[i].data.fd);
                  /*  printf("client: %s\n",buf);
                    int n = write(events[i].data.fd,buf,count);
                    if(n<0)
                    	printf("error in write\n");*/
                    int cmp = strncmp("Bye" , buf ,3);
		
		if(cmp==0)
			done=1;
			break;
                }

              if (done)
                {
                  printf ("Closed connection on descriptor %d\n",
                          events[i].data.fd);

                  /* Closing the descriptor will make epoll remove it
                     from the set of descriptors which are monitored. */
                    pthread_mutex_lock(&arg->thread_mutex);
                  
                  arg->current_clients-=1;
                  /*
                  int fd=events[i].data.fd;		
                  end[fd]=clock();
                  double total_time = (double)((end[fd] - start[fd])*1000/(CLOCKS_PER_SEC));
		double throughput = 10000.0/total_time*1000;
		printf("total time to process client is %f msec and throughput is %f req/sec\n",total_time,throughput);
                  */
                  pthread_mutex_unlock(&arg->thread_mutex);
                  close (events[i].data.fd);
                }
	
	
	
	
	}
	}



}

