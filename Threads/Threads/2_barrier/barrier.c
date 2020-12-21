#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "barrier.h"

void barrier_init(struct barrier_t * barrier, int nthreads)
{
	barrier->nthread=nthreads;
	barrier->nwaiting=0;
	
	pthread_mutex_init(&barrier->lock,NULL);
	pthread_cond_init(&barrier->cv,NULL);
	return;
}

void barrier_wait(struct barrier_t *barrier)
{
	
	pthread_mutex_lock(&barrier->lock);
	
	barrier->nwaiting+=1;
	
	//printf("called by thread %d\n",barrier->nwaiting);
	while(barrier->nwaiting != barrier->nthread)
	{
		int signaled=pthread_cond_wait(&barrier->cv,&barrier->lock);
		if(!signaled)
			break;
	}
	//printf("out of loop %d\n",barrier->nfree);
	if(barrier->nwaiting==barrier->nthread){
		pthread_cond_broadcast(&barrier->cv);
		barrier->nwaiting=0;
	}	
	pthread_mutex_unlock(&barrier->lock);
	
	return;
}
