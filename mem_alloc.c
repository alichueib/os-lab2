#include "mem_alloc.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>


#include "mem_alloc_types.h"
#include "my_mmap.h"

#define HEADER_SIZE sizeof(memblk_free_t)

/* pointer to the beginning of the memory region to manage */
void *heap_begin; 

/* Pointer to the first free block in the heap */
memblk_free_t *first_free; 

//size_t MEM_POOL_SIZE = 1024; //In bytes? //No idea if this is even a global variable or not!

//I am not sure if they should be as global variables:
//Size of headers:
//2.allocated_memblk_header:


#define ULONG(x)((long unsigned int)(x))
size_t size;
struct memblk_free *next; 

#if defined(FIRST_FIT)

// Functions to be implemented:

//1. memory_init

//2. memory_alloc

//3. memory_free

//4. memory_get_allocated_block_size

#elif defined(BEST_FIT)

/* TODO: code specific to the BEST FIT allocation policy can be
 * inserted here */

#elif defined(NEXT_FIT)

/* TODO: code specific to the NEXT FIT allocation policy can be
 * inserted here */

#endif


void run_at_exit(void)
{
    fprintf(stderr,"YEAH B-)\n");
    
    /* TODO: insert your code here */
}




void memory_init(void)
{
    /* register the function that will be called when the programs exits */
    atexit(run_at_exit);

    /* TODO: insert your code here */

    /* TODO: start by using the provided my_mmap function to allocate
     * the memory region you are going to manage */

    //Things to be done:
    //1. reserve heap and let the heap_begin to store the beginning of it:
    //2. initialize the free list
    // Result: Now we should have 2 global variales ready to be used:
    // first: heap_begin, second: first_free_elt_in_list

    // size_t MEM_POOL_SIZE = 1024; //In bytes? 

    heap_begin = my_mmap(MEM_POOL_SIZE); //to be fixed
    //pointer to first free block:
    first_free = (memblk_free_t *)heap_begin; //casting it such that now we deal with it as free block
    first_free->size = MEM_POOL_SIZE - sizeof(memblk_free_t);
    first_free->next = NULL;

}

void *memory_alloc(size_t size)
{
    // size_t payload_size= size + sizeof(memblk_free);

    /* TODO: insert your code here */

    /* TODO : don't forget to call the function print_alloc_info()
     * appropriately */

    //Note: header_size = sizeof(size_t) + sizeof(memblk_free) //I don't know if it the same as memblk_free *
    
    //Plan:
    //1. start from the free list (allocate first fit):
    memblk_free_t *cur= first_free;
    memblk_free_t *prev_free = first_free;

    //I also need another ptr that will be used for reassigning next ptr while removing an elt from the list!
    
    //Here I should Iterate through all the list untill going out the space: 
    while(cur != NULL){ //I want to iterate all the block of heap I have!
        //testing if size if egligible:
        if(cur->size > size + sizeof(memblk_allocated_t)){ //added sizeof(..) since I want to also make sure that not only the payload have space, but also
            size_t remaining_size = cur->size - size -sizeof(memblk_allocated_t); // what have been allocated is a space for the block payload + payload's header, so when I want to compute remaining space then it I should subtract both (payload size + header's size)
            
            // the allocated_header have space as well
            //If I am pointing to the first and allocated the first, then I should update first:

            //create new ptr:
            memblk_free_t *new_free = (memblk_free_t*)((char *)cur + sizeof(memblk_allocated_t) + size);            
            //modify its properties:
            new_free->size = remaining_size;
            new_free->next = cur->next;

            //modify pointers:
            if(first_free == cur){
                first_free = new_free;
            }else{
                //link previous to new 
                prev_free->next = new_free; 
            }

            //1.return address of this block 
            
            //NOTE SURE OF THIS STATEMENT: Pay attention that you are returning an: memblk_allocated (this will be the data structure of the allocated block!)
            
            //2.Split and remove from free block (then add the splitted part to the list!) (and so you should now reassign the ptrs)
            //I need prev pointer to use when I split
            
            //So: Split, then reassign pointers, then return address of the payload for the user:

            //split: remaining side testing (if remaining can fit a header or not):

            //I should have pointer pointing to the block before cur!

            //modifications to be done as know the payload should also allocate a space for the allocated header block:
            
            //size_t remaining_size = cur->size - size -sizeof(memblk_allocated_t); // what have been allocated is a space for the block payload + payload's header, so when I want to compute remaining space then it I should subtract both (payload size + header's size)
            if(remaining_size >= sizeof(memblk_free_t)){
                //alocate a block in heap (with size and header): (TO  BE REVIEWED, AS I HAD TO EDIT DESGIN!)
                memblk_allocated_t *allocated_block = (memblk_allocated_t *)cur;
                allocated_block->size = size; //this is holding the size of payload only

                //split
                //Split == now I should be pointing to a new address which is equal to &cur + size of cur (now I am pointing to first byte in the remaining)
                memblk_free_t *new_free = (memblk_free_t*)((char *)cur + sizeof(memblk_allocated_t) + size);

                //reassign free list pointers:
                prev_free->next = new_free;


                // calling: print_alloc_info function:
                void *returned_ptr = (void *)((char *)cur + sizeof(memblk_allocated_t));

                print_alloc_info(returned_ptr,size); // Result is: stderr, "ALLOC at : %lu (%d byte(s))\n", the allocated address is same as cur! (probably just the payload address)
                
                //return address of payload:
                return returned_ptr; // if we are returning  the address of payload I should also take into consideration the size of the header
                //this payload address will have a allocated_block_header before // so to free it I should shift this size inorder to get the beginning of the block!

            }else{
                //allocate block:
                memblk_allocated_t *allocated_block = (memblk_allocated_t *)cur;
                allocated_block->size = size;

                //No split
                //As: don't bother splitting as this wouldn't be helpfull for allocation: So just keep it as internal fragementation:
                //reassign free list pointers:
                prev_free->next = cur->next; 

                //return address of payload:                
                void *returned_ptr = (void *)((char *)cur + sizeof(memblk_allocated_t));

                //calling print_alloc_info fct: 
                print_alloc_info(returned_ptr,size);

                return returned_ptr;
            }

        }else if(cur->size == size + sizeof(memblk_allocated_t) ){
            //allocate block:
            memblk_allocated_t *allocated_block = (memblk_allocated_t *)cur;
            allocated_block->size = size;

            //NO SPLITTING NEEDED 
            if(first_free == cur){
                first_free = cur->next;
            }
            //I should return an address of the first byte for the user so I type cast to (char*)
            //then I Will type cast to void *
            void *returned_ptr = (void *)((char *)cur + sizeof(memblk_allocated_t));
            
            //running  p_a_i fct():
            print_alloc_info(returned_ptr,size);

            return returned_ptr;
        }
        else{
            prev_free = cur; //Here what I am doing is keeping a pointer to the block before the current pointer , such that will be used later on when splitting (and thus reassign next pointer)
            cur = cur->next; //going to the next free element
        }
    }

    
    return NULL;
}

void memory_free(void *p)
{

    /* TODO: insert your code here */

    /* TODO : don't forget to call the function print_free_info()
     * appropriately */

}

size_t memory_get_allocated_block_size(void *addr)
{

    /* TODO: insert your code here */

    return 0;
}


void print_mem_state(void)
{
    /* TODO: insert your code here */
    // How to get the allocated/non-allocated blocks in a heap?
    
    //Now I should use the allocated structure:
    //Consider a 2 continguous blocks in a free list where: first block at position X, second block at position Y.
    //I Will compute the space from X to Y and then cast through this space to print its structure:

    //I  should Iterate through all the heap address (and keep a pointer in each free block, such I can print all the allocated blocks between 2 free blocks!)
    //Iterate through the whole heap: the pointer should iterate through a allocated block (but as I am making arithmetic I should apply some type casting first)
    
}


void print_info(void) {
    fprintf(stderr, "Memory : [%lu %lu] (%lu bytes)\n", (long unsigned int) heap_begin, (long unsigned int) ((char*)heap_begin+MEM_POOL_SIZE), (long unsigned int) (MEM_POOL_SIZE));
}

void print_free_info(void *addr){
    if(addr){
        fprintf(stderr, "FREE  at : %lu \n", ULONG((char*)addr - (char*)heap_begin));
    }
    else{
        fprintf(stderr, "FREE  at : %lu \n", ULONG(0));
    }    
}

void print_alloc_info(void *addr, int requested_size){
  if(addr){
    fprintf(stderr, "ALLOC at : %lu (%d byte(s))\n", 
	    ULONG((char*)addr - (char*)heap_begin), requested_size);
  }
  else{
    fprintf(stderr, "Warning, system is out of memory\n"); 
  }
}

void print_alloc_error(int requested_size) 
{
    fprintf(stderr, "ALLOC error : can't allocate %d bytes\n", requested_size);
}


#ifdef MAIN
int main(int argc, char **argv){

  /* The main can be changed, it is *not* involved in tests */
  memory_init();
  print_info();
  int i ; 
  for( i = 0; i < 10; i++){
    char *b = memory_alloc(rand()%8);
    memory_free(b);
  }

  char * a = memory_alloc(15);
  memory_free(a);


  a = memory_alloc(10);
  memory_free(a);

  fprintf(stderr,"%lu\n",(long unsigned int) (memory_alloc(9)));
  return EXIT_SUCCESS;
}
#endif 
