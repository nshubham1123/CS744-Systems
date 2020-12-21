#include "common.h"
#include "persistent storage.h"


struct cache_entry{
	int valid;
	int time;
	int frequency;
	char* key;
	char* value;
} cache_entry;

pthread_rwlock_t* locks=NULL;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

struct cache_entry *cache=NULL;
int CAPACITY=0;
int CURRENT_NO=0;
int timer=0;

void initialize_cache(int capacity)
{
	cache =(struct cache_entry*)malloc(sizeof(struct cache_entry)*capacity); 
	locks =(pthread_rwlock_t*)malloc(sizeof(pthread_rwlock_t)*capacity);
	CAPACITY = capacity;
	
	for(int i=0;i<capacity;i++)
	{
		cache[i].valid=0;
		cache[i].time=-1;
		cache[i].frequency=0;
		cache[i].key = NULL;
		cache[i].value = NULL;
		int s = pthread_rwlock_init(&locks[i],NULL);
	}
	
}

//
int find_lru_block()
{
	int cur_time = timer;
	int cur_index = -1;
	
	// lock full cache
	for(int i=0;i<CAPACITY;i++)
	{
		pthread_rwlock_wrlock(&locks[i]);
		if(cache[i].time < cur_time)
		{
			
			cur_time = cache[i].time;
			cur_index = i;
			
		}
		
	}
	for(int i=0;i<CAPACITY;i++)
	{
		pthread_rwlock_unlock(&locks[i]);
	}
	return cur_index;

}

void insert_to_cache_block(char* key,char* value, int block)
{
			timer++;
			cache[block].valid=1;
			cache[block].time=timer;
			cache[block].key=key;
			cache[block].value = value;
			printf("inserted key is %s with value %s in cache block %d\n",key,value,block);
		
			return;
}

void insert_to_lru_block(char* key,char* value)
{
		// mutex lock
		
		pthread_mutex_lock(&mutex);
		
		int lru_block_no = find_lru_block();
		// write lock
		pthread_rwlock_wrlock(&locks[lru_block_no]);
		//evicitng , saving to persistet store
		insert_to_cache_block(key,value,lru_block_no);
		//int i=lru_block_no;
		//add(cache[i].key,cache[i].value);
		// write unlock
		pthread_rwlock_unlock(&locks[lru_block_no]);
		// mutex unlock
		pthread_mutex_unlock(&mutex);
		return;
}

int insert_to_cache(char *key , char *value)
{
	int s;
	// checking if current key already exists in cache
	for(int i=0;i<CAPACITY;i++)
	{
		// read lock cache[i] here
		pthread_rwlock_wrlock(&locks[i]);
		// if entry is valid and key exists , change its value and update in KV Store
		if(cache[i].valid==1 && strcmp(cache[i].key,key)==0)
		{
			timer++;
			cache[i].time=timer;
			//updating key
			cache[i].value=value;
			//printf("key already exist in cache so its update with key %s and value %s\n",key,value);
			
			// update in persistent system
			add(cache[i].key,cache[i].value);
			
			// write unlock
			s = pthread_rwlock_unlock(&locks[i]);
			
			return 1;

		}
		// read unlock cache[i] here
		s=pthread_rwlock_unlock(&locks[i]);
	}
	
	// looping to find a free cache block i.e. block with valid bit 0
	for(int i=0;i<CAPACITY;i++)
	{
		// write lock
		s=pthread_rwlock_wrlock(&locks[i]);
		// free block found , so insert key value in it and add key value to KV store
		if(cache[i].valid==0)
		{
			
			insert_to_cache_block(key,value,i);
			//add in persistent system
			add(cache[i].key,cache[i].value);
			//write unlock
			pthread_rwlock_unlock(&locks[i]);
			return 1;
		}
		//write unlock
		pthread_rwlock_unlock(&locks[i]);
	
	}
	 // if no free block , find the LRU block and do LRU Replcement 
	insert_to_lru_block(key,value);
	add(key,value);	
	return 1;
}

// return value for key iif found else return key not found 
char* find_value(char* key)
{
	//printf("inside find_value in cache\n");
	// searching if key present in cache 
	for(int i=0;i<CAPACITY;i++)
	{
		// write lock
		pthread_rwlock_wrlock(&locks[i]);
		// if key present in cache then return its value 
		if(cache[i].valid==1 && strcmp(cache[i].key,key)==0)
		{
			
			timer++;
			cache[i].time = timer;
			
			//write unlock
			pthread_rwlock_unlock(&locks[i]);
			return cache[i].value;
		}
		//read unlock
		pthread_rwlock_unlock(&locks[i]);
	
	}
	//printf("going to persistent store find store\n");
	char* value = find_store(key);
	
	/******
	char* value = get value corresponding to given key from persistent store
	if key not found in persistent store , return key not found
	if key found , 
		1 . if free block found , add key value here and return value
		2 . if free block not found , do lru replacement.
		
	******/
	
	if(strcmp(value,"key not found")==0)
	{
		return value;
	}
	
	// convertig value to proper format
	for(int i=0;i<257;i++)
	{
		if(value[i]==' ')
		{
			value[i]='\0';
			break;
		}
	}
	
	// searching for free block to sotre key value fetched from KV Store
	for(int i=0;i<CAPACITY;i++)
	{
		// write lock
		pthread_rwlock_wrlock(&locks[i]);
		// free block found , so insert key value in it 
		if(cache[i].valid==0)
		{
			
			insert_to_cache_block(key,value,i);
			//write unlock
			pthread_rwlock_unlock(&locks[i]);
			return value;
		}
		//write unlock
		pthread_rwlock_unlock(&locks[i]);
	}
	
	insert_to_lru_block(key,value);
	return value;
		
}

// return 1 if key found and deleted else return 0

int delete_from_cache(char* key)
{
	// if key or file not found in KV Store , then return.
	// if key found then it gets deleted from KV Store and then delete_from_store() returns true.
	if(!delete_from_store(key)){
		//printf("key or file not found\n");
		return 0;
	}

	// if key is present in cache then mark that block invalid i,e, valid bit = 0
	for(int i=0;i<CAPACITY;i++)
	{
		//write lock
		pthread_rwlock_wrlock(&locks[i]);
		// if key found in cache , valid bit = 0
		if(strcmp(cache[i].key , key)==0){
		
			cache[i].valid=0;
			// write unlock
			pthread_rwlock_unlock(&locks[i]);
			return 1; // key_found
		}
		// write unlock
		pthread_rwlock_unlock(&locks[i]);
	}	

	return 1; // 1 since key found
}

void print_cache()
{
	pthread_mutex_lock(&mutex);
	printf("***************final contents of cache********************\n");
	for(int i=0;i<CAPACITY;i++)
	{
		pthread_rwlock_rdlock(&locks[i]);
		printf("block %d valid %d - %s : %s\n",i,cache[i].valid,cache[i].key,cache[i].value);
	}
	for(int i=0;i<CAPACITY;i++)
	{
		pthread_rwlock_unlock(&locks[i]);
	}
	
	printf("**********************************************************\n");
	pthread_mutex_unlock(&mutex);	
}

void destroy_cache()
{
	for(int i=0;i<CAPACITY;i++)
		pthread_rwlock_destroy(&locks[i]);
	free(locks);
	free(cache);
}


//*********************************************** LFU ******************************************************


int find_lfu_block()
{
	int cur_frequency = INT_MAX;
	int cur_index = -1;
	
	// lock full cache
	for(int i=0;i<CAPACITY;i++)
	{
		pthread_rwlock_wrlock(&locks[i]);
		if(cache[i].frequency <= cur_frequency)
		{
			
			cur_frequency = cache[i].frequency;
			cur_index = i;
			
		}
		
	}
	for(int i=0;i<CAPACITY;i++)
	{
		pthread_rwlock_unlock(&locks[i]);
	}
	return cur_index;

}

void insert_to_cache_block_lfu(char* key,char* value, int block)
{
			timer++;
			cache[block].valid=1;
			cache[block].frequency=1;
			cache[block].key=key;
			cache[block].value = value;
			printf("inserted key is %s with value %s in cache block %d\n",key,value,block);
		
			return;
}

void insert_to_lfu_block(char* key,char* value)
{
		// mutex lock
		
		pthread_mutex_lock(&mutex);
		
		int lru_block_no = find_lfu_block();
		// write lock
		pthread_rwlock_wrlock(&locks[lru_block_no]);
		//evicitng , saving to persistet store
		insert_to_cache_block(key,value,lru_block_no);
		//int i=lru_block_no;
		//add(cache[i].key,cache[i].value);
		// write unlock
		pthread_rwlock_unlock(&locks[lru_block_no]);
		// mutex unlock
		pthread_mutex_unlock(&mutex);
		return;
}

int insert_to_cache_lfu(char *key , char *value)
{
	int s;
	// checking if current key already exists in cache
	for(int i=0;i<CAPACITY;i++)
	{
		// read lock cache[i] here
		pthread_rwlock_wrlock(&locks[i]);
		// if entry is valid and key exists , change its value and update in KV Store
		if(cache[i].valid==1 && strcmp(cache[i].key,key)==0)
		{
			//timer++;
			cache[i].frequency+=1;
			//updating key
			cache[i].value=value;
			//printf("key already exist in cache so its update with key %s and value %s\n",key,value);
			
			// update in persistent system
			add(cache[i].key,cache[i].value);
			
			// write unlock
			s = pthread_rwlock_unlock(&locks[i]);
			
			return 1;

		}
		// read unlock cache[i] here
		s=pthread_rwlock_unlock(&locks[i]);
	}
	
	// looping to find a free cache block i.e. block with valid bit 0
	for(int i=0;i<CAPACITY;i++)
	{
		// write lock
		s=pthread_rwlock_wrlock(&locks[i]);
		// free block found , so insert key value in it and add key value to KV store
		if(cache[i].valid==0)
		{
			
			insert_to_cache_block(key,value,i);
			//add in persistent system
			add(cache[i].key,cache[i].value);
			//write unlock
			pthread_rwlock_unlock(&locks[i]);
			return 1;
		}
		//write unlock
		pthread_rwlock_unlock(&locks[i]);
	
	}
	 // if no free block , find the LRU block and do LRU Replcement 
	insert_to_lfu_block(key,value);
	add(key,value);	
	return 1;
}

// return value for key iif found else return key not found 
char* find_value_lfu(char* key)
{
	//printf("inside find_value in cache\n");
	// searching if key present in cache 
	for(int i=0;i<CAPACITY;i++)
	{
		// write lock
		pthread_rwlock_wrlock(&locks[i]);
		// if key present in cache then return its value 
		if(cache[i].valid==1 && strcmp(cache[i].key,key)==0)
		{
			
			//timer++;
			cache[i].frequency=1;
			
			//write unlock
			pthread_rwlock_unlock(&locks[i]);
			return cache[i].value;
		}
		//read unlock
		pthread_rwlock_unlock(&locks[i]);
	
	}
	//printf("going to persistent store find store\n");
	char* value = find_store(key);
	
	/******
	char* value = get value corresponding to given key from persistent store
	if key not found in persistent store , return key not found
	if key found , 
		1 . if free block found , add key value here and return value
		2 . if free block not found , do lru replacement.
		
	******/
	
	if(strcmp(value,"key not found")==0)
	{
		return value;
	}
	
	// convertig value to proper format
	for(int i=0;i<257;i++)
	{
		if(value[i]==' ')
		{
			value[i]='\0';
			break;
		}
	}
	
	// searching for free block to sotre key value fetched from KV Store
	for(int i=0;i<CAPACITY;i++)
	{
		// write lock
		pthread_rwlock_wrlock(&locks[i]);
		// free block found , so insert key value in it 
		if(cache[i].valid==0)
		{
			
			insert_to_cache_block(key,value,i);
			//write unlock
			pthread_rwlock_unlock(&locks[i]);
			return value;
		}
		//write unlock
		pthread_rwlock_unlock(&locks[i]);
	}
	
	insert_to_lfu_block(key,value);
	return value;
		
}

// return 1 if key found and deleted else return 0

int delete_from_cache_lfu(char* key)
{
	// if key or file not found in KV Store , then return.
	// if key found then it gets deleted from KV Store and then delete_from_store() returns true.
	if(!delete_from_store(key)){
		//printf("key or file not found\n");
		return 0;
	}

	// if key is present in cache then mark that block invalid i,e, valid bit = 0
	for(int i=0;i<CAPACITY;i++)
	{
		//write lock
		pthread_rwlock_wrlock(&locks[i]);
		// if key found in cache , valid bit = 0
		if(strcmp(cache[i].key , key)==0){
		
			cache[i].valid=0;
			// write unlock
			pthread_rwlock_unlock(&locks[i]);
			return 1; // key_found
		}
		// write unlock
		pthread_rwlock_unlock(&locks[i]);
	}	

	return 1; // 1 since key found
}


