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

/**
 * Initialize the standard pool
 * @param p : a pointer to the standard pool to initialize
 * @param size : the size to allocate for the standart pool
 * @param min_request_size : the minimum size required for a block to be allocated in the standard pool
 * @param max_request_size : the maximum size required for a block to be allocated in the standard pool
 */

void init_standard_pool(mem_pool_t *p, size_t size, size_t min_request_size, size_t max_request_size) {
    
    /* Initializing the pool structure */
    char* region = (char*) my_mmap(size);
    p->start = region;
    p->end = region+size-1; // Largest memory address in the region
    p->first_free = region;

    /* Intializating the first free block */
    mem_std_free_block_t* firstBlock = p->first_free;

    /* Header initialization */
    /* Block size is the size of the allocated region using minus the size of header and the footer (Both equal = 8 bytes long) */
    set_block_size((mem_std_block_header_footer_t*)&(firstBlock->header), size-(sizeof(mem_std_block_header_footer_t) * 2)); 
    set_block_free((mem_std_block_header_footer_t*)&(firstBlock->header));

    /* Footer initialization */
    char* footerAddress = region+(size-sizeof(mem_std_block_header_footer_t));  //Footer is 8 bytes long
    set_block_size((mem_std_block_header_footer_t*)footerAddress, size-(sizeof(mem_std_block_header_footer_t) * 2));
    set_block_free((mem_std_block_header_footer_t*)footerAddress);

    /* Predecessor and successor of the only free block */
    firstBlock->next = NULL ;
    firstBlock->prev = NULL ;
}

/**
 * Allocate memory inside the standard pool for the user
 * @param pool : pool where the memory is allocated
 * @param size : size of the memory allocated
 * @return : pointer to the allocated block
 */
void *mem_alloc_standard_pool(mem_pool_t *pool, size_t size) {

    mem_std_free_block_t* curr_block = (mem_std_free_block_t *)pool->first_free;

    /* Iterate over the linked list of free blocks
    Depending on search policy, find and save the address of the block to allocate in curr_block */

    if (std_pool_policy == FIRST_FIT){
        /* Find the first free block that fits */
        while (curr_block != NULL  && get_block_size(&(curr_block->header)) < size)
            curr_block = curr_block->next;

    } else if (std_pool_policy == BEST_FIT){
        /* Find the block with the least internal fragmentation */
        mem_std_free_block_t* traversal_block = curr_block->next;
        while (traversal_block != NULL) {
            if ((get_block_size(&traversal_block->header)-size < get_block_size(&curr_block->header)-size) && (get_block_size(&traversal_block->header) >= size)){
                curr_block = traversal_block;
            }
            traversal_block = traversal_block->next;
        }
    } else {
        perror("Unrecognized pool policy\n");
    }
  

    char* footer1Address = (char *)&(curr_block->header)+sizeof(mem_std_block_header_footer_t)+size;

    /* Split block */
    /* Size of initial block - requested size for allocation >= 32 */
    if ((get_block_size(&(curr_block->header)) - size) >= (sizeof(mem_std_free_block_t) + sizeof(mem_std_block_header_footer_t))){
        
        size_t tmpSize = get_block_size(&(curr_block->header));

        /* Update header in the first block */
        set_block_size(&(curr_block->header),size);
        
        /* Update footer in the first block */
        set_block_size((mem_std_block_header_footer_t *)footer1Address,size);

        /* Create a header and a footer for the newly created block */
        size_t block2size = tmpSize - (size + sizeof(mem_std_block_header_footer_t)*2); 

        char* header2Address = footer1Address + sizeof(mem_std_block_header_footer_t);
        set_block_size((mem_std_block_header_footer_t *)header2Address, block2size);
        set_block_free((mem_std_block_header_footer_t *)header2Address);

        char* footer2Address = header2Address + sizeof(mem_std_block_header_footer_t) + block2size;
        set_block_size((mem_std_block_header_footer_t *)footer2Address, block2size); 
        set_block_free((mem_std_block_header_footer_t *)footer2Address);

        /* Initializa the next and prev pointers for the new block and remove the to-be allocated block from the linked list */
        
        ((mem_std_free_block_t *)header2Address)->next = curr_block->next;
        ((mem_std_free_block_t *)header2Address)->prev = curr_block->prev;

        if (curr_block->prev !=  NULL ) {
            curr_block->prev->next = (mem_std_free_block_t *)header2Address;
        } else {
            pool->first_free = (mem_std_free_block_t *)header2Address;
        }
        if (curr_block->next != NULL) {
            curr_block->next->prev = (mem_std_free_block_t *)header2Address;
        }

    } else { // Do not split // To be fixed with issue #11

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


/**
 * Merges two blocks in a free-block list
 * @param address : address of the header or the footer of the block to be freed
 * @param neighbour : character indicating if it's the (U)pper neighbour or the (L)ower neighbour
 */
void merge(mem_std_block_header_footer_t* address, char neighbour){
    /* This function is called after the verification of the ability to merge */

    size_t neighbour_size, block_size;
    block_size = get_block_size(address);

    switch (neighbour){
        case 'U': {

            /* Case where the merge is with neighbour with a smaller address (upper neighbour);
            The newly created block will contain a header which is the neighbour's block header, and a footer which is the to-be freed block's footer 
            The size of the new block (after merge) is the sum of the sizes of the to-be freed block, the neighbour block, the to-be freed block's header and neighbour's footer
            */

            mem_std_block_header_footer_t *neighbour_header;
            mem_std_block_header_footer_t *block_footer;
            neighbour_size = get_block_size(address - 1); // Get the block size from the neighbour's footer
            neighbour_header = (mem_std_block_header_footer_t *) ((char *) (address - 1) - neighbour_size -
                                                                  sizeof(mem_std_block_header_footer_t));
            block_footer = (mem_std_block_header_footer_t *) ((char *) (address) +
                                                              sizeof(mem_std_block_header_footer_t) + block_size);

            set_block_size(neighbour_header, neighbour_size + block_size + sizeof(mem_std_block_header_footer_t) *
                                                                           2); // Size of header = size of footer = 8 bytes
            set_block_size(block_footer, neighbour_size + block_size + sizeof(mem_std_block_header_footer_t) * 2);

            ((mem_std_free_block_t *) neighbour_header)->next = ((mem_std_free_block_t *) (address))->next;

            if (((mem_std_free_block_t *) (address))->next != NULL) {
                ((mem_std_free_block_t *) (address))->next->prev = ((mem_std_free_block_t *) neighbour_header);
            }

            break;
        }
        case 'L': {

            /* Case where the merge is with neighbour with a larger address (lower neighbour) 
            The header of the newly created block will contain a the header of the to-be freed block as its header and the neighbour's footer as its footer 
            The size of the new block is the sum of sizes of the to-be freed block, the neighbour block, the to-be freed block's footer and the neighbour's header */

            mem_std_block_header_footer_t *neighbour_footer, *block_header;
            neighbour_size = get_block_size(address + 1); // Get the block size from the neighbour's header
            neighbour_footer = (mem_std_block_header_footer_t *) ((char *) (address + 1) +
                                                                  sizeof(mem_std_block_header_footer_t) +
                                                                  neighbour_size);
            block_header = (mem_std_block_header_footer_t *) ((char *) (address) - block_size -
                                                              sizeof(mem_std_block_header_footer_t));

            set_block_size(block_header, neighbour_size + block_size + sizeof(mem_std_block_header_footer_t) * 2);
            set_block_size(neighbour_footer, neighbour_size + block_size + sizeof(mem_std_block_header_footer_t) * 2);

            ((mem_std_free_block_t *) block_header)->next = ((mem_std_free_block_t *) (address + 1))->next;
            ((mem_std_free_block_t *) block_header)->prev = ((mem_std_free_block_t *) (address + 1))->prev;

            if (((mem_std_free_block_t *) (address + 1))->next != NULL)
                ((mem_std_free_block_t *) (address + 1))->next->prev = ((mem_std_free_block_t *) block_header);
            if (((mem_std_free_block_t *) (address + 1))->prev != NULL)
                ((mem_std_free_block_t *) (address + 1))->prev->next = ((mem_std_free_block_t *) block_header);

            break;
        }
    }

}
/**
 * Free a block after an user request
 * @param pool : the memory pool's address
 * @param addr : the address of the to-be freed block (payload address)
 */
void mem_free_standard_pool(mem_pool_t *pool, void *addr){

    mem_std_free_block_t* block_to_free = (mem_std_free_block_t*)((mem_std_allocated_block_t *)addr-1); // Find the address of the header from the payload address
    size_t block_size = get_block_size(&(block_to_free->header));
    mem_std_block_header_footer_t * footerAddress = (mem_std_block_header_footer_t * )((char *)&(block_to_free->header)+sizeof(mem_std_block_header_footer_t)+block_size);

    set_block_free(&(block_to_free->header));
    set_block_free(footerAddress);

    mem_std_free_block_t* curr = (mem_std_free_block_t *)pool->first_free;

    // TODO: Refactor and comment
    if (curr == NULL || curr > block_to_free){
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
        // This part of code causes a logical error
        while (curr->next != NULL && curr < block_to_free)
            curr = curr->next;
        // Added a debugging case (&& curr < block_to_free)
        if (curr->next == NULL && curr < block_to_free){ // Case where we can only merge with upper neighbour
            if(is_block_free(&(block_to_free->header)-1)){ // Can cause error
                merge(&(block_to_free->header),'U');
            } else {
                curr->next = block_to_free;
                block_to_free->prev = curr;
                block_to_free->next = NULL;
            }
        } else {
            
            if (is_block_free(footerAddress + 1) || is_block_free(&(block_to_free->header)-1)){
                if(is_block_free(footerAddress + 1))
                    //merge with lower neighbour
                    merge(footerAddress,'L');
                if(is_block_free(&(block_to_free->header)-1)) // Can cause error
                    //merge with upper neighbour
                    merge(&(block_to_free->header),'U');
            } else {
                block_to_free->next = curr;
                block_to_free->prev = curr->prev;
                if (block_to_free->prev != NULL)
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
