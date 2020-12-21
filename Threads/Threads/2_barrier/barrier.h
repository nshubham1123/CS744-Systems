#include <pthread.h>

struct barrier_t
{
	/*
		Todo
		Barrier related variables
	*/
	int nthread;
	int nwaiting,nfree;
	pthread_mutex_t lock;
	pthread_cond_t cv;
};

void barrier_init(struct barrier_t *b, int i);
void barrier_wait(struct barrier_t *b);
