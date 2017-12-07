/**
 * All functions you make for the assignment must be implemented in this file.
 * Do not submit your assignment with a main function in this file.
 * If you submit with a main function in this file, you will get a zero.
 */
#include "sfmm.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

/**
 * You should store the heads of your free lists in these variables.
 * Doing so will make it accessible via the extern statement in sfmm.h
 * which will allow you to pass the address to sf_snapshot in a different file.
 */
free_list seg_free_list[4] = {
    {NULL, LIST_1_MIN, LIST_1_MAX},
    {NULL, LIST_2_MIN, LIST_2_MAX},
    {NULL, LIST_3_MIN, LIST_3_MAX},
    {NULL, LIST_4_MIN, LIST_4_MAX}
};

int listLength = sizeof(seg_free_list)/sizeof(*seg_free_list);

// Helper function to print the contents of a header or footer
void printHeader(sf_header* header){
    printf("Header address is %p\n", header);
    printf("Header->block_size is %d\n", (*header).block_size*16);
    printf("Header->two_zeroes is %d\n", (*header).two_zeroes);
    printf("Header->padded is %d\n", (*header).padded);
    printf("Header->allocated is %d\n", (*header).allocated);
}

void printFooter(sf_footer* footer){
    printf("Footer address is %p\n", footer);
    printf("Footer->requested_size is %d\n", (*footer).requested_size);
    printf("Footer->block_size is %d\n", (*footer).block_size*16);
    printf("Footer->two_zeroes is %d\n", (*footer).two_zeroes);
    printf("Footer->padded is %d\n", (*footer).padded);
    printf("Footer->allocated is %d\n", (*footer).allocated);
}

void addToFreeList(sf_header* header){
    //printf("Adding to freelist\n");
    int listNum;
    uint64_t block_size = (*header).block_size * 16;
    for(listNum = 0; listNum < listLength; listNum++){
        if(block_size <= seg_free_list[listNum].max){
            sf_free_header* oldHead = seg_free_list[listNum].head;
            if(oldHead == NULL){
                //printf("No old head\n");
                sf_free_header* free_header = (sf_free_header*)((void*)header);
                (*free_header).header = *header;
                (*free_header).next = NULL;
                (*free_header).prev = NULL;
                seg_free_list[listNum].head = free_header;
            }
            else{
                //printf("Replacing old head\n");
                sf_free_header* free_header = (sf_free_header*)((void*)header);
                (*free_header).header = *header;
                (*free_header).next = oldHead;
                (*free_header).prev = NULL;
                (*oldHead).prev = free_header;

                seg_free_list[listNum].head = free_header;
            }
            //printf("New free block added to free list %d\n", listNum + 1);
            break;
        }
    }
    //sf_snapshot();
}

void removeFromFreeList(sf_header* header){ // Assuming that the parameters are valid and that the free list is correct.
    /*if((*seg_free_list[listNum]).head == NULL){
        return -1;
    }*/
    int listNum;
    uint64_t block_size = (*header).block_size * 16;
    //printf("Block size is %lu\n", block_size);
    for(listNum = 0; listNum < listLength; listNum++){
        //printf("Max of list %d is %d\n", listNum, seg_free_list[listNum].max);
        if(block_size <= seg_free_list[listNum].max){
            break;
        }
    }
    //printf("List num is %d\n", listNum);
    sf_free_header* curr = seg_free_list[listNum].head;
    //printf("Address of header is %p\n", header);
    //printf("Address of head is %p\n", &((*curr).header));
    while(header != &((*curr).header)){
        curr = (*curr).next;
    }

    if((*curr).prev == NULL){ // If the block to be freed is the head of the list
        if((*curr).next == NULL){ // If the block to be freed is the only block in the free list
            //printf("Only one block in free list\n");
            seg_free_list[listNum].head = NULL;
        }
        else{
            //printf("Block is the head of the free list and is followed by another free block\n");
            seg_free_list[listNum].head = (*curr).next;
            (*(*curr).next).prev = NULL;
        }
    }
    else{ // The block is between two other free blocks in the free list
        //printf("Block is between two other free blocks\n");
        sf_free_header* prev = (*curr).prev;
        sf_free_header* next = (*curr).next;
        (*prev).next = next;
        (*next).prev = prev;
    }
}

sf_header* returnHeaderAddress(sf_footer* footer){
    sf_header* ret = (sf_header*)((void*)footer + SF_HEADER_SIZE/8 - (*footer).block_size*16);
    return ret;
}

sf_footer* returnFooterAddress(sf_header* header){
    sf_footer* ret = (sf_footer*)((void*)header + (*header).block_size*16 - SF_FOOTER_SIZE/8);
    return ret;
}

void setFooter(sf_header* header, size_t requested_size){ // Sets the footer equal to the header with the addition of the requested size bits
    sf_footer* footer = returnFooterAddress(header);
    (*footer).requested_size = requested_size;
    (*footer).block_size = (*header).block_size;
    (*footer).two_zeroes = 0;
    (*footer).padded = (*header).padded;
    (*footer).allocated = (*header).allocated;
}

void setBlock(sf_header* header, uint64_t requested_size, uint64_t block_size, uint64_t padded, uint64_t allocated){
    (*header).unused = 0;
    (*header).block_size = block_size;
    (*header).two_zeroes = 0;
    (*header).padded = padded;
    (*header).allocated = allocated;
    setFooter(header, requested_size);
}

int isNull(){
    int freeListNull = 1; // 1 means that all the heads of the freelists are NULL
    int i;
    for(i = 0; i < listLength; i++){
        if(seg_free_list[i].head != NULL){
            freeListNull = 0;
            break;
        }
    }
    return freeListNull;
}

void *allocateMemory(){ // Returns start of new page or NULL if failure occurs.
    //printf("Allocating memory\n");
    if(get_heap_end() - get_heap_start() == 0x4000){
        sf_errno = ENOMEM;
        return NULL;
    }
    //printf("Using sf_sbrk\n");
    void* breakpoint = sf_sbrk();
    if(sf_errno == ENOMEM){
        return NULL;
    }
    //printf("Breakpoint is %p\n", breakpoint);
    //printf("Heap start is %p\n", get_heap_start());
    //printf("Heap end is %p\n", get_heap_end());
    //printf("Snapshot of free list before allocating memory\n");
    //sf_snapshot();

    sf_header* header;
    if(isNull()){ // If the heads of all the freelists are NULL
        //printf("Heads of all freelists are null\n");
        header = (sf_header*)(breakpoint);
        setBlock(header, 0, PAGE_SZ/16, 0, 0);
        //printHeader(header);
    }

    else{
        //printf("Heads of all freelists are not null\n");
        sf_footer* prev_footer = (sf_footer*)(breakpoint - SF_FOOTER_SIZE/8);
        if((*prev_footer).allocated == 0){ // If the previous memory block is free
            header = returnHeaderAddress(prev_footer);
            setBlock(header, 0, (*prev_footer).block_size + PAGE_SZ/16, 0, 0);
            //printf("Removing from free list\n");
            removeFromFreeList(header);
            //printf("Successfully removed from free list\n");
        }
    }
    addToFreeList(header);
    //sf_blockprint(header);
    //printf("Snapshot of free list after allocating memory\n");
    //sf_snapshot();
    return breakpoint;
}

void splitBlock(sf_header* header, size_t size){ // Assumes that the header is for a block that is not in the free list and
                                                 // that is >= (size + header + footer)
    //printf("Splitting block function\n");
    uint64_t old_block_size = (*header).block_size * 16;
    //uint64_t payload_size = old_block_size - SF_HEADER_SIZE/8 - SF_FOOTER_SIZE/8;
    uint64_t block_size = size + SF_HEADER_SIZE/8 + SF_FOOTER_SIZE/8;
    //size_t alignedRequestSize = size;
    //printf("Size is %lu\n", size);
    if(block_size % 16 != 0){
        //printf("Needs alignment\n");
        block_size += (16 - (size % 16)); // Align the size to be a multiple of 16
    }
    //printf("Old block size is %lu\n", old_block_size);
    //printf("Block size is %lu\n", block_size);
    if(old_block_size - block_size >= 32){
        //printf("Block can be split\n");
        sf_footer* footer = returnFooterAddress(header);
        sf_header* next_header = (sf_header*)((void*)header + block_size);
        setBlock(next_header, 0, (old_block_size - block_size)/16, 0, 0);
        //sf_header* following_header = (sf_header*)((void*)next_header + SF_HEADER_SIZE/8 + (*next_header).block_size*16 + SF_FOOTER_SIZE/8);
        sf_header* following_header = (sf_header*)((void*)footer + SF_FOOTER_SIZE/8);
        //sf_blockprint(next_header);
        //sf_blockprint(following_header);
        if((*following_header).allocated == 0 && (*following_header).block_size != 0){ // Next block can be coalesced
            //printf("Coalesce with next block\n");
            uint64_t new_block_size = (*next_header).block_size*16 + (*following_header).block_size*16;
            //sf_blockprint(following_header);
            //printf("New block size is %lu\n", new_block_size);
            removeFromFreeList(following_header);
            setBlock(next_header, 0, new_block_size/16, 0, 0);
            //printf("Current header\n");
            //sf_blockprint(next_header);
        }
        //printHeader(next_header);
        addToFreeList(next_header);
        //printf("Block is split\n");
        //sf_blockprint(next_header);
        if((block_size - size) % 16 == 0){ // If the size is a multiple of 16 and equals the given block size
            //printf("No padding required\n");
            setBlock(header, size, block_size/16, 0, 1);
        }
        else{
            //printf("Padding required\n");
            setBlock(header, size, block_size/16, 1, 1);
        }
    }
    else{
        //setBlock(header, 0, (*header).block_size, 0, 0);
        if((block_size - size) % 16 == 0 && block_size == old_block_size){ // If the size is a multiple of 16 and equals the given block size
            //printf("No padding required\n");
            setBlock(header, size, (*header).block_size, 0, 1);
        }
        else{
            //printf("Padding required\n");
            setBlock(header, size, (*header).block_size, 1, 1);
        }
    }
    //printHeader(header);
    //sf_blockprint(header);
}

void *freeListTraversal(int listNum, size_t size){
    if(isNull()){
        void* breakpoint = allocateMemory();
        if(breakpoint == NULL){ // Checks if there was an error when allocating memory
            return NULL;
        }
    }

    if(seg_free_list[listNum].head == NULL){
        return NULL;
    }

    sf_free_header* currFree = seg_free_list[listNum].head;
    //sf_snapshot();
    //printf("currFree set\n");
    //printHeader(&(*currFree).header);
    if((*currFree).header.block_size*16 >= size + SF_HEADER_SIZE/8 + SF_FOOTER_SIZE/8){
        //printf("First block in list %d is valid\n", listNum);
        sf_header* header = (&(*currFree).header);
        if((*currFree).next != NULL){
            //printf("Next is not null\n");
            sf_free_header newHead = *((*currFree).next);
            newHead.prev = NULL;
            seg_free_list[listNum].head = &newHead;
        }
        else{
            //printf("Next is null\n");
            seg_free_list[listNum].head = NULL;
        }
        //printHeader(header);
        //printFooter(footer);
        //sf_blockprint(header);
        splitBlock(header, size);
        void *ret = ((void*)header + SF_HEADER_SIZE/8);
        return ret;
    }
    //int i = 1;
    while((*currFree).next != NULL){
        //printf("On block %d\n", i);
        currFree = (*currFree).next;
        //printHeader(&((*currFree).header));
        if((*currFree).header.block_size*16 >= size){
            sf_header header = (*currFree).header;
            if((*currFree).next == NULL){
                /*sf_free_header prev = *((*currFree).prev);
                prev.next = NULL;*/
                (*((*currFree).prev)).next = NULL;
            }
            else{
                //printf("Block %d is a valid block\n", i);
                sf_free_header prev = *((*currFree).prev);
                sf_free_header next = *((*currFree).next);
                next.prev = &prev;
                prev.next = &next;
            }
            void *ret = ((void*)(&header) + SF_HEADER_SIZE/8);
            return ret;
        }
    }
    return NULL;
}

int sf_errno = 0;

void *sf_malloc(size_t size) {
    //printf("The length of the free list is %d\n", listLength);
    //size += SF_HEADER_SIZE/8 + SF_FOOTER_SIZE/8;
    if(size == 0 || size > 4 * PAGE_SZ){ // Check if the desired size is valid
        //printf("Error detected\n");
        sf_errno = EINVAL;
        return NULL;
    }
    void* ret = NULL;

    int i;
    for(i = 0; i < listLength; i++){
        if(size <= seg_free_list[i].max){
            //printf("Traversing through list %d\n", i);
            ret = freeListTraversal(i, size);
            if(sf_errno == ENOMEM){
                //printf("No memory\n");
                return NULL;
            }
            else if(ret != NULL){
                //printf("Free memory block found\n");
                return ret;
            }
        }
    }

    //printf("No valid memory block found\n");
    // A valid memory block was not found
    allocateMemory();
    return sf_malloc(size);
}

void *sf_realloc(void *ptr, size_t size) {
    void *heap_start = get_heap_start();
    void *heap_end = get_heap_end();
    sf_header* header = (sf_header*)(ptr - SF_HEADER_SIZE/8); // Starting address of the header
    //sf_blockprint(header);
    if(ptr == NULL || ptr < heap_start || ptr > heap_end || (*header).allocated == 0){
        //printf("Realloc error 1\n");
        sf_errno = EINVAL;
        abort();
    }
    sf_footer* footer = returnFooterAddress(header); // Starting address of the footer
    //printHeader(header);
    //printFooter(footer);
    if((*footer).requested_size + SF_HEADER_SIZE/8 + SF_FOOTER_SIZE/8 != (*footer).block_size*16 && (*footer).padded == 0){
        //printf("Realloc error 2\n");
        sf_errno = EINVAL;
        abort();
    }
    if((*header).padded != (*footer).padded || (*header).allocated != (*footer).allocated){
        //printf("Realloc error 3\n");
        sf_errno = EINVAL;
        abort();
    }
    if(size == 0){
        sf_free(ptr);
        return NULL;
    }
    uint64_t old_block_size = (*header).block_size * 16;
    uint64_t new_block_size = size + SF_HEADER_SIZE/8 + SF_FOOTER_SIZE/8;
    if(new_block_size % 16 != 0){
        new_block_size += (16 - (new_block_size % 16));
    }
    if(old_block_size == new_block_size){
        return ptr;
    }

    if(old_block_size < new_block_size){ // Reallocating to a larger block size
        //printf("Reallocating to larger size\n");
        void* ret = sf_malloc(size);
        if(ret == NULL){
            return NULL;
        }
        //printf("Used malloc\n");
        sf_header* new_header = (sf_header*)(ret - SF_HEADER_SIZE/8);
        //sf_snapshot();
        if(new_block_size % 16 == 0){
            //printf("Realloc no padding\n");
            setBlock(new_header, size, new_block_size/16, 0, 1);
        }
        else{
            //printf("Realloc with padding\n");
            setBlock(new_header, size, new_block_size/16, 1, 1);
        }
        //printf("Memcpy\n");
        //sf_snapshot();
        memcpy((void*)new_header + SF_HEADER_SIZE/8, (void*)header + SF_HEADER_SIZE/8, (*header).block_size*16 - SF_HEADER_SIZE/8 - SF_FOOTER_SIZE/8);
        //printf("Freeing previous block\n");
        sf_free(ptr);
        //sf_blockprint(new_header);
        //sf_snapshot();
        return (void*)new_header + SF_HEADER_SIZE/8;
    }
    else{ // Reallocating to a smaller block size
        //printf("Reallocating to smaller size\n");
        splitBlock(header, size);
        if(size % 16 == 0){
            //printf("Realloc no padding\n");
            setBlock(header, size, (*header).block_size, 0, 1);
        }
        else{
            //printf("Realloc with padding\n");
            setBlock(header, size, (*header).block_size, 1, 1);
        }
        //sf_blockprint(header);
        return (void*)header + SF_HEADER_SIZE/8;
    }

	return NULL;
}

void sf_free(void *ptr) {
    // Invalid Pointer Check
    //printf("Freeing a block\n");
    void *heap_start = get_heap_start();
    void *heap_end = get_heap_end();
    //printf("Heap start is %p\n", heap_start);
    //printf("Heap end is %p\n", heap_end);
    sf_header* header = (sf_header*)(ptr - SF_HEADER_SIZE/8); // Starting address of the header
    //sf_blockprint(header);

    if(ptr == NULL || ptr < heap_start || ptr > heap_end || (*header).allocated == 0){
        //printf("Free error 1\n");
        abort();
    }
    sf_footer* footer = returnFooterAddress(header); // Starting address of the footer
    //printHeader(header);
    //printFooter(footer);
    if((*footer).requested_size + SF_HEADER_SIZE/8 + SF_FOOTER_SIZE/8 != (*footer).block_size*16 && (*footer).padded == 0){
        //printf("Free error 2\n");
        abort();
    }
    if((*header).padded != (*footer).padded || (*header).allocated != (*footer).allocated){
        //printf("Free error 3\n");
        abort();
    }

    //printf("Pointer has no errors\n");
    // Checking if the next block can be coalesced
    sf_header* header_next =  (sf_header*)((void*)footer + SF_FOOTER_SIZE/8); // Starting address of the next header;
    //printHeader(&header_next);
    //printf("Next block's header\n");
    //sf_blockprint(header_next);
    if((*header_next).allocated == 0 && (*header_next).block_size != 0){ // Next block can be coalesced
        //printf("Coalesce with next block\n");
        uint64_t new_block_size = (*header).block_size*16 + ((*header_next).block_size*16);
        //printf("New block size is %lu\n", new_block_size);
        removeFromFreeList(header_next);
        setBlock(header, 0, new_block_size/16, 0, 0);
        //printf("Current header\n");
        //sf_blockprint(header);
    }
    else{
        setBlock(header, 0, (*header).block_size, 0, 0);
    }
    //uint64_t block_size = (*header).block_size*16;

    addToFreeList(header);
	return;
}
