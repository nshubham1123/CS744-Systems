#include<stdio.h>
#include<stdlib.h>
#include <pthread.h>
#include <unistd.h>

/* 
 * This is the counter value which is to
 * be incremented by all the threads created 
 * by your program
*/
int counter = 0;
pthread_mutex_t lock;

void* inc(void * arg)
{
	pthread_mutex_lock(&lock);
	counter++;
	pthread_mutex_unlock(&lock);
}

int main(int argc, char const *argv[])
{
	/* Write you code to create n threads here*/
	/* Each thread must increment the counter once and exit*/
	
	int n=atoi(argv[1]);
	pthread_t thread[n];
	
	pthread_mutex_init(&lock,NULL);
	
	for(int i=0;i<n;i++)
		pthread_create(&thread[i],NULL,&inc,NULL);
		
	for(int i=0;i<n;i++)
		pthread_join(thread[i],NULL);


	/* Make sure you reap all threads created by your program
	 before printing the counter*/
	printf("%d\n", counter);
	pthread_mutex_destroy(&lock);
	sleep(10000);
	
	return 0;
}
