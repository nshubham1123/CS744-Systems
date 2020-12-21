//function to update and append database
#include "common.h"

#define MAX_FILES 32
#define max_file_name_length 100
#define KeyValueLen 100

pthread_rwlock_t file_locks[32];



void add(char *, char *);
char * find_store(char *);
int  delete_from_store(char *);


int get_file_no(char *key){
//	printf("inside get file no\n");
    int size  = strlen(key);
    int sum = 0, i = 0;
    while(i< size){
        sum = sum + key[i];
        i++;
    }
 //   printf("key si %s and sum is %d\n",key,sum);
    return (sum % MAX_FILES);
}

void initialize_files()
{
	for(int i=0;i<32;i++)
	{
		pthread_rwlock_init(&file_locks[i],NULL);
		char *file_name = (char *)malloc(max_file_name_length);
    		sprintf(file_name, "%d", i);
    		strcat(file_name, ".csv");
    		FILE *fp = fopen(file_name, "w+");
    		fclose(fp);
	}
}

//simply append the key and value
void add(char* KVkey, char* KVvalue)
{
	//printf("inside add in persistent\n");
    //printf("key:%s value:%s\n", KVkey, KVvalue);
    //get file name and open the file
    int file_no = get_file_no(KVkey), flag = 0;
    char *file_name = (char *)malloc(max_file_name_length);
    sprintf(file_name, "%d", file_no);
    strcat(file_name, ".csv");
    
    // write lock on file no
    pthread_rwlock_wrlock(&file_locks[file_no]);
    
   // pthread_mutex_lock(&file_locks[file_no]);    
    FILE *fp = fopen(file_name, "r+");

    if(fp == NULL){
        perror("file doesnt exist or some other problem with file\n");
        pthread_rwlock_unlock(&file_locks[file_no]);
        return;
        
    }
 //    printf("opened file %s\n",file_name);
    //make the key value in appropriate form
    char ValueToBeInserted[517];
    memset(ValueToBeInserted,' ',517);
    ValueToBeInserted[0]='1';
    int key_length = strlen(KVkey);
    int value_length = strlen(KVvalue);
    memcpy(&ValueToBeInserted[1],KVkey,key_length);
    memcpy(&ValueToBeInserted[258],KVvalue,value_length);
   ValueToBeInserted[515]='\n';
   ValueToBeInserted[516]='\0';
   
//	printf("key len= %d , value length = %d ,value inserted length=%ld, last inserted char= %s\n",key_length,value_length,strlen(ValueToBeInserted),&ValueToBeInserted[516]);

   char line[517];
   int free_found=0,seek=0;
    while(fgets(line, 517, fp)!= NULL){
    //	printf("traversing file reading line by line\n");
    	if(line[0]=='0')
    	{
    		free_found=1;
    	}
    	if(!free_found)
    		seek+=516;
   //    printf("line %s and key %s\n",&line[1],KVkey);
	if(strncmp(&line[1],&ValueToBeInserted[1],257)==0 && line[0]=='1')
	{
		
	//	printf("overwriting\n");
		
		fseek(fp,-258,SEEK_CUR);
		
		// key found , overwriting value
		fputs(&ValueToBeInserted[258],fp);
		pthread_rwlock_unlock(&file_locks[file_no]);
		fclose(fp);
            	return;
	}

    }
    // if free block found and add there.
    if(free_found)
    {
    	fseek(fp,seek,SEEK_SET);
    }
    	//printf("current file pointer at %lu\n",ftell(fp));
    	fputs(ValueToBeInserted,fp);
	//printf("after insertion file pointer at %lu\n",ftell(fp));
    	pthread_rwlock_unlock(&file_locks[file_no]);
    	fclose(fp);
        return;
 
}

char* find_from_file(char* KVkey,char* file_name)
{
	 FILE *fp = fopen(file_name, "r+");

    // read lock on filename
    
    if(fp == NULL){
        perror("file doesnt exist or some other problem with file\n");
        free(file_name);
        //fclose(fp);
        return "key not found";
    }
    //   printf("opened file %s\n",file_name);
     char line[517];
 
    while(fgets(line, 517, fp)!= NULL){
  //  	printf("line by line reading from file\n");
	if(strncmp(&line[1],KVkey,257)==0 && line[0]=='1')
	{
		// key found , returing value
		char* value = (char*)malloc(257);
		memcpy(value,&line[258],257);
		fclose(fp);
            	return value;
	}

    }
    fclose(fp);
    return "key not found";    

}

//fetch the value of a given key
char *find_store(char *KVkey){
  //  printf("inside find store\n");
    //get file name and open the file
    char c[257];
    int file_no = get_file_no(KVkey);
    char *file_name = (char *)malloc(max_file_name_length);
    sprintf(file_name, "%d", file_no);
    strcat(file_name, ".csv");
    pthread_rwlock_rdlock(&file_locks[file_no]);
    
     char ValueToBeInserted[257];
    memset(ValueToBeInserted,' ',257);
  
    int key_length = strlen(KVkey);
    
    memcpy(ValueToBeInserted,KVkey,key_length);
   	
   char* value = find_from_file(ValueToBeInserted,file_name);
   pthread_rwlock_unlock(&file_locks[file_no]);
   return value;
}


//delte the value from store
int  delete_from_store(char *KVkey){

    //get file name and open the file
    int file_no = get_file_no(KVkey);
    char *file_name = (char *)malloc(max_file_name_length);
    sprintf(file_name, "%d", file_no);
    strcat(file_name, ".csv");
    // write mutex lock
    	pthread_rwlock_wrlock(&file_locks[file_no]);
    
    FILE *fp = fopen(file_name, "r+");
    

    if(fp == NULL){
        perror("file doesnt exist, nothing toz delte\n");
         // write mutex unlock
       
    		pthread_rwlock_unlock(&file_locks[file_no]);
        free(file_name);
	fclose(fp);
    	
        return 0;
    }
    
     char KeySearched[257];
    memset(KeySearched,' ',257);
    
    int key_length = strlen(KVkey);
    
    memcpy(KeySearched,KVkey,key_length);
	 char line[517];
  
    while(fgets(line, 517, fp)!= NULL){
  //  	printf("reading file line by line\n");
	if(strncmp(&line[1],KeySearched,257)==0 && line[0]=='1')
	{
		
	//	printf("deleting\n");
	//	printf("curetn fp at location %lu\n",ftell(fp));
		fseek(fp,-516,SEEK_CUR);
		// key found , overwriting value
	//	printf("adding 0 at location %lu\n",ftell(fp));
		fputs("0",fp);
		pthread_rwlock_unlock(&file_locks[file_no]);
		fclose(fp);
            	return 1;
	}

    }

    
    pthread_rwlock_unlock(&file_locks[file_no]);
    fclose(fp);
    return 0;
}
