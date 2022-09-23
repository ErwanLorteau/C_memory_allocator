#include <assert.h>
#include <stdio.h>

#include "mem_alloc_fast_pool.h"
#include "my_mmap.h"
#include "mem_alloc.h"

void init_fast_pool(mem_pool_t *p, size_t size, size_t min_request_size, size_t max_request_size)
{
    char* region = (char *)my_mmap(size);

    p->start = region;
    p->end = region+size;
    p->first_free = region;

    //printf("min: %d, max: %d\n",min_request_size, max_request_size);

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

void *mem_alloc_fast_pool(mem_pool_t *pool, size_t size)
{
    if (size > pool->max_request_size)
        return NULL;
    
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

void mem_free_fast_pool(mem_pool_t *pool, void *b)
{
    mem_fast_free_block_t* next = (mem_fast_free_block_t*) pool->first_free;
    ((mem_fast_free_block_t*) b)->next = next;
    pool->first_free = b;
}

size_t mem_get_allocated_block_size_fast_pool(mem_pool_t *pool, void *addr)
{
    size_t res;
    res = pool->max_request_size;
    return res;
}
