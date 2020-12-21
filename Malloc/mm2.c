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


typedef struct tree_node{
	//char *ptr;
	int size;
	int height;
	struct tree_node *left,*right;
} tree_node;

void *init_mem_sbrk_break = NULL;
void coaelesce(tree_node *);

tree_node *allocated,*free_list;


int height(tree_node *N)  
{  
    if (N == NULL)  
        return 0;  
   
    return N->height;  
}  
  
// A utility function to get maximum 
// of two integers  
int max(int a, int b)  
{  
    return (a > b)? a : b;  
}  
  
// A utility function to right 
// rotate subtree rooted with y  
// See the diagram given above.  
tree_node *rightRotate(tree_node *y)  
{  
    tree_node *x = y->left;  
    tree_node *T2 = x->right;  
  
    // Perform rotation  
    x->right = y;  
    y->left = T2;  
  
    // Update heights  
    y->height = max(height(y->left), 
                    height(y->right)) + 1;  
    x->height = max(height(x->left), 
                    height(x->right)) + 1;  
  
    // Return new root  
    return x;  
}  
  
// A utility function to left  
// rotate subtree rooted with x    
tree_node *leftRotate(tree_node *x)  
{  
	//printf("\ninside left rotation\n");
    tree_node *y = x->right;  
    tree_node *T2 = y->left;  
  
    // Perform rotation  
    y->left = x;  
    x->right = T2;  
  
    // Update heights  
    x->height = max(height(x->left),     
                    height(x->right)) + 1;  
    y->height = max(height(y->left),  
                    height(y->right)) + 1;  
  
    // Return new root  
    return y;  
}  
  
// Get Balance factor of node N  
int getBalance(tree_node *N)  
{  
    if (N == NULL)  
        return 0;  
    return height(N->left) - height(N->right);  
}  

tree_node* insert(tree_node *root,tree_node* node)
{
	//printf("\ngoing for insertion\n");
	
	if (root == NULL)  
        	return(node);
        	
	if (node < root)  
        	root->left = insert(root->left,node);  
    	else 
        	root->right = insert(root->right, node); 
        	
	root->height = 1 + max(height(root->left),  
                        height(root->right));  
  
    
    	int balance = getBalance(root);  
  	//printf("\nbalance is %d\n",balance);
    // If this node becomes unbalanced, then  
    // there are 4 cases  
  
    // Left Left Case  
    tree_node *left_node=root->left,*right_node=root->right;
    if (balance > 1 && node < left_node)  
        return rightRotate(root);  
  
    // Right Right Case  
    if (balance < -1 && node > right_node)  
        return leftRotate(root);  
  
    // Left Right Case  
    if (balance > 1 && node > left_node)  
    {  
        root->left = leftRotate(root->left);  
        return rightRotate(root);  
    }  
  
    // Right Left Case  
    if (balance < -1 && node < right_node)  
    {  
        root->right = rightRotate(root->right);  
        return leftRotate(root);  
    }  
  
    /* return the (unchanged) node pointer */
 //   printf("\nnow returning form insert\n");
    return root; 


}

void insert_allocated(tree_node *node)
{
	
	allocated=insert(allocated,node);
	
}

void insert_free(tree_node *node)
{
	//printf("\ngoing for free block insertion\n");
	free_list=insert(free_list,node);
	
}


tree_node * minValueNode( tree_node* node) 
{ 
     tree_node* current = node; 
  
    /* loop down to find the leftmost leaf */
    while (current->left != NULL) 
        current = current->left; 
  
    return current; 
} 

tree_node* find_parent_node(tree_node *root,tree_node* node)
{
	if(root==node)
		return root;
	if(node < root)
	{
		if(node==root->left)
			return root;
		return find_parent_node(root->left,node);
	}
	if(node==root->right)
		return root;
	return find_parent_node(root->right,node);

}

tree_node* find_allocated(tree_node *root,void* ptr)
{
	if((void*)root+16==ptr)
		return root;
	if(ptr < (void*)root+16)
	{
		//if(node==root->left)
		//	return root;
		return find_allocated(root->left,ptr);
	}
	
	//if(node==root->right)
		//return root;
	return find_allocated(root->right,ptr);

}

tree_node* find_parent(tree_node *node,int is_allocated)
{
	if(is_allocated)
		return find_parent_node(allocated,node);
	else
		return find_parent_node(free_list,node);

}
  
// Recursive function to delete a node with given key 
// from subtree with given root. It returns root of 
// the modified subtree. 
tree_node* deleteNode( tree_node* root, tree_node *node,int is_allocated) 
{ 
    // STEP 1: PERFORM STANDARD BST DELETE 
  
    if (root == NULL) 
        return root; 
  
    // If the key to be deleted is smaller than the 
    // root's key, then it lies in left subtree 
    if ( node < root ) 
        root->left = deleteNode(root->left, node,is_allocated); 
  
    // If the key to be deleted is greater than the 
    // root's key, then it lies in right subtree 
    else if( node > root ) 
        root->right = deleteNode(root->right, node,is_allocated); 
  
    // if key is same as root's key, then This is 
    // the node to be deleted 
    else
    { 
        // node with only one child or no child 
        if( (root->left == NULL) || (root->right == NULL) ) 
        { 
            tree_node *child = root->left ? root->left : 
                                             root->right; 
  		tree_node *par_node=find_parent(root,is_allocated);
           
            if(par_node == root)
            {
            	if(is_allocated)
            		allocated=child;
            	else
            		free_list=child;
            }
            else{
                if(par_node->left==root)
                	par_node->left=child;
                if(par_node->right==root)
                	par_node->right=child;
               }
               root=child;	
           
        } 
        else
        { 
            // node with two children: Get the inorder 
            // successor (smallest in the right subtree) 
             tree_node* replacement = minValueNode(root->right);
            
          //  tree_node *par_replacement=find_parent(replacement,is_allocated);
  		/*if(par_replacement->left==replacement)
  			par_replacement->left=replacement->right;
  		else
  			par_replacement->right=replacement->right;*/
  		 root->right = deleteNode(root->right, replacement,is_allocated);
  		
  		replacement->left=root->left;
  		replacement->right=root->right; 
  
            // Copy the inorder successor's data to this node 
             //we can't just swap contents
             tree_node *par_node=find_parent(root,is_allocated);
             
             if(par_node == root)
             {
             	if(is_allocated)
             		allocated=replacement;
             	else
             		free_list=replacement;
             }
  	     else{
  		
  			
                if(par_node->left==root)
                	par_node->left=replacement;
                if(par_node->right==root)
                	par_node->right=replacement;
                	           
  		}
  		root=replacement;
            // Delete the inorder successor 
          //  root->right = deleteNode(root->right, replacement); 
        } 
    } 
  
    // If the tree had only one node then return 
    if (root == NULL) 
      return root; 
  
    // STEP 2: UPDATE HEIGHT OF THE CURRENT NODE 
    root->height = 1 + max(height(root->left), 
                           height(root->right)); 
  
    // STEP 3: GET THE BALANCE FACTOR OF THIS NODE (to 
    // check whether this node became unbalanced) 
    int balance = getBalance(root); 
  
    // If this node becomes unbalanced, then there are 4 cases 
  
    // Left Left Case 
    if (balance > 1 && getBalance(root->left) >= 0) 
        return rightRotate(root); 
  
    // Left Right Case 
    if (balance > 1 && getBalance(root->left) < 0) 
    { 
        root->left =  leftRotate(root->left); 
        return rightRotate(root); 
    } 
  
    // Right Right Case 
    if (balance < -1 && getBalance(root->right) <= 0) 
        return leftRotate(root); 
  
    // Right Left Case 
    if (balance < -1 && getBalance(root->right) > 0) 
    { 
        root->right = rightRotate(root->right); 
        return leftRotate(root); 
    } 
  
    return root; 
} 

void delete_free(tree_node *node)
{
	free_list=deleteNode(free_list,node,0);
}


void delete_allocated(tree_node *node)
{
	allocated=deleteNode(allocated,node,1);
}

tree_node* find_best_fit(tree_node* root,int size)
{
	if(root==NULL)
		return NULL;
		
	if(root->size==size || root->size>=size+16)
		return root;
		
	tree_node* left,*right;
	
	left=find_best_fit(root->left,size);
	if(left){
	//	printf("\nleft best fir\n");
		return left;
		}
	right=find_best_fit(root->right,size);
	//printf("\nright best fit\n");
	
		return right;
}

int mm_init(void)
{
	
	//This function is called every time before each test run of the trace.
	//It should reset the entire state of your malloc or the consecutive trace runs will give wrong answer.	
	

	/* 
	 * This function should initialize and reset any data structures used to represent the starting state(empty heap)
	 * 
	 * This function will be called multiple time in the driver code "mdriver.c"
	 */
	allocated=NULL;
	free_list=NULL;
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
	
	tree_node *free_start=find_best_fit(free_list ,size);
	//free_start=NULL;
	if(free_start !=NULL)
	{	
		// if of same size , remove it form free list and add to allocated list
		//printf("\nallocating from free list\n");
		if(free_start->size==size)
		{
			//printf("\nallocating from free list\n");
			tree_node *new_node=free_start;
			delete_free(free_start);
			new_node->height=1;
			new_node->left=NULL;
			new_node->right=NULL;
			insert_allocated(new_node);
			return (void*)new_node+16;
		}
		else // change its size in free list and add new node in allocated list
		{
			//printf("\nchanging size\n");
			int ptr_increment=(free_start->size-size-16);
			void *free_block_ptr=(void*)free_start+16;
			
			tree_node *new_mem_block=(tree_node*)(free_block_ptr+ptr_increment);
			
			new_mem_block->size=size;
			new_mem_block->height=1;
			new_mem_block->left=NULL;
			new_mem_block->right=NULL;
			
			delete_free(free_start);
			
			free_start->size=ptr_increment;
			free_start->height=1;
			free_start->left=NULL;
			free_start->right=NULL;
			insert_free(free_start);
			
			insert_allocated(new_mem_block);
			
			return (void*)new_mem_block+16;
			
		}
	}
	
	void *new_mem=mem_sbrk(size+16);		//mem_sbrk() is wrapper function for the sbrk() system call. 
								//Please use mem_sbrk() instead of sbrk() otherwise the evaluation results 
								//may give wrong results
	tree_node *new_node=(tree_node*)new_mem;
	new_mem+=16;
	//new_node->ptr=(char*)new_mem;
	new_node->size=size;
	new_node->height=1;
	new_node->left=NULL;
	new_node->right=NULL;
	insert_allocated(new_node);
	
	return (void*)new_node+16;
}

tree_node *find_inorder_prev(tree_node *node)
{
	if(!node->left){
		tree_node* par= find_parent_node(free_list,node);
		if(par==node)
			return NULL;
		return par;
	}
	tree_node* temp=node->left;
	while(temp->right!=NULL)
		temp=temp->right;
	return temp;
}

tree_node *find_inorder_next(tree_node *node)
{
	if(!node->right){
		tree_node *par=find_parent_node(free_list,node);
		if(par==node)
			return NULL;
		return par;
	}
	tree_node* temp=node->right;
	while(temp->left!=NULL)
		temp=temp->left;
	return temp;
}


void coaelesce(tree_node *node)
{
	tree_node *left_node,*right_node;
	void *temp;
	
	left_node= find_inorder_prev(node);
	right_node= find_inorder_next(node);
	
	if(left_node)
	{
		//printf("\ncolesing with left node\n");
		temp=left_node;
		temp+=(16+left_node->size);
		if(temp==node)
		{
			//printf("\ncolesing with left node\n");
			delete_free(left_node);
			delete_free(node);
			left_node->size+=(16+node->size);
			left_node->left=NULL;
			left_node->right=NULL;
			left_node->height=1;
			insert_free(left_node);
			node=left_node;
		}
	}
	
	if(right_node)
	{
		//printf("\ncolesing with right node\n");
		temp=node;
		temp+=(16+node->size);
		if(temp==right_node)
		{
			//printf("\ncolesing with right node\n");
			delete_free(right_node);
			delete_free(node);
			node->size+=(16+right_node->size);
			node->left=NULL;
			node->right=NULL;
			node->height=1;
			insert_free(node);
		
		}
	}
	
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
	// printf("\nfinding allocated\n");
	  tree_node* allocated_block = find_allocated(allocated,ptr);
//	 printf("\nbefore deletion of allocated block\n");
	 tree_node* free_block=allocated_block;
	 delete_allocated(allocated_block);
//	 printf("\nhere\n");
	 free_block->height=1;
	 free_block->left=NULL;
	 free_block->right=NULL;
	 
	
	insert_free(free_block);
	//if(free_list->right)
	//	printf("\nyes free block size is %d\n",free_list->size);
	coaelesce(free_block);
//	
}

tree_node * find_left_realloc(tree_node* root,tree_node *ptr)
{
	if(root==NULL)
		return NULL;
	
	tree_node *cmpare=(void*)root+16+root->size;
	if(cmpare==ptr)
		return root;
	
	if(cmpare<ptr)
		return find_left_realloc(root->right,ptr);
	return find_left_realloc(root->left,ptr);
}

tree_node * find_right_realloc(tree_node* root,tree_node *ptr)
{
	if(root==NULL)
		return NULL;
	
	tree_node *cmpare=(void*)ptr+16+ptr->size;
	if(cmpare==root)
		return root;
	
	if(cmpare > root)
		return find_right_realloc(root->right,ptr);
		
	return find_right_realloc(root->left,ptr);
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
	tree_node *allocated_block,*prev_node,*next_node;
	
	allocated_block=find_allocated(allocated,ptr);
	
	prev_node=find_left_realloc(free_list,allocated_block);
	next_node=find_right_realloc(free_list ,allocated_block);
	
	//case 1 - sirf prev se allocate ho jaae
	//case -2 - sirf next se allocate ho jaae
	
	
    if(allocated_block->size < size){
	
	/*if(prev_node!=NULL) // && (void*)prev_node+32+prev_node->size==ptr)
	{
		//printf("\nprev realloc\n");
		int cur_size=allocated_block->size;
		int prev_size = prev_node->size;
		void* new_allocated_mem=(void*)prev_node;
		
		 if(prev_size+cur_size>=size)
		{
		//	printf("\nprev realloc\n");
			delete_free(prev_node);
			delete_allocated(allocated_block);
			prev_node->size=size;
			prev_node->height=1;
			prev_node->left=NULL;
			prev_node->right=NULL;
			memcpy(new_allocated_mem+16 , ptr,cur_size);
			
			tree_node *free_block=new_allocated_mem+16+size;
			free_block->size=prev_size+cur_size-size;
			free_block->height=1;
			free_block->left=NULL;
			free_block->right=NULL;
			insert_free(free_block);
			//coaelesce(free_block);
			insert_allocated(prev_node);
			return new_allocated_mem+16;
		}
	}*/
	
	if(next_node!=NULL )
	{
		
		int cur_size=allocated_block->size;
		int next_size = next_node->size;
		void* new_allocated_mem=(void*)allocated_block;
		
		 if(next_node->size+allocated_block->size>=size)
		{
		//	printf("\nnext realloc\n");
			delete_free(next_node);
			delete_allocated(allocated_block);
			allocated_block->size=size;
			allocated_block->height=1;
			allocated_block->left=NULL;
			allocated_block->right=NULL;
			//memcpy((void*)prev_node+24 , ptr,cur_size);
			
			tree_node *free_block=new_allocated_mem+16+size;
			free_block->size=next_size+cur_size-size;
			free_block->height=1;
			free_block->left=NULL;
			free_block->right=NULL;
			insert_free(free_block);
			insert_allocated(allocated_block);
			return new_allocated_mem+16;
		}
	}
	
	
    }
	
	
	
	//printf("\nnew malloc for realloc\n");
	void* new_mem=mm_malloc(size);
	//tree_node *allocated_block=find_allocated(allocated,ptr);
	int copy_size=allocated_block->size;
	
	if(copy_size > size)
		copy_size=size;
	
	memcpy(new_mem,ptr,copy_size);
	mm_free(ptr);
	return new_mem;	
	
}

