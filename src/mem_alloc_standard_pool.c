#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#include "mem_alloc_types.h"
#include "mem_alloc_standard_pool.h"
#include "my_mmap.h"
#include "mem_alloc.h"

#include <stdbool.h>

/////////////////////////////////////////////////////////////////////////////

#ifdef STDPOOL_POLICY
/* Get the value provided by the Makefile */
std_pool_placement_policy_t std_pool_policy = STDPOOL_POLICY;
#else
std_pool_placement_policy_t std_pool_policy = DEFAULT_STDPOOL_POLICY;
#endif

/////////////////////////////////////////////////////////////////////////////

void init_standard_pool(mem_pool_t *p, size_t size, size_t min_request_size, size_t max_request_size) {
    
    //Call my_mmap & set the head of the pool
    char* region = (char*) my_mmap(size); 

    p->start = region;
    p->end = region+size; 

    p->first_free = region;

    //Initialize the list of free blocs
    mem_std_free_block_t* firstBlock = p->first_free;

    //Initialize the first free block
    //Header & footer setup ( same function for the two of them)

    /**Header**/
    set_block_size((mem_std_block_header_footer_t*)&(firstBlock->header), size); //set_block_size(firstAdressOfPool + 8), size) ; //Optimized
    set_block_free((mem_std_block_header_footer_t*)&(firstBlock->header));

    //We don't know yet the size of the payload, except in the initialization since we know the size of the pool and there is only one block.
    //Inside the 'malloc' functiontion, we need to know the size of the payload in order to know the starting adress of the footer
    
    /**Footer**/
    char* footerAddress = region+(size-sizeof(mem_std_block_header_footer_t));  //Footer is 8 bytes long*
    //mem_std_block_headerfooter_t* footerPointer = firstAdressOfPool + size -8  ;
    // = -8 * size de mem std header  ou = -8 octet
    // Se renseigner sur comment fonctionne le comportement des operateur selon le type static du pointeur a droit et a gauche de l'asignement
    //if char* is passed as parameter instead of any other type*, i'ts probably cast, '->' is an operator which compute the size of the data before the field and modify the pointer ?

    set_block_size((mem_std_block_header_footer_t*)footerAddress, size);
    set_block_free((mem_std_block_header_footer_t*)footerAddress);

    /**List management**/
    firstBlock->next = NULL ;
    firstBlock->prev = NULL ;
}

void *mem_alloc_standard_pool(mem_pool_t *pool, size_t size) {

    mem_std_free_block_t* curr_block = (mem_std_free_block_t *)pool->first_free;

    // Searching for the first fit free bloc

    #if std_pool_policy == FIRST_FIT

    while (curr_block != NULL ) {
        if (get_block_size(&curr_block->header) < size) { 
            curr_block = curr_block->next;
        }
    }

    #elif std_pool_policy == BEST_FIT
    
    mem_std_free_block_t* curr_block = (mem_std_free_block_t *)pool->first_free;
    mem_std_free_block_t* travesal_block = curr_block->next;

    while (traversal_block != NULL) {
        if (get_block_size(&traversal_block->header)-size < get_block_size(&curr_block->header)-size) && (get_block_size(&travesal_block->header) >= size){
            curr_block = traversal_block;
        }
    }
    
    #else
    perror("Unrecognized pool policy\n");
    #endif

    // Split block

    if ((get_block_size(&(curr_block->header)) - size) > (sizeof(mem_std_free_block_t) + sizeof(mem_std_block_header_footer_t))){
        
        size_t tmpSize = get_block_size(&(curr_block->header));

        // Update header in the first block
        set_block_size(&(curr_block->header),size);
        
        // Update footer in the first block
        char* footer1Address = (char *)&(curr_block->header)+sizeof(mem_std_block_header_footer_t)+size;
        set_block_free(footer1Address);
        set_block_size(footer1Address,size);

        // Create a header and a footer for the newly created block
        size_t block2size = tmpSize - size - (sizeof(mem_std_free_block_t) + sizeof(mem_std_block_header_footer_t)); 

        char* header2Address = footer1Address + sizeof(mem_std_block_header_footer_t);
        set_block_size((mem_std_block_header_footer_t*)header2Address, block2size);
        set_block_free((mem_std_block_header_footer_t*)header2Address);

        char* footer2Address = header2Address + sizeof(mem_std_block_header_footer_t) + block2size;
        set_block_size((mem_std_block_header_footer_t*)header2Address, block2size); 
        set_block_free((mem_std_block_header_footer_t*)header2Address);

        ((mem_std_free_block_t *)header2Address)->next = curr_block->next;
        ((mem_std_free_block_t *)header2Address)->prev = curr_block->prev;

        //  Remove the block from the linked list

        if (curr_block->prev !=  NULL ) {
            curr_block->prev->next = header2Address;
        } else {
            pool->first_free = header2Address;
        }
        if (curr_block->next != NULL) {
            curr_block->next->prev = header2Address;
        }

    } else { // Do not split

        if (curr_block->prev !=  NULL ) {
            curr_block->prev->next = curr_block->next;
        } else {
            pool->first_free = curr_block->next;
        }
        if (curr_block->next != NULL) {
            curr_block->next->prev = curr_block->prev;
        }
    }

    set_block_used(&(curr_block->header));

    return (mem_std_allocated_block_t *)curr_block + 1;
}



void mem_free_standard_pool(mem_pool_t *pool, void *addr){

    // mem_std_free_block_t* free_block = (mem_std_free_block_t*) addr;
    // set_block_free(&free_block->header);
    
    // mem_std_free_block_t* curr = (mem_std_free_block_t *)pool->first_free;

    // while (curr->next < addr){
    //     curr = curr->next;
    // }

    // mem_std_free_block_t* tmp_next = curr->next;
    // curr->next = free_block;
    // free_block->prev = curr;
    // free_block->next = tmp_next;
    
}

size_t mem_get_allocated_block_size_standard_pool(mem_pool_t *pool, void *addr)
{
    /* TO BE IMPLEMENTED */
    printf("%s:%d: Please, implement me!\n", __FUNCTION__, __LINE__);
    return 0;
}
