#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#include "mem_alloc_types.h"
#include "mem_alloc_standard_pool.h"
#include "my_mmap.h"
#include "mem_alloc.h"

#include <math.h>

/////////////////////////////////////////////////////////////////////////////

#ifdef STDPOOL_POLICY
/* Get the value provided by the Makefile */
std_pool_placement_policy_t std_pool_policy = STDPOOL_POLICY;
#else
std_pool_placement_policy_t std_pool_policy = DEFAULT_STDPOOL_POLICY;
#endif

/////////////////////////////////////////////////////////////////////////////

void init_standard_pool(mem_pool_t *p, size_t size, size_t min_request_size, size_t max_request_size)
{
    /*
     * Size = 65536
     * Size = total size (including header and footer
     * 1 address = 4 bytes, the value saved = 1 byte long
     * Don't use void * to do pointer arithmetics
     */

    //Call my_mmap & set the head of the pool
    char* firstAdressOfPool = (char*) my_mmap(size) ;  //cast en mem_std_freeblock?
    p->first_free = firstAdressOfPool ;
    p->start =  firstAdressOfPool ;
    p->end = firstAdressOfPool + size ;

    //Initialize the list of free blocs
    mem_std_free_block_t* firstBlock = p->first_free ; //Facilitate access


    //Initialize the first free block
    //Header & footer setup ( same function for the two of them)
    /**Header**/
    set_block_size(&(firstBlock->header), size) ; //set_block_size(firstAdressOfPool + 8), size) ; //Optimized
    set_block_free(&(firstBlock->header)) ;

    //We don't know yet the size of the payload, except in the initialization since we know the size of the pool and there is only one block.
    //Inside the 'malloc' functiontion, we need to know the size of the payload in order to know the starting adress of the footer
    /**Footer**/
    char* addressFooter = firstAdressOfPool + (size -8 ) ;  //Footer is 8 bytes long*
    //mem_std_block_headerfooter_t* footerPointer = firstAdressOfPool + size -8  ;
    // = -8 * size de mem std header  ou = -8 octet
    // Se renseigner sur comment fonctionne le comportement des operateur selon le type static du pointeur a droit et a gauche de l'asignement
    //if char* is passed as parameter instead of any other type*, i'ts probably cast, '->' is an operator which compute the size of the data before the field and modify the pointer ?

    set_block_size(addressFooter, size) ;
    set_block_free(addressFooter) ;

    /*
    int bs = (int) get_block_size(addressFooter) ;
    printf("size of the block : %d \n" ,bs) ;
     */

    //Debug
    printf("first address of pool : %x \n" , firstAdressOfPool) ;
    printf("first address of footer %x \n" , addressFooter) ;
    printf("size de l'espace mémoire alloué en bit :  %d \n" , firstAdressOfPool - size*8) ;


    //List management
    firstBlock->next = NULL ;
    firstBlock->prev = NULL ;
}

void* search_first_free_bloc(size_t size, mem_std_free_block_t* head) {
    while (head->next != NULL ) {
        if (head->header.flag_and_size - (uint64_t)(pow(2.0, 63)) >= size) {
            return head;
        }
        head = head->next;
    }
    exit(0) ; //If no free block of the rigth size is available, fail
}

//size = size wanted by the user, doesn't include the header/footer size
void *mem_alloc_standard_pool(mem_pool_t *pool, size_t size) {
   //Search for the first free block
    mem_std_free_block_t* first_free_bloc = search_first_free_bloc(size, pool->first_free) ;

   //Deleting the block from the free-list
    first_free_bloc->prev->next = first_free_bloc->next ;
    first_free_bloc->next->prev = first_free_bloc->prev ;
    return first_free_bloc ;
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
