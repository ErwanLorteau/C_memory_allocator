#define _GNU_SOURCE

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <dlfcn.h>
#include <stdbool.h>

#include "mem_alloc.h"
#include "mem_alloc_types.h"
#include "mem_alloc_fast_pool.h"
#include "mem_alloc_standard_pool.h"

/* Number of memory pools managed by the allocator */
#define NB_MEM_POOLS 4

#define ULONG(x) ((long unsigned int)(x))

/* Pointers to the original malloc functions */
void *(*o_malloc)(size_t) = NULL;
void (*o_free)(void *) = NULL;
void *(*o_realloc)(void *, size_t) = NULL;
void *(*o_calloc)(size_t, size_t) = NULL;

/* Array of memory pool descriptors (indexed by pool id) */
static mem_pool_t mem_pools[NB_MEM_POOLS];


/* Note: the other fields will be setup by the init procedure */
static mem_pool_t fast_pool_1_64 = {
    .pool_id = 0,
    .pool_name = "pool-0-fast (1_64)",
    .pool_size = MEM_POOL_0_SIZE,
    .min_request_size = 1,
    .max_request_size = 64,
    .pool_type = FAST_POOL};

/* Note: the other fields will be setup by the init procedure */
static mem_pool_t fast_pool_65_256 = {
    .pool_id = 1,
    .pool_name = "pool-1-fast (65_256)",
    .pool_size = MEM_POOL_1_SIZE,
    .min_request_size = 65,
    .max_request_size = 256,
    .pool_type = FAST_POOL};

/* Note: the other fields will be setup by the init procedure */
static mem_pool_t fast_pool_257_1024 = {
    .pool_id = 2,
    .pool_name = "pool-2-fast (257_1024)",
    .pool_size = MEM_POOL_2_SIZE,
    .min_request_size = 257,
    .max_request_size = 1024,
    .pool_type = FAST_POOL};

/* Note: the other fields will be setup by the init procedure */
static mem_pool_t standard_pool_1025_and_above = {
    .pool_id = 3,
    .pool_name = "pool-3-std (1024_Max)",
    .pool_size = MEM_POOL_3_SIZE,
    .min_request_size = 1025,
    .max_request_size = SIZE_MAX,
    .pool_type = STANDARD_POOL};


/**
 * to execute before quiting, to make sure every block has been freed in the fast pools
 * @param pool the pool to check
 * @return true if the full doesn't contain any allocated block
 */
bool fast_pool_is_empty(mem_pool_t pool) {
    int number_of_free_block = 0 ;
    mem_fast_free_block_t* current = pool.first_free ;

    while (current != NULL) {
        number_of_free_block++ ;
        current = current->next ;
    }
    return (number_of_free_block == pool.pool_size / pool.max_request_size ) ;
}

/* This function is automatically called upon the termination of a process. */
void run_at_exit(void) {
    if ( fast_pool_is_empty(mem_pools[0]) ) {
        printf("First fast pool....... OK \n") ;
    } else {
        printf("First fast poool...... NOT OK  : Some block haven't been freed in the first fast pool. \n") ;
    }

    if ( fast_pool_is_empty(mem_pools[1]) ) {
        printf("Second fast pool....... OK \n") ;
    } else {
        printf("Second fast poool...... NOT OK  : Some block haven't been freed in the second fast pool. \n") ;
    }


    if ( fast_pool_is_empty(mem_pools[2]) ) {
        printf("Third fast pool....... OK \n") ;
    } else {
        printf("Third fast poool...... NOT OK  : Some block haven't been freed in the third fast Pool. \n") ;
    }
}




/* 
 * Returns the id of the pool in charge of a given block size.
 * Note:
 * We assume that the contents of the pools are consistent
 * (pools covering all possible block sizes,
 * sorted by increasing sizes,
 * no overlap).
 */
static int find_pool_from_block_size(size_t size)
{
    int i;
    int res = -1;
    debug_printf("size=%lu\n", size);
    for (i = 0; i < NB_MEM_POOLS; i++)
    {
        if ((size >= mem_pools[i].min_request_size) && (size <= mem_pools[i].max_request_size))
        {
            res = i;
            break;
        }
    }
    debug_printf("will return %d\n", res);
    assert(res >= 0);
    return res;
}

/*
 * Returns the id of the pool in charge of a given block.
 */
int find_pool_from_block_address(void *addr)
{
    int i;
    int res = -1;
    debug_printf("enter - addr = %p\n", addr);
    for (i = 0; i < NB_MEM_POOLS; i++)
    {
        if ((addr >= mem_pools[i].start) && (addr <= mem_pools[i].end))
        {
            res = i;
            break;
        }
    }
    debug_printf("will return %d\n", res);
    return res;
}

void memory_init(void)
{
    int i;

    /* Register the function that will be called when the process exits. */
    atexit(run_at_exit);

    /* Init all the pools */
    mem_pools[0] = fast_pool_1_64;
    mem_pools[1] = fast_pool_65_256;
    mem_pools[2] = fast_pool_257_1024;
    mem_pools[3] = standard_pool_1025_and_above;

    for (i = 0; i < 3; i++)
    {
        init_fast_pool(&(mem_pools[i]), mem_pools[i].pool_size, mem_pools[i].min_request_size, mem_pools[i].max_request_size);
    }
    init_standard_pool(&(mem_pools[3]), mem_pools[3].pool_size, mem_pools[3].min_request_size, mem_pools[3].max_request_size);

    /* checks that the pools request sizes are not overlapping */
    assert(mem_pools[0].min_request_size == 1);
    for (i = 0; i < NB_MEM_POOLS - 1; i++)
    {
        assert(mem_pools[i].max_request_size + 1 == mem_pools[i + 1].min_request_size);
        debug_printf("mem_pools[%d]: size=%lu, min_request_size=%lu, max_request_size=%lu\n", i, mem_pools[i].pool_size, mem_pools[i].min_request_size, mem_pools[i].max_request_size);
    }
    debug_printf("mem_pools[%d]: size=%lu, min_request_size=%lu, max_request_size=%lu\n", i, mem_pools[i].pool_size, mem_pools[i].min_request_size, mem_pools[i].max_request_size);
    assert(mem_pools[NB_MEM_POOLS - 1].max_request_size == SIZE_MAX);

    /* Init the pointers to the original malloc functions */
    o_malloc = dlsym(RTLD_NEXT, "malloc");
    o_free = dlsym(RTLD_NEXT, "free");
    o_realloc = dlsym(RTLD_NEXT, "realloc");
    o_calloc = dlsym(RTLD_NEXT, "calloc");
}

/* 
 * Entry point for allocation requests.
 * Forwards the request to the appopriate pool.
 */
void *memory_alloc(size_t size)
{
    int i;
    void *alloc_addr = NULL;
    debug_printf("enter size = %lu\n", size);
    i = find_pool_from_block_size(size);
    switch (mem_pools[i].pool_type)
    {
    case FAST_POOL:
        alloc_addr = mem_alloc_fast_pool(&(mem_pools[i]), size);
        break;
    case STANDARD_POOL:
        alloc_addr = mem_alloc_standard_pool(&(mem_pools[i]), size);
        break;
    default: /* we should never reach this case */
        assert(0);
    }
    if (alloc_addr == NULL)
    {
        print_alloc_error(size);
        exit(0);
    }
    else
    {
        print_alloc_info(alloc_addr, size);
    }
    debug_printf("return %p\n", alloc_addr);
    return alloc_addr;
}

/* 
 * Entry point for deallocation requests.
 * Forwards the request to the appopriate pool.
 */
void memory_free(void *p)
{
    int i;

    debug_printf("enter p = %p\n", p);
    i = find_pool_from_block_address(p);

    switch (mem_pools[i].pool_type)
    {
    case FAST_POOL:
        mem_free_fast_pool(&(mem_pools[i]), p);
        break;
    case STANDARD_POOL:
        mem_free_standard_pool(&(mem_pools[i]), p);
        break;
    default: /* we should never reach this case */
        assert(0);
    }
    print_free_info(p);
    debug_printf("exit\n");
}

/* Returns the payload size of an allocated block */
size_t memory_get_allocated_block_size(void *addr)
{
    int i;
    size_t res;
    i = find_pool_from_block_address(addr);
    debug_printf("i = %d\n", i);
    assert(i >= 0);
    switch (mem_pools[i].pool_type)
    {
    case FAST_POOL:
        res = mem_get_allocated_block_size_fast_pool(&(mem_pools[i]), addr);
        break;
    case STANDARD_POOL:
        res = mem_get_allocated_block_size_standard_pool(&(mem_pools[i]), addr);
        break;
    default: /* we should never reach this case */
        assert(0);
    }
    return res;
}


/**
 * Print the standart pool state in the terminal
 */

void print_standart_pool(void) {

    //Starting & ending address of the pool
    char* start = mem_pools[3].start;
    printf("start: %p\n",start);
    printf("end: %p\n", (char *)(mem_pools[3].end));

    int size = get_block_size((mem_std_block_header_footer_t *)start);
    char* curr = start;

    //Printing each block informations
    while (curr < (char *)(mem_pools[3].end)){
        size = get_block_size((mem_std_block_header_footer_t *)curr);
        printf("[ ");
        if (!is_block_free((mem_std_block_header_footer_t *)curr)){
            printf("XX ");
        }
        printf("from: %p to: %p -- size: %d",curr,curr+15+size,size);
        printf(" ]\n");
        curr = curr + size +16;
    }
}

/**
 * print fast pools
 */

void print_fast_pool(void) {
    for (int  i = 0 ; i < 3 ; i ++) {
        char *start = mem_pools[i].start;
        printf("Pool n° %d", i);
        printf("Start: %p\n", start);
        printf("End: %p\n", (char *) (mem_pools[i].end));

        //Initializing traversal block
        int size = get_block_size((mem_std_block_header_footer_t *) start);
        char *current = start;

        //Printing
        size = mem_pools[i].max_request_size ;
        while (current < (char *) (mem_pools[i].end)) {
            printf("[ ");
            printf("from: %p to: %p -- size: %d", current, current + size, size);
            printf(" ]\n");
            current = current + size ;
        }
    }
}

/**
 * print standart pool
 */
void print_mem_state(void) {
    print_fast_pool() ;
    print_standart_pool() ;
}

void print_free_info(void *addr)
{
    if (addr)
    {
        int i;
        i = find_pool_from_block_address(addr);

        fprintf(stderr, "FREE  at : %lu -- pool %d\n", ULONG((char*)addr - (char*)mem_pools[i].start), mem_pools[i].pool_id);
    }
    else
    {
        fprintf(stderr, "FREE  at : %lu \n", ULONG(0));
    }
}

void print_alloc_info(void *addr, int size)
{
    if (addr)
    {
        int i;
        i = find_pool_from_block_address(addr);

        fprintf(stderr, "ALLOC at : %lu (%d byte(s)) -- pool %d\n",
                ULONG((char*)addr - (char*)mem_pools[i].start), size, mem_pools[i].pool_id);
    }
    else
    {
        print_alloc_error(size);
    }
}

void print_alloc_error(int size)
{
    fprintf(stderr, "ALLOC error : can't allocate %d bytes\n", size);
}

#ifdef MAIN
/* The main function can be changed (to perform simple tests).
 * It is NOT involved in the automated tests
 */
int main(int argc, char **argv)
{

    memory_init();

    int i;
    for (i = 0; i < 10; i++)
    {
        char *b = memory_alloc(rand() % 8);
        memory_free(b);
    }

    char *a = memory_alloc(15);
    memory_free(a);

    a = memory_alloc(10);
    memory_free(a);

    fprintf(stderr, "%lu\n", (long unsigned int)(memory_alloc(9)));
    return EXIT_SUCCESS;
}
#endif /* MAIN */
