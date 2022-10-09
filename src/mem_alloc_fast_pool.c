#include <assert.h>
#include <stdio.h>

#include "mem_alloc_fast_pool.h"
#include "my_mmap.h"
#include "mem_alloc.h"


/**
 * @param p : pointer to the mem_pool
 * @param size : size of the region to allocate
 * @param min_request_size : minimum amount of payload required be to allocated in this pool
 * @param max_request_size : maximum amount of payload required be to allocated in this pool
 */
void init_fast_pool(mem_pool_t *p, size_t size, size_t min_request_size, size_t max_request_size) {

    /* Initializing the pool */
    char* region = (char *)my_mmap(size);
    p->start = region;
    p->end = region+size-1;
    p->first_free = region;

    /* Initializing the linked list of free-block (each block have the same size (max_request-size)) */
    for(int i=0; i<size; i+=max_request_size){
        if (i+max_request_size <size){
            ((mem_fast_free_block_t *)&region[i])->next = (mem_fast_free_block_t *)&region[i+max_request_size];
        } else {
            ((mem_fast_free_block_t *)&region[i])->next = NULL;
        }
    }    
}

/**
 * Allocates in a fast pool
 * @param pool : pool where the memory is going to be allocated
 * @param size : size to allocate
 * @return : pointer to the returned block
 */
void *mem_alloc_fast_pool(mem_pool_t *pool, size_t size) {

    if (size > pool->max_request_size)
        return NULL;

    /* Updating the list of free block */
    void* result = pool->first_free;
    mem_fast_free_block_t*  next = ((mem_fast_free_block_t *)pool->first_free)->next;
    pool->first_free = (void *)next;

    return result;
}

/**
 * Frees an allocated block
 * @param pool : memory pool where the block is
 * @param b : pointer containing the address of the block to be freed
 */
void mem_free_fast_pool(mem_pool_t *pool, void *b) {

    /* As the alloc function chooses the element with the smallest address,
    when freed this element is going to be the list's head */
    mem_fast_free_block_t* next = (mem_fast_free_block_t*) pool->first_free;
    ((mem_fast_free_block_t*) b)->next = next;
    pool->first_free = b;
}

void free_only_when_allocated_guard(mem_pool_t *pool, void* block_to_free) {


}

/**
 * Returns the size of a block.
 * @param pool : memory pool where the block is
 * @param addr : pointer containing the address of the block
 * @return : size of the block
 */
size_t mem_get_allocated_block_size_fast_pool(mem_pool_t *pool, void *addr) {

    /* The fast pool ignores internal fragmentation, the allocated block sizes are equal to the max_request_size */
    size_t res;
    res = pool->max_request_size;
    return res;
}
