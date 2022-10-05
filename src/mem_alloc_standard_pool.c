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
    set_block_size((mem_std_block_header_footer_t*)&(firstBlock->header), size-(sizeof(mem_std_block_header_footer_t) * 2)); //set_block_size(firstAdressOfPool + 8), size) ; //Optimized
    set_block_free((mem_std_block_header_footer_t*)&(firstBlock->header));

    //We don't know yet the size of the payload, except in the initialization since we know the size of the pool and there is only one block.
    //Inside the 'malloc' functiontion, we need to know the size of the payload in order to know the starting adress of the footer
    
    /**Footer**/
    char* footerAddress = region+(size-sizeof(mem_std_block_header_footer_t));  //Footer is 8 bytes long*
    //mem_std_block_headerfooter_t* footerPointer = firstAdressOfPool + size -8  ;
    // = -8 * size de mem std header  ou = -8 octet
    // Se renseigner sur comment fonctionne le comportement des operateur selon le type static du pointeur a droit et a gauche de l'asignement
    //if char* is passed as parameter instead of any other type*, i'ts probably cast, '->' is an operator which compute the size of the data before the field and modify the pointer ?

    set_block_size((mem_std_block_header_footer_t*)footerAddress, size-(sizeof(mem_std_block_header_footer_t) * 2));
    set_block_free((mem_std_block_header_footer_t*)footerAddress);

    /**List management**/
    firstBlock->next = NULL ;
    firstBlock->prev = NULL ;
}

void *mem_alloc_standard_pool(mem_pool_t *pool, size_t size) {

    mem_std_free_block_t* curr_block = (mem_std_free_block_t *)pool->first_free;

    // Searching for the first fit free bloc

    //#if std_pool_policy == FIRST_FIT

    // make sure to cover case where curr_block == NULL
    while (curr_block != NULL  && get_block_size(&(curr_block->header)) < size)
       curr_block = curr_block->next;

    // #elif std_pool_policy == BEST_FIT
    
    // mem_std_free_block_t* curr_block = (mem_std_free_block_t *)pool->first_free;
    // mem_std_free_block_t* travesal_block = curr_block->next;

    // while (traversal_block != NULL) {
    //     if (get_block_size(&traversal_block->header)-size < get_block_size(&curr_block->header)-size) && (get_block_size(&travesal_block->header) >= size){
    //         curr_block = traversal_block;
    //     }
    // }
    
    // #else
    // perror("Unrecognized pool policy\n");
    // #endif

    char* footer1Address = (char *)&(curr_block->header)+sizeof(mem_std_block_header_footer_t)+size;

    // Split block

    if ((get_block_size(&(curr_block->header)) - size) > (sizeof(mem_std_free_block_t) + sizeof(mem_std_block_header_footer_t))){
        
        size_t tmpSize = get_block_size(&(curr_block->header));

        // Update header in the first block
        set_block_size(&(curr_block->header),size);
        
        // Update footer in the first block
        set_block_free((mem_std_block_header_footer_t *)footer1Address);
        set_block_size((mem_std_block_header_footer_t *)footer1Address,size);

        // Create a header and a footer for the newly created block
        size_t block2size = tmpSize - size - (sizeof(mem_std_free_block_t) + sizeof(mem_std_block_header_footer_t)); 

        char* header2Address = footer1Address + sizeof(mem_std_block_header_footer_t);
        set_block_size((mem_std_block_header_footer_t *)header2Address, block2size);
        set_block_free((mem_std_block_header_footer_t *)header2Address);

        char* footer2Address = header2Address + sizeof(mem_std_block_header_footer_t) + block2size;
        set_block_size((mem_std_block_header_footer_t *)footer2Address, block2size); 
        set_block_free((mem_std_block_header_footer_t *)footer2Address);

        ((mem_std_free_block_t *)header2Address)->next = curr_block->next;
        ((mem_std_free_block_t *)header2Address)->prev = curr_block->prev;

        //  Remove the block from the linked list

        if (curr_block->prev !=  NULL ) {
            curr_block->prev->next = (mem_std_free_block_t *)header2Address;
        } else {
            pool->first_free = (mem_std_free_block_t *)header2Address;
        }
        if (curr_block->next != NULL) {
            curr_block->next->prev = (mem_std_free_block_t *)header2Address;
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

    set_block_used((mem_std_block_header_footer_t *)footer1Address);
    set_block_used(&(curr_block->header));

    return (mem_std_allocated_block_t *)curr_block + 1;
}

/* function merging a block with a header or a footer address with it's (U)pper or (L)ower neighbour */
void merge(mem_std_block_header_footer_t* address, char neighbour){
    /* This function is called after the verification of the ability to merge */
    size_t neighbour_size, block_size;
    block_size = get_block_size(address);

    switch (neighbour){
        case 'U':
            mem_std_block_header_footer_t *neighbour_header, *block_footer;
            neighbour_size = get_block_size(address-1); // Get the block size from the neighbour's footer
            neighbour_header = (mem_std_block_header_footer_t*)((char*)(address-1)-neighbour_size-sizeof(mem_std_block_header_footer_t));
            block_footer = (mem_std_block_header_footer_t*)((char*)(address)+sizeof(mem_std_block_header_footer_t)+block_size);

            set_block_size(neighbour_header,neighbour_size+block_size+sizeof(mem_std_block_header_footer_t)*2);
            set_block_size(block_footer,neighbour_size+block_size+sizeof(mem_std_block_header_footer_t)*2);
            break;

        case 'L':
            mem_std_block_header_footer_t *neighbour_footer, *block_header;
            neighbour_size = get_block_size(address+1); // Get the block size from the neighbour's header
            neighbour_footer = (mem_std_block_header_footer_t*)((char*)(address+1)+sizeof(mem_std_block_header_footer_t)+neighbour_size);
            block_header = (mem_std_block_header_footer_t*)((char*)(address)-block_size-sizeof(mem_std_block_header_footer_t));

            set_block_size(block_header,neighbour_size+block_size+sizeof(mem_std_block_header_footer_t)*2);
            set_block_size(neighbour_footer,neighbour_size+block_size+sizeof(mem_std_block_header_footer_t)*2);

            ((mem_std_free_block_t *)block_header)->next=((mem_std_free_block_t *)(address+1))->next;
            ((mem_std_free_block_t *)block_header)->prev=((mem_std_free_block_t *)(address+1))->prev;
            ((mem_std_free_block_t *)(address+1))->next->prev = ((mem_std_free_block_t *)block_header);
            ((mem_std_free_block_t *)(address+1))->prev->next = ((mem_std_free_block_t *)block_header);
            break;
    }

}

void mem_free_standard_pool(mem_pool_t *pool, void *addr){

    mem_std_free_block_t* block_to_free = (mem_std_free_block_t*)((mem_std_allocated_block_t *)addr-1);
    size_t block_size = get_block_size(&(block_to_free->header));
    mem_std_block_header_footer_t * footerAddress = (mem_std_block_header_footer_t * )((char *)&(block_to_free->header)+sizeof(mem_std_block_header_footer_t)+block_size);
    
    /* Check if block is allocated */
    // if (is_block_free(&(free_block->header))){
    //     perror("Block is already free");
    // }

    set_block_free(&(block_to_free->header));
    
    mem_std_free_block_t* curr = (mem_std_free_block_t *)pool->first_free;

    if (curr == NULL || curr > block_to_free){ // Case where we can only merge with lower neighbour
        pool->first_free = (mem_std_free_block_t *)&(block_to_free->header);
        if(curr != NULL){
            if(is_block_free(footerAddress + 1)){
                merge(footerAddress,'L');
            } else {
                block_to_free->next = curr;
                block_to_free->prev = NULL;
                curr->prev = block_to_free;
            }
        } else {
            block_to_free->next = NULL;
            block_to_free->prev = NULL;
        }
    } else {
        while (curr->next != NULL && curr < block_to_free)
            curr = curr->next;
        if (curr->next == NULL){ // Case where we can only merge with upper neighbour
            if(is_block_free(&(block_to_free->header)-1)){ // Can cause error
                merge(&(block_to_free->header)-1,'U');
            } else {
                curr->next = block_to_free;
                block_to_free->prev = curr;
                block_to_free->next = NULL;
            }
        } else {
            
            if (is_block_free(footerAddress + 1) || is_block_free(&(block_to_free->header)-1)){
                if(is_block_free(footerAddress + 1))
                    //merge with lower neighbour
                    merge(footerAddress + 1,'L');
                if(is_block_free(&(block_to_free->header)-1)) // Can cause error
                    //merge with upper neighbour
                    merge(&(block_to_free->header)-1,'U');
            } else {
                block_to_free->next = curr;
                block_to_free->prev = curr->prev;
                block_to_free->prev->next = block_to_free;
                curr->prev = block_to_free;                
            }
            
        }

    }
}

size_t mem_get_allocated_block_size_standard_pool(mem_pool_t *pool, void *addr)
{
    /* TO BE IMPLEMENTED */
    printf("%s:%d: Please, implement me!\n", __FUNCTION__, __LINE__);
    return 0;
}
