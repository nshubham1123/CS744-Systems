/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "We Tried",
    /* First member's full name */
    "Ashwani Kumar Jha",
    /* First member's email address */
    "ashwani@cse.iitb.ac.in",
    /* Second member's full name (leave blank if none) */
    "Shubham Nemani",
    /* Second member's email address (leave blank if none) */
    "nshubham@cse.iitb.ac.in"
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* 
 * mm_init - initialize the malloc package.
 */
typedef struct mem_block{
	//char *ptr;
	size_t size;
	struct mem_block *next,*prev;
	
} mem_block;

void *init_mem_sbrk_break = NULL;

// allocated and free are two list of memory bolcks allocated and free
struct mem_block *allocated,*free_list,*last_allocated;
void  add_block_to_free_list(struct mem_block*);

int mm_init(void)
{
	
	//This function is called every time before each test run of the trace.
	//It should reset the entire state of your malloc or the consecutive trace runs will give wrong answer.	
	

	/* 
	 * This function should initialize and reset any data structures used to represent the starting state(empty heap)
	 * 
	 * This function will be called multiple time in the driver code "mdriver.c"
	 */
	//mem_init();
	allocated=NULL;
	free_list=NULL;
	last_allocated=NULL;
	//printf("\nsize of struct is %d\n",sizeof(mem_block));
	
    return 0;		//Returns 0 on successfull initialization.
}

//---------------------------------------------------------------------------------------------------------------
/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{	
	/* 
	 * This function should keep track of the allocated memory blocks.
	 * The block allocation should minimize the number of holes (chucks of unusable memory) in the heap memory.
	 * The previously freed memory blocks should be reused.
	 * If no appropriate free block is available then the increase the heap  size using 'mem_sbrk(size)'.
	 * Try to keep the heap size as small as possible.
	 */

	if(size <= 0){		// Invalid request size
		return NULL;
	}
	size = ((size+7)/8)*8;		//size alligned to 8 bytes
	
	// finding best fit
	struct mem_block *free_start=free_list,*best_fit=NULL;
	while(free_start!=NULL){
		if((free_start->size==size) || (free_start->size >= size+16))
		{
			if(!best_fit)
				best_fit=free_start;
			else if(free_start->size < best_fit->size)
				best_fit=free_start;
		}
		free_start=free_start->next;
	}
	
	free_start=best_fit;

	if(free_start !=NULL)
	{	
		// if of same size , remove it form free list and add to allocated list
		if(free_start->size==size)
		{
			struct mem_block *prev_block=free_start->prev;
			if(!prev_block) // if first block of free list is best fit
				free_list=free_start->next;
			else
				prev_block->next=free_start->next;
			if(free_start->next){ // if middle block of free list is best fit
				struct mem_block *next_block=free_start->next;
				next_block->prev=prev_block;
			}
			if(!last_allocated) // if allocated list is empty
			{
				last_allocated=free_start;
				free_start->prev=NULL;
				free_start->next=NULL;
				allocated=last_allocated;	
			}
			else{
				free_start->prev=last_allocated;
				last_allocated->next = free_start;
				last_allocated=free_start;
				free_start->next=NULL;
			}
			return (void*)free_start+16;
		}
		else // change its size in free list and add new node in allocated list
		{
		/*	int cur_size = free_start->size;	
			struct mem_block *new_mem ,*free_block;
			new_mem = free_start;
			free_block = (void*)free_start + size + 16;
			
			new_mem->size=size;
			new_mem->prev=NULL;
			new_mem->next=NULL;
			
			if(free_start->prev==NULL)
				free_list=free_start->next;
				
			if(free_start->next)
					free_start->next->prev=free_start->prev;
				
			if(free_start->prev)
					free_start->prev->next=free_start->next;
		
			free_block->size=cur_size - size -16;
			free_block->prev=NULL;
			free_block->next=NULL;
			add_block_to_free_list(free_block);
		
			if(!last_allocated) // if allocated list is empty
			{
				last_allocated=new_mem;
				new_mem->prev=NULL;
				new_mem->next=NULL;
				allocated=last_allocated;	
			}
			else{
				new_mem->prev=last_allocated;
				last_allocated->next = new_mem;
				last_allocated=new_mem;
				last_allocated->next=NULL;
			}
		
			return (void*)new_mem + 16;
		
		
		*/
		
		
		
		//	printf("\nallocating from free block changing size\n");
			int ptr_increment=(free_start->size-size-16);
			void *free_block_ptr=(void*)free_start + 16;
			//printf("\nfree block pointer %p\n",free_block_ptr);
			free_block_ptr+=ptr_increment;
			//printf("\nfree block pointer %p\n",free_block_ptr);
			struct mem_block *new_mem_block=(struct mem_block*)(free_block_ptr);
			free_block_ptr+=16;
			//printf("\nfree block pointer %p\n",free_block_ptr);
			//new_mem_block->ptr=(char*)(free_block_ptr);
			new_mem_block->size=size;
			new_mem_block->next=NULL;
		
			if(!last_allocated) // if allocated list is empty
			{
				last_allocated=new_mem_block;
				new_mem_block->prev=NULL;
				new_mem_block->next=NULL;
				allocated=last_allocated;	
			}
			else{
				new_mem_block->prev=last_allocated;
				last_allocated->next = new_mem_block;
				last_allocated=new_mem_block;
				last_allocated->next=NULL;
			}
			free_start->size=ptr_increment;
		//	printf("\nfree size %d , alocated size %d\n",free_start->size , new_mem_block->size);
			return (void*)new_mem_block+16;
			
		}
	}
	
	
	void *new_mem=mem_sbrk(size+16);
//	printf("\nsbrk %p\n",new_mem);
	// adding new memory block at last of allocated linked list
	struct mem_block *new_mem_block=(struct mem_block*)new_mem;
	//printf("\nblock pointer %p\n",new_mem);
	new_mem+=16;
	//printf("\n block pointer %p\n",new_mem);
	//new_mem_block->ptr=(char*)(new_mem);
	new_mem_block->size=size;
	new_mem_block->next=NULL;
	
	new_mem_block->prev=last_allocated;
	if(last_allocated)
		last_allocated->next = new_mem_block;
	else
		allocated=new_mem_block;
	last_allocated=new_mem_block;
	
	return new_mem;		//mem_sbrk() is wrapper function for the sbrk() system call. 
								//Please use mem_sbrk() instead of sbrk() otherwise the evaluation results 
								//may give wrong results
}

void coelesce(struct mem_block * block)
{
	//merging with previous block
	struct mem_block *prev_block , *next_block;
	prev_block=block->prev;
	next_block=block->next;
	void *prev_compare;
	if(prev_block)
	{
		prev_compare=(void*)prev_block+16;
		prev_compare+=(prev_block->size+16);
	}
	
	if(block->prev !=NULL && (prev_compare == (void*)block+16))
	{
	//	printf("\nprev coalseing\n");
		prev_block->size+=(block->size+16);
		prev_block->next=block->next;
		if(block->next)
			next_block->prev=block->prev;
		//mem_block *temp=block;
		//free(temp);
		block=prev_block;
	}
	//merging with next block
	next_block=block->next;
	void *next_compare;
	if(next_block)
	{
		next_compare=(void*)block+16;
		next_compare+=(block->size+16);
	}
	
	if(block->next !=NULL && (next_compare == (void*)next_block+16))
	{
		//printf("\nnext coalseing\n");
		struct mem_block *temp=block->next;
		block->size+=(temp->size+16);
		block->next=temp->next;
		next_block=block->next;
		if(block->next)
			next_block->prev=block;
		//free(temp);
	}
	return;
}

void add_block_to_free_list(struct mem_block *free_block)
{
			struct mem_block *free_start=free_list,*prev_block;
			
			if(free_start==NULL)
			{
				free_list=free_block;
				free_block->next=NULL;
				free_block->prev=NULL;
			//	printf("\nsize of fre eblock is %d\n",free_block->size);
				return;
			}
			
			
			while(free_start->next!=NULL && free_start < free_block)
				free_start=free_start->next;
			//adding in between the list	
			if(free_start->next !=NULL || free_start > free_block)
			{
				prev_block=free_start->prev;
				
				if(prev_block==NULL)
				{
					free_block->next=free_start;
					free_block->prev=NULL;
					free_start->prev=free_block;
					free_list=free_block;
					coelesce(free_block);
					return;
				}
				
				prev_block->next=free_block;
				free_block->prev=prev_block;
				free_block->next=free_start;
				free_start->prev=free_block;
			}
			else
			{	// adding at last of list
				free_block->next=NULL;
				free_block->prev=free_start;
				free_start->next=free_block;
			}
			
			coelesce(free_block);
		//	printf("\nsize of fre eblock is %d\n",free_block->size);
			
			return;

}


void mm_free(void *ptr)
{
	/* 
	 * Searches the previously allocated node for memory block with base address ptr.
	 * 
	 * It should also perform coalesceing on both ends i.e. if the consecutive memory blocks are 
	 * free(not allocated) then they should be combined into a single block.
	 * 
	 * It should also keep track of all the free memory blocks.
	 * If the freed block is at the end of the heap then you can also decrease the heap size 
	 * using 'mem_sbrk(-size)'.
	 */
	 struct mem_block *start=allocated;
	 
	 
	 while(start!=NULL)
	 {
		if((void*)start+16==ptr)
		{
			// delete this block from allocated and add it to free list
			//printf("\nyes pointer matched , freeing memory\n");
			struct mem_block *free_block=start , *prev_block ,*next_block;
			prev_block=free_block->prev;
			next_block=free_block->next;
			//delete from allocated
			if(next_block==NULL && prev_block==NULL)
			{
				allocated=NULL;
				last_allocated=NULL;
				
			}
			else if(next_block==NULL)
			{
				prev_block->next=NULL;
				last_allocated=prev_block;
			}
			else if(prev_block==NULL)
			{
				next_block->prev=NULL;
				allocated=next_block;
			}
			else{
				prev_block->next=start->next;
				next_block->prev=start->prev;
			}
			//now add to free list
			return add_block_to_free_list(free_block);
		
		
		}
		start=start->next;
	 }
	 
}

struct mem_block* find_prev_realloc(struct mem_block* node)
{
	struct mem_block* free_start=free_list;
	if(free_start==NULL)
		return NULL;
	
	while(free_start->next!=NULL && free_start < node)
		free_start=free_start->next;
	//void* ptr=(void*)free_start+free_start->size +16;
//	printf("\nfree size %d node size %d\n",free_start->size,node->size);	
	if((void*)free_start+free_start->size +16==node)
		return free_start;
	
	return NULL;

}

struct mem_block* find_next_realloc(struct mem_block* node)
{
	struct mem_block* free_start=free_list;
	if(free_start==NULL)
		return NULL;
	
	while(free_start->next!=NULL && (void*)node+node->size + 16 < (void*)free_start)
		free_start=free_start->next;
	//printf("\nfree size %d node size %d\n",free_start->size,node->size);
	if((void*)node + node->size+16==free_start)
		return free_start;
	//printf("\nnext not found\n");
	return NULL;

}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{	
	size = ((size+7)/8)*8; //8-byte alignement	
	
	if(ptr == NULL){			//memory was not previously allocated
		return mm_malloc(size);
	}
	
	if(size == 0){				//new size is zero
		mm_free(ptr);
		return NULL;
	}

	/*
	 * This function should also copy the content of the previous memory block into the new block.
	 * You can use 'memcpy()' for this purpose.
	 * 
	 * The data structures corresponding to free memory blocks and allocated memory 
	 * blocks should also be updated.
	*/
	
	size_t copy_size=0;
	struct mem_block *start=allocated,*allocated_block;
	// to get size of block to be deleted
	//printf("\n----------checking allocated list in realloc=====\n");
	while(start!=NULL)
	{
		if((void*)start+16==ptr)
		{
			allocated_block=start;
			copy_size=start->size;
		}
	//	printf("\n%d  %d\n",&start->ptr,start->size);
		start=start->next;
	}
	
	
	void* new_mem=mm_malloc(size);
	
	//printf("\nprev and next are null\n");
	if(copy_size > size)
		copy_size=size;
	
	memcpy(new_mem,ptr,copy_size);
	mm_free(ptr);
	return new_mem;	
}














