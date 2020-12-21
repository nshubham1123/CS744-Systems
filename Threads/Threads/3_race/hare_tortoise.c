#include "cs_thread.h"
#include <pthread.h>

struct Repositioning {
	char player; 		// T for turtle and H for hare
	int time; 		// At what time god interrupt's
	int distance;		// How much does god move any of the player. 
							// distance can be negetive or positive.
							// using this distance if any of players position is less than zero then bring him to start line.
							// If more than finish_distance then make him win.
							// If time is after completion of game than you can ignore it as we will already have winner.
};

struct race {
	
	//	Don't change these variables.
	//	speeds are unit distance per unit time.
	int printing_delay;
	int tortoise_speed;					// speed of Turtle
	int hare_speed;						// speed of hare
	int hare_sleep_time; 				// how much time does hare sleep (in case he decides to sleep)
	int hare_turtle_distance_for_sleep; // minimum lead which hare has on turtle after which hare decides to move
										// Any lead greater than this distance and hare will ignore turtle and go to sleep
	int finish_distance;				// Distance between start and finish line
	struct Repositioning* reposition;	// pointer to array containing Randomizer's decision
	int repositioning_count;			// number of elements in array of repositioning structure
	
	//	Add your custom variables here.
	int hare_distance;                    // current position of hare
	int tortoise_distance;			// current position of turtle
	int hare_time;				// current time till hare ran
	int tortoise_time;			// current time till turtle ran
	pthread_mutex_t lock;			// mutex lock to access all shared variables
	pthread_cond_t hare_cv,tortoise_cv,report_cv,randomizer_cv;		//condition variables for all 4 threads
	char winner;				// winner of race
	int hare_turn,tortoise_turn,report_turn,randomizer_turn;			// variable denoting if thread's turn to run or not
	int hare_sleeping;			// current sleeping time left for hare
	
	
};


void* Turtle(void *race);
void* Hare(void *race);
void* Randomizer(void *race);
void* Report(void *race);

char init(struct race *race)
{
	race->tortoise_distance=0;
	race->hare_distance=0;
	
	race->hare_time=0;
	race->tortoise_time=0;
	
	race->winner=NULL;
	
	// thread run flow is : randomizer -> turtle -> hare -> reporter
	race->hare_turn=0;
	race->tortoise_turn=0;
	race->report_turn=0;
	race->randomizer_turn=1;
	
	race->hare_sleeping=0;
	
	pthread_mutex_init(&race->lock,NULL);
	
	pthread_cond_init(&race->hare_cv,NULL);
	pthread_cond_init(&race->tortoise_cv,NULL);
	pthread_cond_init(&race->report_cv,NULL);
	pthread_cond_init(&race->randomizer_cv,NULL);
	
	
	pthread_t turtle,hare,report,randomizer;
	pthread_create(&turtle,NULL,&Turtle,(void*)race);
	pthread_create(&hare,NULL,&Hare,(void*)race);
	pthread_create(&report,NULL,&Report,(void*)race);
	pthread_create(&randomizer,NULL,&Randomizer,(void*)race);
	
	pthread_join(turtle,NULL);
	pthread_join(hare,NULL);
	pthread_join(report,NULL);
	pthread_join(randomizer,NULL);
	return race->winner; 
}

void* Turtle(void *arg)
{
	struct race* race=(struct race*)arg;
	int finish_distance=race->finish_distance;
	int speed=race->tortoise_speed;
	pthread_mutex_t lock=race->lock;
	
	while(race->winner==NULL){
	
		pthread_mutex_lock(&lock);
			
		while(!race->tortoise_turn){
			pthread_cond_wait(&race->tortoise_cv,&lock);
		}
		
		race->tortoise_turn=0;
		
		// if race is finished then thread leaves
		if(race->winner!=NULL)
		{
			race->hare_turn=1;		
			pthread_cond_signal(&race->hare_cv);
			pthread_mutex_unlock(&lock);
			break;
		}
		
		race->tortoise_distance+=speed;
		race->tortoise_time++;
		
		if(race->tortoise_distance>=finish_distance)
			race->winner='T';
		
		race->hare_turn=1;	
		pthread_cond_signal(&race->hare_cv);
		pthread_mutex_unlock(&lock);
	
	}
	
	return NULL;
  
}

void* Hare(void *arg)
{
	struct race* race=(struct race*)arg;
	int finish_distance=race->finish_distance;
	int speed=race->hare_speed;
	int sleep_dist=race->hare_turtle_distance_for_sleep;
	pthread_mutex_t lock=race->lock;
	
	while(race->winner==NULL){
		
		pthread_mutex_lock(&lock);
		
		while(!race->hare_turn){
			pthread_cond_wait(&race->hare_cv,&lock);
		}
		race->hare_turn=0;
		
		// if race finished then hare stops
		if(race->winner!=NULL)
		{
			race->report_turn=1;		
			pthread_cond_signal(&race->report_cv);
			pthread_mutex_unlock(&lock);
			break;
		}
		
		race->hare_time++;
		
		// after hare wakes up it checks for distance with turtle , if more then it sleep again
		if(race->hare_sleeping==0 && (race->hare_distance - race->tortoise_distance + race->tortoise_speed )>sleep_dist)
		{
			race->hare_sleeping=race->hare_sleep_time;
			race->report_turn=1;
			pthread_cond_signal(&race->report_cv);
			pthread_mutex_unlock(&lock);
			continue;
		}
		
		if(race->hare_sleeping==0)
			race->hare_distance+=speed;
		else{
			race->hare_sleeping--; // decrease the sleeping time of hare by 1 and continue execution of report
			race->report_turn=1;
			pthread_cond_signal(&race->report_cv);
			pthread_mutex_unlock(&lock);
			continue;
		}
		
		if(race->hare_distance>=finish_distance){
			race->winner='H';
		}
	
		race->hare_sleeping=race->hare_sleep_time; // as hare run for 1 time unit , it goes for sleep
			
		race->report_turn=1;
		pthread_cond_signal(&race->report_cv);
		pthread_mutex_unlock(&lock);

	}
	
	return NULL;
}


void* Randomizer(void *arg)
{
	struct race* race=(struct race*)arg;
	struct Repositioning *reposition=race->reposition;
	int no_reposition=race->repositioning_count;
	int final_distance=race->finish_distance;
	
	int index_reposition=0;// current index of repositoning
	
	while(race->winner==NULL)
	{
		pthread_mutex_lock(&race->lock);
		
		while(!race->randomizer_turn)
			pthread_cond_wait(&race->randomizer_cv,&race->lock);
			
		race->randomizer_turn=0;
		
		// if race already finished , then leave
		if(race->winner!=NULL)
		{
			race->tortoise_turn=1;
			pthread_cond_signal(&race->tortoise_cv);
			pthread_mutex_unlock(&race->lock);
			break;
		
		}
		
		if(index_reposition < no_reposition)
		{
			if(reposition->time == race->tortoise_time)
			{
				char player=reposition->player;
				int distance=reposition->distance;
				if(player=='T')
				{
					distance=race->tortoise_distance + distance;
					//if distance after repositioning is negative then set it to 0
					if(distance<0)
						race->tortoise_distance=0;
					else if(distance  >= final_distance) // if repositioning result in turtle won
					{
						race->tortoise_distance = final_distance;
						race->winner='T';
					}
					else
						race->tortoise_distance=distance;
				}
				else
				{
					distance=race->hare_distance + distance;
					//if distance after repositioning is negative then set it to 0
					if(distance<0)
						race->hare_distance=0;
					else if(distance  >= final_distance) //if repositioning result in hare won
					{
						race->hare_distance = final_distance;
						race->winner='H';
					}
					else
						race->hare_distance=distance;
				}
				reposition++;
				index_reposition++;
			}
					
		}
		
		race->tortoise_turn=1;
		pthread_cond_signal(&race->tortoise_cv);
		pthread_mutex_unlock(&race->lock);
		
	
	}
	return NULL;
}

void* Report(void *arg)
{
	struct race* race=(struct race*)arg;
	
	int delay=race->printing_delay;
	
	while(race->winner=='\0')
	{
		pthread_mutex_lock(&race->lock);
		
		while(!race->report_turn)
		{
			pthread_cond_wait(&race->report_cv,&race->lock);
		}
		
		race->report_turn=0;	
		
		if(race->winner!=0)
		{
			race->randomizer_turn=1;
			pthread_cond_signal(&race->randomizer_cv);
			pthread_mutex_unlock(&race->lock);
			break;
		}
		
		if(race->hare_time==delay){
			printf("---------------------------time is %d---------------------------------\n",race->hare_time);
			printf("Turtoise distance is %d\n",race->tortoise_distance);
			printf("Hare distance is %d\n",race->hare_distance);
			delay+=race->printing_delay;
		}
		
		race->randomizer_turn=1;
		pthread_cond_signal(&race->randomizer_cv);
		pthread_mutex_unlock(&race->lock);
		
	}	
	
	return NULL;
}

