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

#define FREE_HEADER_SIZE sizeof(memblk_free_t)
#define ALLOCATED_HEADER_SIZE sizeof(memblk_allocated_t)
#define MAX_HEADER_SIZE (FREE_HEADER_SIZE > ALLOCATED_HEADER_SIZE) ? FREE_HEADER_SIZE : ALLOCATED_HEADER_SIZE
#define MIN_SPLIT_SIZE (FREE_HEADER_SIZE + 0) //Here since it is allowed to have an allocated size of payload = 0 :alloc(0) is allowed!


// Helper function to align a size UP to the next multiple of MEM_ALIGNMENT
size_t align_size(size_t size) {
    if (MEM_ALIGNMENT <= 1) return size;
    return ((size + MEM_ALIGNMENT - 1) / MEM_ALIGNMENT) * MEM_ALIGNMENT;
}

//We will have to align the header size 
#define FREE_HEADER_SIZE_AL ( align_size(FREE_HEADER_SIZE ) )
#define ALLOCATED_HEADER_SIZE_AL ( align_size(ALLOCATED_HEADER_SIZE) )
#define MIN_SPLIT_SIZE_AL (FREE_HEADER_SIZE_AL + 0) 

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


// Functions to be implemented:

//1. memory_init

//2. memory_alloc

//3. memory_free

//4. memory_get_allocated_block_size


#if defined(FIRST_FIT)

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

    //Note: header_size = sizeof(size_t) + sizeof(memblk_free)
    
    //Since we will using a free memory block for a allocated memory block, then we should make sure to make the correct definition of the needed size and the available size:
    //Needed size can be figured out from the input size + size of header (as this will be an allocated block):
    // size_t needed_size = align_size(size) + ALLOCATED_HEADER_SIZE;
    //size_t aligned_size_t = align_size(size);
    //size_t needed_size = size + ALLOCATED_HEADER_SIZE_AL; //Now what should the result be is an alignment of whole of this!
    //size_t aligned_size_t = align_size(needed_size); //Now this won't just align payload, but considering starting of header!
    //size_t payload_allocated_space = aligned_size_t - ALLOCATED_HEADER_SIZE_AL;

    //but now since the header size is always aligned -> we can change the approach of aligning for sure!
    // IMPLEMENT:
    size_t aligned_payload_size = align_size(size);

    //Ensure minimum payload size = 8 bytes 
    if( aligned_payload_size < ALLOCATED_HEADER_SIZE ){ //WHY DO THIS: since we want to ensure that when want to free it, it can be represented in a free block (which has minimum 16 bytes length == header size of free)
        aligned_payload_size = ALLOCATED_HEADER_SIZE; //Since the problem will happen with alignments of 2 or 4 (starting from 8, this wont be seen as the space for free block already there and avvialable trivially!)
    }

    size_t needed_size = aligned_payload_size + ALLOCATED_HEADER_SIZE_AL;


    //we will be using only aigned header size instead of the headers we were using in the previous parts!


    //size_t needed_size = aligned_size_t + ALLOCATED_HEADER_SIZE;

    //Plan:
    //1. start from the free list (allocate first fit):
    memblk_free_t *cur= first_free;
    memblk_free_t *prev_free = NULL;
    
    
    //I also need another ptr that will be used for reassigning next ptr while removing an elt from the list!
    //Iterating through the whole list:
    while(cur !=NULL){
        //Now we will compute the avialable size at each node: eq size at node + size of free header
        size_t available_size = cur->size + FREE_HEADER_SIZE_AL;

        if(available_size >= needed_size){ // size is enough -> allocate the block in the free block! (but note: that you need to update the free list after allocating a )
            //compute remaining part:
            // size_t remaining_size = cur->size - size - diff_header_size; //same as: allocated_header_size
            size_t remaining_size = available_size - needed_size;

            if(remaining_size >= MIN_SPLIT_SIZE_AL ){
                //split is possible: Allocate a new free block -> link to the list and modify first_free ptr:
                //Returned Block:
                memblk_allocated_t *returned_block = (memblk_allocated_t *)cur;
                returned_block->size = aligned_payload_size; 

                //Allocate a new free block (the split block):
                //memblk_free_t *new_free  = (memblk_free_t *) ( (char *)cur + (size - diff_header_size)  );
                memblk_free_t *new_free = (memblk_free_t *) ( (char *)cur + needed_size ); 
                new_free->size = remaining_size - FREE_HEADER_SIZE_AL;
                new_free->next = cur->next;

                

                //Update first_free (if cur is pointing to the first_free, same as if prev_free is pointing to null):
                if(prev_free == NULL){
                    first_free  = new_free;
                }else{
                    prev_free->next = new_free;
                }

                //Return payload address of the allocated block:
                void *returned_ptr = (void *)( (char *)returned_block + ALLOCATED_HEADER_SIZE_AL );

                //calling print_alloc_info fct: 
                print_alloc_info(returned_ptr,size);

                //Testing the block size function:
                printf("The payload size of the allocated block is: %ld\n",memory_get_allocated_block_size(returned_ptr));

                return returned_ptr;
                
            }else{
                // split is not poss: JUST RETURN ADDRESS & MODIFY list and first_free ptr
                //Returned Block:
                memblk_allocated_t *returned_block = (memblk_allocated_t *)cur;
                // returned_block->size = aligned_size_t;

                // size_t actual_payload = available_size - ALLOCATED_HEADER_SIZE;//no idea
                returned_block->size = available_size - ALLOCATED_HEADER_SIZE_AL; 

                
                //Update first_free:
                if(cur == first_free){
                    first_free = cur->next;
                }

                if(prev_free == NULL){ //In this case we didn't split, so the next node is the new first free, as cur is now allocate!
                    first_free  = cur->next;
                }else{
                    prev_free->next = cur->next;
                }

                //Return payload address of the allocated block:
                void *returned_ptr = (void *)( (char *)returned_block + ALLOCATED_HEADER_SIZE_AL );

                //calling print_alloc_info fct: 
                print_alloc_info(returned_ptr,size);

                //Testing the block size function:
                printf("The size of the allocate block is: %ld\n",memory_get_allocated_block_size(returned_ptr));

                return returned_ptr;

            }

        }else{ // keep going
            prev_free = cur;
            cur = cur->next;
        }
    }

    //No allocation was done!
    return NULL;
}


#elif defined(BEST_FIT)

/* TODO: code specific to the BEST FIT allocation policy can be
 * inserted here */
/*
Smallest waste (Best Fit) : We choose the block b that has the smallest waste. In other
words we choose the block b so that size(b) - block_size is as small as possible.
*/



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

    //Note: header_size = sizeof(size_t) + sizeof(memblk_free)
    
    //Since we will using a free memory block for a allocated memory block, then we should make sure to make the correct definition of the needed size and the available size:
    //Needed size can be figured out from the input size + size of header (as this will be an allocated block):
    // size_t needed_size = size + ALLOCATED_HEADER_SIZE;

    //Wrap-up: Align Header, then align payload -> this is the needed size!
    size_t aligned_payload_size = align_size(size);

    //Testing minimal cases: == forcing the payload size to be at least of 8 bytes such that the whole needed size is at least of 16 bytes size, then we will get a the minimal free block size 
    if( aligned_payload_size < ALLOCATED_HEADER_SIZE ){ 
        aligned_payload_size = ALLOCATED_HEADER_SIZE;
    }
    
    size_t needed_size = aligned_payload_size + ALLOCATED_HEADER_SIZE_AL;

    //Plan:
    //1. start iterating from the free list
    memblk_free_t *cur= first_free;
    memblk_free_t *prev = NULL;
    
    memblk_free_t *best = NULL;
    memblk_free_t *prev_best = NULL;
    
    //The plan is to get the usable block (have enough size) with least remaining size!
    //The best case is if I found a block with remaining =0 -> break loop and use it!
    size_t best_remaining =(size_t)-1;  // large initial value;

    
    //I also need another ptr that will be used for reassigning next ptr while removing an elt from the list!
    //Iterating through the whole list:

    //What should be done is to loop through the whole free list and keep a minimum size AND ptr to the block that gave the min size:
    while(cur != NULL){
        //find first fit:
        size_t available_size = cur->size + FREE_HEADER_SIZE_AL;
        if(available_size >= needed_size){
            size_t remaining_size = available_size - needed_size;
            //Checking if this is the minimum
            if(remaining_size < best_remaining){
                best = cur;
                prev_best = prev;
                best_remaining = remaining_size;

                //particular case where remaining = 0 then just break the loop, no need to keep iterating!
                if(remaining_size == 0){
                    break;
                }
            }
        }
        prev = cur;
        cur = cur->next;
    }

    if(best == NULL ) return NULL;
    
    cur = best;
    prev = prev_best;
    
    if(best_remaining >= MIN_SPLIT_SIZE_AL ){
                //split is possible: Allocate a new free block -> link to the list and modify first_free ptr:
                //Returned Block:
                memblk_allocated_t *returned_block = (memblk_allocated_t *)cur;
                returned_block->size = aligned_payload_size;

                //Allocate a new free block (the split block):
                //memblk_free_t *new_free  = (memblk_free_t *) ( (char *)cur + (size - diff_header_size)  );
                memblk_free_t *new_free = (memblk_free_t *) ( (char *)cur + needed_size ); 
                new_free->size = best_remaining - FREE_HEADER_SIZE_AL;
                new_free->next = cur->next;

                

                //Update first_free (if cur is pointing to the first_free, same as if prev_free is pointing to null):
                if(prev == NULL){
                    first_free  = new_free;
                }else{
                    prev->next = new_free;
                }

                //Return payload address of the allocated block:
                void *returned_ptr = (void *)( (char *)returned_block + ALLOCATED_HEADER_SIZE_AL );

                //calling print_alloc_info fct: 
                print_alloc_info(returned_ptr,size);

                return returned_ptr;
                
            }else{
                // split is not poss: JUST RETURN ADDRESS & MODIFY list and first_free ptr
                //Returned Block:
                memblk_allocated_t *returned_block = (memblk_allocated_t *)cur;

                // Calculate available size for the best block //EDIT COMS
                size_t available_size = cur->size + FREE_HEADER_SIZE_AL;

                // Store actual consumed payload //EDIT COM
                returned_block->size = available_size - ALLOCATED_HEADER_SIZE_AL;

                
                //Update first_free:
                if(prev == NULL){ //In this case we didn't split, so the next node is the new first free, as cur is now allocate!
                    first_free  = cur->next;
                }else{
                    prev->next = cur->next;
                }

                //Return payload address of the allocated block:
                void *returned_ptr = (void *)( (char *)returned_block + ALLOCATED_HEADER_SIZE_AL );

                //calling print_alloc_info fct: 
                print_alloc_info(returned_ptr,size);

                return returned_ptr;

            }


    //No allocation was done!
    return NULL;
}


#elif defined(NEXT_FIT)

/* TODO: code specific to the NEXT FIT allocation policy can be
 * inserted here */

 /*
 • Next Fit: a variant of First Fit, in which the next lookup in the list resumes at the place where
the previous lookup finished. Compared to first fit, this policy tries to avoid having a large
number of very small blocks in the beginning of the list.
 */

 //So just like first fit, but now when we want to keep track of the last inserted 


memblk_free_t *next_fit_start = NULL;
memblk_free_t *next_fit_prev_start = NULL;

void memory_init(void)
{
    /* register the function that will be called when the programs exits */
    atexit(run_at_exit);


    heap_begin = my_mmap(MEM_POOL_SIZE); //to be fixed
    //pointer to first free block:
    first_free = (memblk_free_t *)heap_begin; //casting it such that now we deal with it as free block
    first_free->size = MEM_POOL_SIZE - sizeof(memblk_free_t);
    first_free->next = NULL;

    //pointer to where we start searching
    next_fit_start = first_free;
}


static memblk_free_t *next_free = NULL;

void *memory_alloc(size_t size)
{
    //Wrap-up: Align Header, then align payload -> this is the needed size!
    size_t aligned_payload_size = align_size(size);

    //Testing minimal cases: == forcing the payload size to be at least of 8 bytes such that the whole needed size is at least of 16 bytes size, then we will get a the minimal free block size 
    if( aligned_payload_size < ALLOCATED_HEADER_SIZE_AL ){ 
        aligned_payload_size = ALLOCATED_HEADER_SIZE_AL;
    }
    
    // Needed size for the allocated block
    size_t needed_size = aligned_payload_size + ALLOCATED_HEADER_SIZE_AL;
    

    if (next_free == NULL) {
        next_free = first_free;
    }

    memblk_free_t *cur = next_free;
    memblk_free_t *prev_free = NULL;
    
    // Find the previous node to next_free for proper list management
    //Let us first, get a pointer to the node before first_free
    //Why? Before we were having a pointer to first_free, so prev_free was just NULL and then we update iteratively, but now next_free can be anywhere
    //so then we can search for by just stopping when cur->next == next_free , this will be our prev_free
    if (cur != first_free && first_free != NULL) {
        memblk_free_t *temp = first_free;
        while (temp != NULL && temp->next != cur) {
            temp = temp->next;
        }
        prev_free = temp;
    }

    //First we iterate from next_free to the end of the list
    while (cur != NULL) {
        size_t available_size = cur->size + FREE_HEADER_SIZE_AL;

        if (available_size >= needed_size) {
            size_t remaining_size = available_size - needed_size;

            if (remaining_size >= MIN_SPLIT_SIZE_AL) {
                // Split is possible
                memblk_allocated_t *returned_block = (memblk_allocated_t *)cur;
                returned_block->size = aligned_payload_size;

                // Create new free block
                memblk_free_t *new_free = (memblk_free_t *)((char *)cur + needed_size);
                new_free->size = remaining_size - FREE_HEADER_SIZE_AL;
                new_free->next = cur->next;

                // Update list pointers
                if (prev_free == NULL) {
                    first_free = new_free;
                } else {
                    prev_free->next = new_free;
                }

                // Update next_free for next allocation
                next_free = new_free;

                void *returned_ptr = (void *)((char *)returned_block + ALLOCATED_HEADER_SIZE_AL);
                print_alloc_info(returned_ptr, size);
                return returned_ptr;

            } else {
                // No split
                memblk_allocated_t *returned_block = (memblk_allocated_t *)cur;
                // returned_block->size = size;
                returned_block->size = available_size - ALLOCATED_HEADER_SIZE_AL; 


                // Update list pointers
                if (prev_free == NULL) {
                    first_free = cur->next;
                } else {
                    prev_free->next = cur->next;
                }

                // Update next_free for next allocation
                next_free = cur->next;
                if (next_free == NULL) {
                    next_free = first_free; // Wrap around
                }

                void *returned_ptr = (void *)((char *)returned_block + ALLOCATED_HEADER_SIZE_AL);
                print_alloc_info(returned_ptr, size);
                return returned_ptr;
            }
        }

        prev_free = cur;
        cur = cur->next;
    }

    //Now we iterate from first_free to next_free's position (such that we have iterated the nodes before first_free)
    if (next_free != first_free) {
        cur = first_free;
        prev_free = NULL;

        while (cur != next_free && cur != NULL) {
            size_t available_size = cur->size + FREE_HEADER_SIZE_AL;

            if (available_size >= needed_size) {
                size_t remaining_size = available_size - needed_size;

                if (remaining_size >= MIN_SPLIT_SIZE_AL) {
                    // Splitting case:
                    memblk_allocated_t *returned_block = (memblk_allocated_t *)cur;
                    returned_block->size = aligned_payload_size;

                    memblk_free_t *new_free = (memblk_free_t *)((char *)cur + needed_size);
                    new_free->size = remaining_size - FREE_HEADER_SIZE_AL;
                    new_free->next = cur->next;

                    if (prev_free == NULL) {
                        first_free = new_free;
                    } else {
                        prev_free->next = new_free;
                    }

                    next_free = new_free;

                    void *returned_ptr = (void *)((char *)returned_block + ALLOCATED_HEADER_SIZE_AL);
                    print_alloc_info(returned_ptr, size);
                    return returned_ptr;

                } else {
                    // No split
                    memblk_allocated_t *returned_block = (memblk_allocated_t *)cur;
                    // returned_block->size = size;
                    returned_block->size = available_size - ALLOCATED_HEADER_SIZE_AL; 


                    
                    if (prev_free == NULL) {
                        first_free = cur->next;
                    } else {
                        prev_free->next = cur->next;
                    }

                    next_free = cur->next;
                    if (next_free == NULL) {
                        next_free = first_free;
                    }

                    void *returned_ptr = (void *)((char *)returned_block + ALLOCATED_HEADER_SIZE_AL);
                    print_alloc_info(returned_ptr, size);
                    return returned_ptr;
                }
            }

            prev_free = cur;
            cur = cur->next;
        }
    }

    // No block found
    return NULL;
}



// void *memory_alloc(size_t size)
// {
//     size_t needed_size = size + ALLOCATED_HEADER_SIZE;
    
//     memblk_free_t *cur = NULL;
//     memblk_free_t *prev_free = NULL;
//     memblk_free_t *start = NULL;

//     // Set starting point for next-fit
//     if(next_fit_start != NULL){
//         cur = next_fit_start;
//         prev_free = next_fit_prev_start;
//         start = next_fit_start;
//     } else {
//         cur = first_free;
//         prev_free = NULL;
//         start = first_free;
//     }

//     // If list is empty, return NULL
//     if(cur == NULL){
//         return NULL;
//     }

//     // Search from starting point to end of list
//     while(cur != NULL){
//         size_t available_size = cur->size + FREE_HEADER_SIZE;

//         if(available_size >= needed_size){ 
//             size_t remaining_size = available_size - needed_size;

//             if(remaining_size >= MIN_SPLIT_SIZE){
//                 // Split the block
//                 memblk_allocated_t *returned_block = (memblk_allocated_t *)cur;
//                 returned_block->size = size;

//                 memblk_free_t *new_free = (memblk_free_t *)((char *)cur + needed_size); 
//                 new_free->size = remaining_size - FREE_HEADER_SIZE;
//                 new_free->next = cur->next;

//                 if(prev_free == NULL){
//                     first_free = new_free;
//                 } else {
//                     prev_free->next = new_free;
//                 }

//                 next_fit_start = new_free;
//                 next_fit_prev_start = prev_free;

//                 void *returned_ptr = (void *)((char *)returned_block + ALLOCATED_HEADER_SIZE);
//                 print_alloc_info(returned_ptr, size);
//                 return returned_ptr;
                
//             } else {
//                 // No split - use entire block
//                 memblk_allocated_t *returned_block = (memblk_allocated_t *)cur;
//                 returned_block->size = available_size - ALLOCATED_HEADER_SIZE;

//                 if(prev_free == NULL){
//                     first_free = cur->next;
//                 } else {
//                     prev_free->next = cur->next;
//                 }

//                 next_fit_start = cur->next;
//                 next_fit_prev_start = prev_free;

//                 void *returned_ptr = (void *)((char *)returned_block + ALLOCATED_HEADER_SIZE);
//                 print_alloc_info(returned_ptr, size);
//                 return returned_ptr;
//             }
//         }
        
//         // Move to next block
//         prev_free = cur;
//         cur = cur->next;
//     }

//     // Wrap around: search from beginning to starting point
//     cur = first_free;
//     prev_free = NULL;

//     while(cur != NULL && cur != start){
//         size_t available_size = cur->size + FREE_HEADER_SIZE;

//         if(available_size >= needed_size){ 
//             size_t remaining_size = available_size - needed_size;

//             if(remaining_size >= MIN_SPLIT_SIZE){
//                 // Split the block
//                 memblk_allocated_t *returned_block = (memblk_allocated_t *)cur;
//                 returned_block->size = size;

//                 memblk_free_t *new_free = (memblk_free_t *)((char *)cur + needed_size); 
//                 new_free->size = remaining_size - FREE_HEADER_SIZE;
//                 new_free->next = cur->next;

//                 if(prev_free == NULL){
//                     first_free = new_free;
//                 } else {
//                     prev_free->next = new_free;
//                 }

//                 next_fit_start = new_free;
//                 next_fit_prev_start = prev_free;

//                 void *returned_ptr = (void *)((char *)returned_block + ALLOCATED_HEADER_SIZE);
//                 print_alloc_info(returned_ptr, size);
//                 return returned_ptr;
                
//             } else {
//                 // No split - use entire block
//                 memblk_allocated_t *returned_block = (memblk_allocated_t *)cur;
//                 returned_block->size = available_size - ALLOCATED_HEADER_SIZE;

//                 if(prev_free == NULL){
//                     first_free = cur->next;
//                 } else {
//                     prev_free->next = cur->next;
//                 }

//                 next_fit_start = cur->next;
//                 next_fit_prev_start = prev_free;

//                 void *returned_ptr = (void *)((char *)returned_block + ALLOCATED_HEADER_SIZE);
//                 print_alloc_info(returned_ptr, size);
//                 return returned_ptr;
//             }
//         }
        
//         // Move to next block
//         prev_free = cur;
//         cur = cur->next;
//     }
    
//     return NULL;
// }




//////////////////////////////////////////////////NEW///////////////////////////////////////////////////

// memblk_free_t *starting_point = NULL;
// memblk_free_t *prev_starting = NULL;

// void * memory_alloc(size_t requested_size){
//     size_t needed_size = requested_size + ALLOCATED_HEADER_SIZE;

//     memblk_free_t *cur = NULL;
//     memblk_free_t *prev = NULL;

//     if( starting_point  != NULL ){
//         cur = starting_point;
//         prev = prev_starting;
//     }else{
//         cur = first_free;
//         prev = NULL;
//     }

//     int wrapped = 0; //0 means I haven't reached last element in the list!

//     while( cur != NULL){ 
//         //Test if already iterated the list!
//         if( cur == starting_point && wrapped == 1){
//             return ;
//         }

//         size_t available_size = cur->size + FREE_HEADER_SIZE;

//         if( available_size >= needed_size ){
//             //This block has enough available space:
//             size_t remaining_size = available_size - needed_size;

//             if( remaining_size >= MIN_SPLIT_SIZE){ 
//                 //we can split CASE
//                 memblk_allocated_t *returned_block = (memblk_allocated_t *)(char *)cur;
//                 returned_block->size = requested_size;

//                 //Returned payload address:
//                 void * returned_payload_address = (void *)(  (char*)returned_block + ALLOCATED_HEADER_SIZE);

//                 //New split Free Block:
//                 memblk_free_t *new_free = (memblk_free_t  *) ( (char *)cur + needed_size);
//                 new_free->size = remaining_size - FREE_HEADER_SIZE;
//                 new_free->next = cur->next;

//                 //Updating free list:
//                 if(prev == NULL){
//                     first_free = new_free; //If the allocated block was the first_free (in this case prev == null)
//                 }else{
//                     prev->next = new_free; //normal case!
//                 }

//                 //Updating the Global pointers starting_position AND prev_starting:
//                 starting_point = cur->next;
//                 prev_starting = prev;

//                 //Calling print  function:
//                 print_alloc_info(returned_payload_address,requested_size);

//                 //Returning the payload address:
//                 return returned_payload_address;
                
//             }else{
//                 //We can't split CASE, so take the whole blovk (Result: Internal fragmentation!)
//                 //returned block:
//                 memblk_allocated_t * returned_block = (memblk_allocated_t *)(char *)cur;
//                 returned_block->size = available_size - ALLOCATED_HEADER_SIZE;

//                 //Returned payload address:
//                 void * returned_payload_address = (void *)(  (char*)returned_block + ALLOCATED_HEADER_SIZE);

//                 //Updating free list:
//                 if(prev == NULL){
//                     first_free = cur->next; //If the allocated block was the first_free (in this case prev == null)
//                 }else{
//                     prev->next = cur->next; //normal case!
//                 }

//                 //Updating the Global pointers starting_position AND prev_starting:
//                 starting_point = cur->next;
//                 prev_starting = prev;

//                 //priting alloc fct: input: payload_size
//                 print_alloc_info(returned_payload_address,requested_size);

//                 //returning payload size:
//                 return returned_payload_address;
//             }
//         }else{
//             //Keep Searching through the list:
//             if( cur->next == NULL && wrapped == 0){ //Already reached the end! - Now to not exit loop -> set cur = first_free
//                 cur = first_free;
//                 prev = NULL;
//                 wrapped = 1; 
//             }else{
//                 prev = cur;
//                 cur = cur->next;
//             }
//         }
//     }
// }

///////////////////////////////////////////////////////////////////////////////////////////////


/*

void *memory_alloc(size_t size)
{

    //Note: header_size = sizeof(size_t) + sizeof(memblk_free)
    
    //Since we will using a free memory block for a allocated memory block, then we should make sure to make the correct definition of the needed size and the available size:
    //Needed size can be figured out from the input size + size of header (as this will be an allocated block):
    // size_t needed_size = size + ALLOCATED_HEADER_SIZE;

    size_t aligned_payload_size = align_size(size);

    //minimum of 8 payload enforcement:
    if( aligned_payload_size < ALLOCATED_HEADER_SIZE ){ 
        aligned_payload_size = ALLOCATED_HEADER_SIZE;
    }

    size_t needed_size = aligned_payload_size + ALLOCATED_HEADER_SIZE_AL;

    //memblk_allocated_t *start = next_fit_start==NULL? next_fit_start:first_free; //if next_fit_start = null, then we start from  first_free
    memblk_free_t *start = (next_fit_start != NULL) ? next_fit_start : first_free;

    //Plan:
    //1. start from the free list (allocate first fit):
    memblk_free_t *cur= start;
    memblk_free_t *prev_free = NULL;

    //We want to know if we have already iterated the whole loop and no find a suitable enough space from the free list -> we use wrap flag 
    // such that if we re-visit the starting point -> set flag to true
    //Initially flaf = false:
    int wrapped = 0;

    while( cur != NULL){

        if(cur == start && wrapped == 1){  //In this case we have completed a circle iteration (iterated the whole list)
            return NULL;
        }

        // if(cur == start && prev_free != NULL){ //to be tested!
        //     return NULL;
        // }
        
        size_t available_size = cur->size + FREE_HEADER_SIZE_AL;

        if(available_size >= needed_size){ 
            //compute remaining size:
            size_t remaining_size = available_size - needed_size;

            if(remaining_size >= MIN_SPLIT_SIZE_AL ){
                //split is possible: Allocate a new free block -> link to the list and modify first_free ptr:
                //Returned Block:
                memblk_allocated_t *returned_block = (memblk_allocated_t *)cur;
                returned_block->size = aligned_payload_size;

                //Allocate a new free block (the split block):
                //memblk_free_t *new_free  = (memblk_free_t *) ( (char *)cur + (size - diff_header_size)  );
                memblk_free_t *new_free = (memblk_free_t *) ( (char *)cur + needed_size ); 
                new_free->size = remaining_size - FREE_HEADER_SIZE_AL;
                new_free->next = cur->next;

                

                //Update first_free (if cur is pointing to the first_free, same as if prev_free is pointing to null):
                if(prev_free == NULL){
                    first_free  = new_free;
                }else{
                    prev_free->next = new_free;
                }

                next_fit_start = new_free;

                //Return payload address of the allocated block:
                void *returned_ptr = (void *)( (char *)returned_block + ALLOCATED_HEADER_SIZE_AL );

                //calling print_alloc_info fct: 
                print_alloc_info(returned_ptr,size);

                return returned_ptr;
                
            }else{
                // split is not poss: JUST RETURN ADDRESS & MODIFY list and first_free ptr
                //Returned Block:
                memblk_allocated_t *returned_block = (memblk_allocated_t *)cur;
                returned_block->size = available_size - ALLOCATED_HEADER_SIZE_AL;

                
                //Update first_free:
                if(cur == first_free){
                    first_free = cur->next;
                }

                if(prev_free == NULL){ //In this case we didn't split, so the next node is the new first free, as cur is now allocate!
                    first_free  = cur->next;
                }else{
                    prev_free->next = cur->next;
                }

                next_fit_start = cur->next;

                //Return payload address of the allocated block:
                void *returned_ptr = (void *)( (char *)returned_block + ALLOCATED_HEADER_SIZE_AL );

                //calling print_alloc_info fct: 
                print_alloc_info(returned_ptr,size);

                return returned_ptr;

            }

        }else{ // keep going
            if(cur->next == NULL){
                cur = first_free;
                prev_free = NULL;
                wrapped = 1;
            }else{
                prev_free = cur;
                cur = cur->next;
            }
        }
    }
    return NULL;
} 
*/
#endif


void run_at_exit(void)
{
    fprintf(stderr,"YEAH B-)\n");
    
    /* TODO: insert your code here */
}





void memory_free(void *p)
{

    //Let us check the user is passing a non null add:
    if( p == NULL){
        return;
    }

    /* TODO: insert your code here */

    /* TODO : don't forget to call the function print_free_info()
     * appropriately */
    //Step 0: get the address of p: (shift to the beginning of the block to make sure have taken into consideration the header size):
    //first let us get the size of the allocated block (this is to be passed to the size of free): then we should compute how much is remaining for the free_block:
    //first type casting to correctly get the size of allocated header:
    memblk_allocated_t *add = (memblk_allocated_t *)(((char *)(p) - ALLOCATED_HEADER_SIZE_AL)); 
    // size_t allocated_size = align_size(add->size);
    // printf("Size is: %d, freeing: %d\n",add->size, align_size(add->size));
    size_t allocated_size = add->size;
    // size_t size_of_free_payload = add->size;

    //Now recasting it to free_block
    memblk_free_t *new_add = (memblk_free_t *)add;
    new_add->size = allocated_size + ALLOCATED_HEADER_SIZE_AL - FREE_HEADER_SIZE_AL; //why -8 , because the header size has already 8 additional bits (when allocating free block, this will shrink -> which result with +8 bytes)
    
    //Step1: Search for the correct position of where Block "X" is to be inserted in the free_list:
    memblk_free_t *cur=first_free;
    memblk_free_t *prev= NULL;
    memblk_free_t *temp;

    //Step2: Iterating:
    //cases:
    //case1: if free_list was empty: meaning that it was fully booked, then the pointer to the first_free would be null!
    //In this case the list was already empty,so nothing to mergee
    if(first_free == NULL || new_add < first_free){ //OR: cur == NULL // also case  of first_free > new_add
        
        //temp = first_free; // Here since cur is poiting to the first_free so no need for temp
    
        new_add->next = first_free;
        first_free = new_add;

        //first_free->next = cur; //or: add->next = cur;

        if(new_add->next != NULL){
            //MERGE 2 blocks:
            //lets compute address diff between 2 blocks:
            size_t add_diff = (char *)cur - (char *)first_free;//in terms of byte, so I should first do do byte type casting (char *):

            if(add_diff == first_free->size + FREE_HEADER_SIZE_AL){ //so they are adj: (the diff should be equal to the left part that we are subtracting)
                //then we will the fist block to point to the second's block next 
                temp = new_add->next;
                new_add->size += temp->size + FREE_HEADER_SIZE_AL;
                new_add->next = temp->next;
            }
        }

        //call print_free() function: 
        // print_mem_state();  /////
        print_free_info(p);
        return;
    }

    // //case2: if the freed block is to be placed at the beginning og the heap
    // //HERE we have to check if first(new first_free) and second blocks(where cur is pointing) can be merged
    // if(first_free > new_add ){ //OR: cur> new_add

    // }

    //case3: Usual case; where new_add > first_free (with taking an edge case where new block is at the end of the list):
    while(cur != NULL && new_add > cur){ //Iterating through the whole list (untill current == NULL)
        prev  = cur;
        cur = cur->next;
    }

    //Now we are at the position where the block is to be inserted:

    //Insert Node:
    new_add->next = cur;
    prev->next = new_add;

    //Now check for merge cases:

    //We should merge in sequence: left then right,so:

    //Merge left checking:
    size_t size_diff_left = (char *)new_add  -(char *)prev ;
    if(size_diff_left == prev->size + FREE_HEADER_SIZE_AL ){ // IF left:
        prev->next = new_add->next;
        prev->size += new_add->size + FREE_HEADER_SIZE_AL; // re-make sure of it!
        new_add = prev; // Now we will be checking if this newly block is to be merged on the right
    }

    //Merge right checking:
    //First we have an edge case, which is what if the inserted node was the last node in the list -> no right merging as next == NULL:
    if(new_add->next != NULL){
        size_t size_diff_right = (char *)new_add->next - (char *)new_add; //But there is an edfe
        if(size_diff_right == new_add->size + FREE_HEADER_SIZE_AL){
            temp = new_add->next;
            new_add->size += temp->size + FREE_HEADER_SIZE_AL;
            new_add->next = temp->next;
        }
    }
    

    print_free_info(p);
    //Step4: check for adjacency: If cur & new_add are adj -> merge / If new add & cur->next are adj -> also merge.
    //But what does adj defined in terms of addresses? Difference of 1 byte? (so we should see them in term of byte -> char * casting! then compare)
    //But should I repeat all this block of comparison inside each case, or do a helper function that acccepts addresses as parameter and decide whether to merge or no!

    //What I will just start with is: just do this for each case!
    //In merging we should also increment the size of merged block!
    
}

size_t memory_get_allocated_block_size(void *addr)
{

    /* TODO: insert your code here */

    //So I just shift by the size of the allocated block header and then I now have access to the size, so I can just use:
    //First lets understand what are we taking as input: 
    //Input: address of payload (of the allocated block!):
    memblk_allocated_t *block_beginning = (memblk_allocated_t *)((char *)addr - ALLOCATED_HEADER_SIZE);

    //Now we are pointing at the beginning of the block!
    return block_beginning->size; //just print payload size!
}



void print_mem_state(void){
    //Iterate through the whole heap and print the structure:
    //Lower bound: heap_begin, higher bound: heap_begin + MEM_POOL_SIZE (Those are addresses)
    // start from lower bound and compare if this block is in the free list(if exist -> print, ELSE -> act as it is allocated -> keep iterating untill u didn't reach a free block!) 
    printf("////////////////////////////PRINTING HEAP STRUCTURE/////////////////////\n");
    
    // void *lower_bound  = heap_begin;
    void *higher_bound = (char *)heap_begin + MEM_POOL_SIZE;

    //pointer to current free (the next free block - in each iteration)
    memblk_free_t * cur_free = first_free;//

    void * cur_heap = heap_begin;// 
    while (cur_heap < higher_bound ) //this is never meet
    {
        //We have test what type of blocks we are pointing to:
        if(cur_free == NULL || (char *)cur_heap < (char *)cur_free){ //those are allocated blocks
            //Type casting: As here we are using allocated blocks:
            memblk_allocated_t *cur_block = (memblk_allocated_t *) cur_heap;

            //We have to print number of X -- relative to the size of the bytes!:
            for(int i=0; i< cur_block->size + ALLOCATED_HEADER_SIZE; i++){
                // printf("%d X", i+1);
                printf("%d:X,",i+1);
            }

            //Since Allocated blocks don't have next -> we just increment the pointer with the size of the next position !
            //This will work since we are iterating contigous memory!
            cur_heap =(char *)cur_heap + cur_block->size + ALLOCATED_HEADER_SIZE;
        }else{  //THose   are free blocks
            //Type casting: As here we are using free blocks (using: iterating!)
            memblk_free_t *cur_block = (memblk_free_t*) cur_heap;

            //We have to print number of * -- relative to the size of the bytes!:
            for(int i=0; i< cur_block->size + FREE_HEADER_SIZE; i++){
                // printf("%d *", i+1);
                printf("*");
            }

            //Now since we are dealing with free blocks, then we have to take the size of the free into considerat
            cur_heap = (char *)cur_heap + cur_block->size + FREE_HEADER_SIZE;
            cur_free = cur_free->next;
        }
    }
    printf("\n/////////////////////////////////////////////////END OF PRINTING///////////////////////////////\n");
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
