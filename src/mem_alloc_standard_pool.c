#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#include "mem_alloc_types.h"
#include "mem_alloc_standard_pool.h"
#include "my_mmap.h"
#include "mem_alloc.h"

#include<stdbool.h>

/////////////////////////////////////////////////////////////////////////////

#ifdef STDPOOL_POLICY
/* Get the value provided by the Makefile */
std_pool_placement_policy_t std_pool_policy = STDPOOL_POLICY;
#else
std_pool_placement_policy_t std_pool_policy = DEFAULT_STDPOOL_POLICY;
#endif

/////////////////////////////////////////////////////////////////////////////
//Size = total size (including header and footer
void init_standard_pool(mem_pool_t *p, size_t size, size_t min_request_size, size_t max_request_size) {
    //Call my_mmap & set the head of the pool
    char* firstAddressOfPool = (char*) my_mmap(size) ;  //cast en mem_std_freeblock?
    p->first_free = firstAddressOfPool ;
    p->start =  firstAddressOfPool ;
    p->end = firstAddressOfPool + size ;

    //Initialize the list of free blocs
    mem_std_free_block_t* firstBlock = p->first_free ; //Facilitate access

    /**Initialize first free bloc**/
    /**Header**/
    mem_std_block_header_footer_t* header = firstAddressOfPool  ;  //Footer is 8 bytes long
    initializeHeader(header, size, true) ;

    /**Footer**/
    mem_std_block_header_footer_t* footer = firstAddressOfPool + (size - 8 ) ;  //Footer is 8 bytes long
    initializeHeader(addressFooter, size, true) ;

    //List management
    firstBlock->next = NULL ;
    firstBlock->prev = NULL ;
}


/**
 *
 * @param size_t : minimal size needed
 * @param head head of the free block list
 * @return first free block of the list which is bug enough
 */
mem_std_free_block_t* search_first_fit_free_bloc(size_t size, mem_std_free_block_t* head) {
    while (head->next != NULL ) {
        if (get_block_size(&head->header) + 16 >= size && head != NULL) { //16 is the size of succ/prec which will be removed
            return head;
        } else {
            exit(1); //fail
        }
    }
}

/**
 * Delete the given block of the free list
 * @param bloc the bloc to delete
 */
void deleteFromFreeList(mem_std_free_block_t* bloc) {
        if (bloc->prev !=  NULL ) {
            bloc->prev->next = bloc->next;
        }
        if (bloc->next != NULL) {
            bloc->next->prev = bloc->prev;
        }
    }
}

/**
 * @param header an header/footer address
 * @param size size to set in the header
 * @param isFree
 */

void initializeHeader(mem_std_block_headerfooter_t* header, size_t size, boolean isFree) {
    if (isFree) {
        set_block_free(header) ;
    } else {
        set_block_used(header) ;
    }
    set_block_size(header , size) ;
}

void *mem_alloc_standard_pool(mem_pool_t *pool, size_t size) {
    #IFDEF (DEFAULT_STDPOOL_POLICY = 'FIRSTFIT')
        //Searching  the first free bloc
        mem_std_free_block_t *first_free_bloc = search_first_fit_free_bloc(size, pool->first_free);

        //Deleting the block from the list
        deleteFromFreeList(first_free_bloc) ;

        //Initializing the header/footer
        mem_std_block_header_footer_t  header = NULL, footer = NULL ;
        header = &(first_free_bloc->header) ;
        /**TODO::SPLIT**/
        /**
         * If to small (<1048 ) --> raison the payload size
         * if big enough (split)
         * ! size will change in next line
         */
        footer = (char*) (header) + 8 + size ; //Start address of footer is header address + header size + payload size (use the left)
        initializeHeader(header, size, false) ; //BE CAREFULL THAT THE SIZE SAVED MUST BE THE REAL SIZE (POSSIBLY RAISE BY SPLIT/MERGE OPERATION, NOT THE SIZE OF THE REAL DATA USED)
        initializeHeader(footer, size, false) ;

        //No predecessor / successor since the block is allocated
        return header + 1  ; //header starting address + 8 ( header size = 8 )
    #ENDIFDEF
}

void mem_free_standard_pool(mem_pool_t *pool, void *addr)
{
    /* TO BE IMPLEMENTED */
    printf("%s:%d: Please, implement me!\n", __FUNCTION__, __LINE__);
}

size_t mem_get_allocated_block_size_standard_pool(mem_pool_t *pool, void *addr)
{
    /* TO BE IMPLEMENTED */
    printf("%s:%d: Please, implement me!\n", __FUNCTION__, __LINE__);
    return 0;
}
