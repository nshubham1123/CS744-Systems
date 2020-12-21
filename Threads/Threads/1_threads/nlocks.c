#include<stdio.h>
#include<stdlib.h>
#include <pthread.h>
#include <unistd.h>
/* 
 * This is the data array which is to
 * be incremented by all the threads created 
 * by your program as specified in the problem state
*/
int data[10];
pthread_mutex_t lock[10];

void* add(void* arg)
{
	int thread_no=(int)arg;
	//printf("%d",thread_no);
	for(int i=0;i<1000;i++)
	{
		pthread_mutex_lock(&lock[thread_no]);
		data[thread_no]++;
		pthread_mutex_unlock(&lock[thread_no]);
	}
}

int main(int argc, char const *argv[])
{
	/* Write you code to create 10 threads here*/
	/* Increment the data array as specified in the problem statement*/
	pthread_t thread[10];
	
	for(int i=0;i<10;i++){
		pthread_mutex_init(&lock[i],NULL);
		pthread_create(&thread[i],NULL,add,(void*)i);
	}
		
	for(int i=0;i<10;i++)
	{
		add((void*)i);
	}
	
	for(int i=0;i<1;i++)
		pthread_join(thread[i],NULL);

	/* Make sure you reap all threads created by your program
	 before printing the counter*/
	for (int i = 0; i < 10; ++i)
	{
		printf("%d\n", data[i]);
	}
	for(int i=0;i<10;i++)
		pthread_mutex_destroy(&lock[i]);	
	sleep(10000);
	return 0;
}
