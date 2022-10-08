#include <assert.h>
#include <stdio.h>

#include "mem_alloc_fast_pool.h"
#include "my_mmap.h"
#include "mem_alloc.h"


/**
 *
 * @param p a pointer to the mem_pool
 * @param the size of the pool to allocate
 * @param min_request_size : minimal amount of payload required be to allocated in this pool
 * @param max_request_size maximal amount of payload required be to allocated in this pool
 */
void init_fast_pool(mem_pool_t *p, size_t size, size_t min_request_size, size_t max_request_size) {

    //Initializing the pool
    char* region = (char *)my_mmap(size);
    p->start = region;
    p->end = region+size-1;
    p->first_free = region;

    //Initializing the list of free-block (each block have the same size (max_request-size)
    for(int i=0; i<size; i+=max_request_size){
        if (i+max_request_size <size){
            ((mem_fast_free_block_t *)&region[i])->next = (mem_fast_free_block_t *)&region[i+max_request_size];
        } else {
            ((mem_fast_free_block_t *)&region[i])->next = NULL;
        }
        // if (max_request_size == 64){
        //     printf("%d:\t",i/max_request_size);
        //     printf("curr: %p\t",((mem_fast_free_block_t *)&region[i]));
        //     printf("next: %p\n",((mem_fast_free_block_t *)&region[i])->next);
            
        // }

    }    
}

/**
 * Allocate in a fast pool
 * @param pool : The pool where the memory is allocated
 * @param size : Size to allocate
 * @return a pointer to the returned block
 */
void *mem_alloc_fast_pool(mem_pool_t *pool, size_t size) {

    //Guards
    if (size > pool->max_request_size)
        return NULL;

    //Updating the list of free block.
    void* result = pool->first_free;
    mem_fast_free_block_t*  next = ((mem_fast_free_block_t *)pool->first_free)->next;
    pool->first_free = (void *)next;

    return result;
    // if (pool->max_request_size == 64){
    //     //printf("%d:\t",i/max_request_size);
    //     printf("curr: %p\t",((mem_fast_free_block_t *)pool->first_free));
    //     //printf("next: %p\n",((mem_fast_free_block_t *)&region[i])->next);
    //     printf("next: %p\n",next);

    // }
}

/**
 * Free an allocated block
 * @param pool the pool where the block is
 * @param a pointer to the block
 */
void mem_free_fast_pool(mem_pool_t *pool, void *b) {

    //Replacing the block in the first index of the free block list
    mem_fast_free_block_t* next = (mem_fast_free_block_t*) pool->first_free;
    ((mem_fast_free_block_t*) b)->next = next;
    pool->first_free = b;
}

/**
 * Return the size of a block.
 * @param pool the pool where the block is
 * @param addr the adress of the block
 * @return the size of the block
 */
size_t mem_get_allocated_block_size_fast_pool(mem_pool_t *pool, void *addr) {
    size_t res;
    res = pool->max_request_size;
    return res;
}
