#include <criterion/criterion.h>
#include <errno.h>
#include <signal.h>
#include "sfmm.h"


int find_list_index_from_size(int sz) {
	if (sz >= LIST_1_MIN && sz <= LIST_1_MAX) return 0;
	else if (sz >= LIST_2_MIN && sz <= LIST_2_MAX) return 1;
	else if (sz >= LIST_3_MIN && sz <= LIST_3_MAX) return 2;
	else return 3;
}

Test(sf_memsuite_student, Malloc_an_Integer_check_freelist, .init = sf_mem_init, .fini = sf_mem_fini) {
    sf_errno = 0;
    int *x = sf_malloc(sizeof(int));

    cr_assert_not_null(x);

    *x = 4;

    cr_assert(*x == 4, "sf_malloc failed to give proper space for an int!");

    sf_header *header = (sf_header*)((char*)x - 8);

    // There should be one block of size 4064 in list 3
    free_list *fl = &seg_free_list[find_list_index_from_size(PAGE_SZ - (header->block_size << 4))];

    cr_assert_not_null(fl, "Free list is null");

    cr_assert_not_null(fl->head, "No block in expected free list!");
    cr_assert_null(fl->head->next, "Found more blocks than expected!");
    cr_assert(fl->head->header.block_size << 4 == 4064);
    cr_assert(fl->head->header.allocated == 0);
    cr_assert(sf_errno == 0, "sf_errno is not zero!");
    cr_assert(get_heap_start() + PAGE_SZ == get_heap_end(), "Allocated more than necessary!");
}

Test(sf_memsuite_student, Malloc_over_four_pages, .init = sf_mem_init, .fini = sf_mem_fini) {
    sf_errno = 0;
    void *x = sf_malloc(PAGE_SZ << 2);

    cr_assert_null(x, "x is not NULL!");
    cr_assert(sf_errno == ENOMEM, "sf_errno is not ENOMEM!");
}

Test(sf_memsuite_student, free_double_free, .init = sf_mem_init, .fini = sf_mem_fini, .signal = SIGABRT) {
    sf_errno = 0;
    void *x = sf_malloc(sizeof(int));
    sf_free(x);
    sf_free(x);
}

Test(sf_memsuite_student, free_no_coalesce, .init = sf_mem_init, .fini = sf_mem_fini) {
    sf_errno = 0;
    /* void *x = */ sf_malloc(sizeof(long));
    void *y = sf_malloc(sizeof(double) * 10);
    /* void *z = */ sf_malloc(sizeof(char));

    sf_free(y);

    free_list *fl = &seg_free_list[find_list_index_from_size(96)];

    cr_assert_not_null(fl->head, "No block in expected free list");
    cr_assert_null(fl->head->next, "Found more blocks than expected!");
    cr_assert(fl->head->header.block_size << 4 == 96);
    cr_assert(fl->head->header.allocated == 0);
    cr_assert(sf_errno == 0, "sf_errno is not zero!");
}

Test(sf_memsuite_student, free_coalesce, .init = sf_mem_init, .fini = sf_mem_fini) {
    sf_errno = 0;
    /* void *w = */ sf_malloc(sizeof(long));
    void *x = sf_malloc(sizeof(double) * 11);
    void *y = sf_malloc(sizeof(char));
    /* void *z = */ sf_malloc(sizeof(int));

    sf_free(y);
    sf_free(x);

    free_list *fl_y = &seg_free_list[find_list_index_from_size(32)];
    free_list *fl_x = &seg_free_list[find_list_index_from_size(144)];

    cr_assert_null(fl_y->head, "Unexpected block in list!");
    cr_assert_not_null(fl_x->head, "No block in expected free list");
    cr_assert_null(fl_x->head->next, "Found more blocks than expected!");
    cr_assert(fl_x->head->header.block_size << 4 == 144);
    cr_assert(fl_x->head->header.allocated == 0);
    cr_assert(sf_errno == 0, "sf_errno is not zero!");
}

Test(sf_memsuite_student, freelist, .init = sf_mem_init, .fini = sf_mem_fini) {
    /* void *u = */ sf_malloc(1);          //32
    void *v = sf_malloc(LIST_1_MIN); //48
    void *w = sf_malloc(LIST_2_MIN); //160
    void *x = sf_malloc(LIST_3_MIN); //544
    void *y = sf_malloc(LIST_4_MIN); //2080
    /* void *z = */ sf_malloc(1); // 32

    int allocated_block_size[4] = {48, 160, 544, 2080};

    sf_free(v);
    sf_free(w);
    sf_free(x);
    sf_free(y);

    // First block in each list should be the most recently freed block
    for (int i = 0; i < FREE_LIST_COUNT; i++) {
        sf_free_header *fh = (sf_free_header *)(seg_free_list[i].head);
        cr_assert_not_null(fh, "list %d is NULL!", i);
        cr_assert(fh->header.block_size << 4 == allocated_block_size[i], "Unexpected free block size!");
        cr_assert(fh->header.allocated == 0, "Allocated bit is set!");
    }

    // There should be one free block in each list, 2 blocks in list 3 of size 544 and 1232
    for (int i = 0; i < FREE_LIST_COUNT; i++) {
        sf_free_header *fh = (sf_free_header *)(seg_free_list[i].head);
        if (i != 2)
            cr_assert_null(fh->next, "More than 1 block in freelist [%d]!", i);
        else {
            cr_assert_not_null(fh->next, "Less than 2 blocks in freelist [%d]!", i);
            cr_assert_null(fh->next->next, "More than 2 blocks in freelist [%d]!", i);
        }
    }
}

Test(sf_memsuite_student, realloc_larger_block, .init = sf_mem_init, .fini = sf_mem_fini) {
    void *x = sf_malloc(sizeof(int));
    /* void *y = */ sf_malloc(10);
    x = sf_realloc(x, sizeof(int) * 10);

    free_list *fl = &seg_free_list[find_list_index_from_size(32)];

    cr_assert_not_null(x, "x is NULL!");
    cr_assert_not_null(fl->head, "No block in expected free list!");
    cr_assert(fl->head->header.block_size << 4 == 32, "Free Block size not what was expected!");

    sf_header *header = (sf_header*)((char*)x - 8);
    cr_assert(header->block_size << 4 == 64, "Realloc'ed block size not what was expected!");
    cr_assert(header->allocated == 1, "Allocated bit is not set!");
}

Test(sf_memsuite_student, realloc_smaller_block_splinter, .init = sf_mem_init, .fini = sf_mem_fini) {
    void *x = sf_malloc(sizeof(int) * 8);
    void *y = sf_realloc(x, sizeof(char));

    cr_assert_not_null(y, "y is NULL!");
    cr_assert(x == y, "Payload addresses are different!");

    sf_header *header = (sf_header*)((char*)y - 8);
    cr_assert(header->allocated == 1, "Allocated bit is not set!");
    cr_assert(header->block_size << 4 == 48, "Block size not what was expected!");

    free_list *fl = &seg_free_list[find_list_index_from_size(4048)];

    // There should be only one free block of size 4048 in list 3
    cr_assert_not_null(fl->head, "No block in expected free list!");
    cr_assert(fl->head->header.allocated == 0, "Allocated bit is set!");
    cr_assert(fl->head->header.block_size << 4 == 4048, "Free block size not what was expected!");
}

Test(sf_memsuite_student, realloc_smaller_block_free_block, .init = sf_mem_init, .fini = sf_mem_fini) {
    void *x = sf_malloc(sizeof(double) * 8);
    void *y = sf_realloc(x, sizeof(int));

    cr_assert_not_null(y, "y is NULL!");

    sf_header *header = (sf_header*)((char*)y - 8);
    cr_assert(header->block_size << 4 == 32, "Realloc'ed block size not what was expected!");
    cr_assert(header->allocated == 1, "Allocated bit is not set!");


    // After realloc'ing x, we can return a block of size 48 to the freelist.
    // This block will coalesce with the block of size 4016.
    free_list *fl = &seg_free_list[find_list_index_from_size(4064)];

    cr_assert_not_null(fl->head, "No block in expected free list!");
    cr_assert(fl->head->header.allocated == 0, "Allocated bit is set!");
    cr_assert(fl->head->header.block_size << 4 == 4064, "Free block size not what was expected!");
}


//############################################
//STUDENT UNIT TESTS SHOULD BE WRITTEN BELOW
//DO NOT DELETE THESE COMMENTS
//############################################

// Test that checks if the footer and the header contain the same values for freed and allocated blocks
// and if all the values for the header and footer are correct.
Test(sf_memsuite_student, check_header_and_footer, .init = sf_mem_init, .fini = sf_mem_fini) {
	sf_errno = 0;
	sf_header* x = (sf_header*)(sf_malloc(sizeof(char)) - SF_HEADER_SIZE/8);
	cr_assert((*x).allocated == 1, "Allocated bit is wrong for malloc of size char.");
	cr_assert((*x).padded == 1, "Padded bit is wrong for malloc of size char.");
	cr_assert((*x).two_zeroes == 0, "Two zeroes bits are wrong for malloc of size char.");
	cr_assert((*x).block_size * 16 == 32, "Block_size is wrong for malloc of size char.");
	sf_footer* y = (sf_footer*)((void*)x + (*x).block_size*16 - SF_FOOTER_SIZE/8);
	cr_assert((*y).requested_size == sizeof(char), "Requested size bits are wrong for malloc of size char.");
	cr_assert((*x).allocated == (*y).allocated, "Header and footer are not equal for malloc of size char.");
	cr_assert((*x).padded == (*y).padded, "Header and footer are not equal for malloc of size char.");
	cr_assert((*x).two_zeroes == (*y).two_zeroes, "Header and footer are not equal for malloc of size char.");
	cr_assert((*x).block_size == (*y).block_size, "Header and footer are not equal for malloc of size char.");

	x = (sf_header*)(sf_malloc(sizeof(int)) - SF_HEADER_SIZE/8);
	cr_assert((*x).allocated == 1, "Allocated bit is wrong for malloc of size int.");
	cr_assert((*x).padded == 1, "Padded bit is wrong for malloc of size int.");
	cr_assert((*x).two_zeroes == 0, "Two zeroes bits are wrong for malloc of size int.");
	cr_assert((*x).block_size * 16 == 32, "Block_size is wrong for malloc of size int.");
	y = (sf_footer*)((void*)x + (*x).block_size*16 - SF_FOOTER_SIZE/8);
	cr_assert((*y).requested_size == sizeof(int), "Requested size bits are wrong for malloc of size int.");
	cr_assert((*x).allocated == (*y).allocated, "Header and footer are not equal for malloc of size int.");
	cr_assert((*x).padded == (*y).padded, "Header and footer are not equal for malloc of size int.");
	cr_assert((*x).two_zeroes == (*y).two_zeroes, "Header and footer are not equal for malloc of size int.");
	cr_assert((*x).block_size == (*y).block_size, "Header and footer are not equal for malloc of size int.");

	x = (sf_header*)(sf_malloc(sizeof(long)) - SF_HEADER_SIZE/8);
	cr_assert((*x).allocated == 1, "Allocated bit is wrong for malloc of size long.");
	cr_assert((*x).padded == 1, "Padded bit is wrong for malloc of size long.");
	cr_assert((*x).two_zeroes == 0, "Two zeroes bits are wrong for malloc of size long.");
	cr_assert((*x).block_size * 16 == 32, "Block_size is wrong for malloc of size long.");
	y = (sf_footer*)((void*)x + (*x).block_size*16 - SF_FOOTER_SIZE/8);
	cr_assert((*y).requested_size == sizeof(long), "Requested size bits are wrong for malloc of size long.");
	cr_assert((*x).allocated == (*y).allocated, "Header and footer are not equal for malloc of size long.");
	cr_assert((*x).padded == (*y).padded, "Header and footer are not equal for malloc of size long.");
	cr_assert((*x).two_zeroes == (*y).two_zeroes, "Header and footer are not equal for malloc of size long.");
	cr_assert((*x).block_size == (*y).block_size, "Header and footer are not equal for malloc of size long.");

	x = (sf_header*)(sf_malloc(sizeof(double)) - SF_HEADER_SIZE/8);
	cr_assert((*x).allocated == 1, "Allocated bit is wrong for malloc of size double.");
	cr_assert((*x).padded == 1, "Padded bit is wrong for malloc of size double.");
	cr_assert((*x).two_zeroes == 0, "Two zeroes bits are wrong for malloc of size double.");
	cr_assert((*x).block_size * 16 == 32, "Block_size is wrong for malloc of size double.");
	y = (sf_footer*)((void*)x + (*x).block_size*16 - SF_FOOTER_SIZE/8);
	cr_assert((*y).requested_size == sizeof(double), "Requested size bits are wrong for malloc of size double.");
	cr_assert((*x).allocated == (*y).allocated, "Header and footer are not equal for malloc of size double.");
	cr_assert((*x).padded == (*y).padded, "Header and footer are not equal for malloc of size double.");
	cr_assert((*x).two_zeroes == (*y).two_zeroes, "Header and footer are not equal for malloc of size double.");
	cr_assert((*x).block_size == (*y).block_size, "Header and footer are not equal for malloc of size double.");

	x = (sf_header*)(sf_malloc(sizeof(double)*2) - SF_HEADER_SIZE/8);
	cr_assert((*x).allocated == 1, "Allocated bit is wrong for malloc of size double * 2.");
	cr_assert((*x).padded == 0, "Padded bit is wrong for malloc of size double * 2.");
	cr_assert((*x).two_zeroes == 0, "Two zeroes bits are wrong for malloc of size double * 2.");
	cr_assert((*x).block_size * 16 == 32, "Block_size is wrong for malloc of size double * 2.");
	y = (sf_footer*)((void*)x + (*x).block_size*16 - SF_FOOTER_SIZE/8);
	cr_assert((*y).requested_size == sizeof(double) * 2, "Requested size bits are wrong for malloc of size double * 2.");
	cr_assert((*x).allocated == (*y).allocated, "Header and footer are not equal for malloc of size double * 2.");
	cr_assert((*x).padded == (*y).padded, "Header and footer are not equal for malloc of size double * 2.");
	cr_assert((*x).two_zeroes == (*y).two_zeroes, "Header and footer are not equal for malloc of size double * 2.");
	cr_assert((*x).block_size == (*y).block_size, "Header and footer are not equal for malloc of size double * 2.");

	x = (sf_header*)(sf_malloc(sizeof(char) * 16) - SF_HEADER_SIZE/8);
	cr_assert((*x).allocated == 1, "Allocated bit is wrong for malloc of size char * 16.");
	cr_assert((*x).padded == 0, "Padded bit is wrong for malloc of size char * 16.");
	cr_assert((*x).two_zeroes == 0, "Two zeroes bits are wrong for malloc of size char * 16.");
	cr_assert((*x).block_size * 16 == 32, "Block_size is wrong for malloc of size char * 16.");
	y = (sf_footer*)((void*)x + (*x).block_size*16 - SF_FOOTER_SIZE/8);
	cr_assert((*y).requested_size == sizeof(char) * 16, "Requested size bits are wrong for malloc of size char * 16.");
	cr_assert((*x).allocated == (*y).allocated, "Header and footer are not equal for malloc of size char * 16.");
	cr_assert((*x).padded == (*y).padded, "Header and footer are not equal for malloc of size char * 16.");
	cr_assert((*x).two_zeroes == (*y).two_zeroes, "Header and footer are not equal for malloc of size char * 16.");
	cr_assert((*x).block_size == (*y).block_size, "Header and footer are not equal for malloc of size char * 16.");

	x = (sf_header*)(sf_malloc(sizeof(int) * 16) - SF_HEADER_SIZE/8);
	cr_assert((*x).allocated == 1, "Allocated bit is wrong for malloc of size int * 16.");
	cr_assert((*x).padded == 0, "Padded bit is wrong for malloc of size int * 16.");
	cr_assert((*x).two_zeroes == 0, "Two zeroes bits are wrong for malloc of size int * 16.");
	cr_assert((*x).block_size * 16 == 80, "Block_size is wrong for malloc of size int * 16.");
	y = (sf_footer*)((void*)x + (*x).block_size*16 - SF_FOOTER_SIZE/8);
	cr_assert((*y).requested_size == sizeof(int) * 16, "Requested size bits are wrong for malloc of size int * 16.");
	cr_assert((*x).allocated == (*y).allocated, "Header and footer are not equal for malloc of size int * 16.");
	cr_assert((*x).padded == (*y).padded, "Header and footer are not equal for malloc of size int * 16.");
	cr_assert((*x).two_zeroes == (*y).two_zeroes, "Header and footer are not equal for malloc of size int * 16.");
	cr_assert((*x).block_size == (*y).block_size, "Header and footer are not equal for malloc of size int * 16.");

	x = (sf_header*)(sf_malloc(sizeof(long) * 16) - SF_HEADER_SIZE/8);
	cr_assert((*x).allocated == 1, "Allocated bit is wrong for malloc of size long * 16.");
	cr_assert((*x).padded == 0, "Padded bit is wrong for malloc of size long * 16.");
	cr_assert((*x).two_zeroes == 0, "Two zeroes bits are wrong for malloc of size long * 16.");
	cr_assert((*x).block_size * 16 == 144, "Block_size is wrong for malloc of size long * 16.");
	y = (sf_footer*)((void*)x + (*x).block_size*16 - SF_FOOTER_SIZE/8);
	cr_assert((*y).requested_size == sizeof(long) * 16, "Requested size bits are wrong for malloc of size long * 16.");
	cr_assert((*x).allocated == (*y).allocated, "Header and footer are not equal for malloc of size long * 16.");
	cr_assert((*x).padded == (*y).padded, "Header and footer are not equal for malloc of size long * 16.");
	cr_assert((*x).two_zeroes == (*y).two_zeroes, "Header and footer are not equal for malloc of size long * 16.");
	cr_assert((*x).block_size == (*y).block_size, "Header and footer are not equal for malloc of size long * 16.");

	int z = sizeof(char) + sizeof(int) + sizeof(long) + sizeof(double) + sizeof(double)*2 + sizeof(char)*16 + sizeof(int)*16 + sizeof(long)*16;
	int index = find_list_index_from_size(PAGE_SZ - z);
	x = &((*seg_free_list[index].head).header);
	cr_assert((*x).allocated == 0, "Allocated bit is wrong for free block");
	cr_assert((*x).two_zeroes == 0, "Two zeroes bits are wrong for free block");
	cr_assert((*x).block_size * 16 == PAGE_SZ - 416, "Block_size is wrong for free block");
	y = (sf_footer*)((void*)x + (*x).block_size*16 - SF_FOOTER_SIZE/8);
	cr_assert((*x).allocated == (*y).allocated, "Header and footer are not equal for free_block.");
	cr_assert((*x).two_zeroes == (*y).two_zeroes, "Header and footer are not equal for free block.");
	cr_assert((*x).block_size == (*y).block_size, "Header and footer are not equal for free block.");

}

// Test that checks for enomem and einval errors for malloc and realloc
Test(sf_memsuite_student, check_malloc_realloc_error_values, .init = sf_mem_init, .fini = sf_mem_fini) {
	sf_errno = 0;
	void* ret = sf_malloc(0);
	cr_assert_null(ret, "Return value is not null.");
	cr_assert(sf_errno == EINVAL, "sf_errno is not EINVAL.");
	int index = find_list_index_from_size(PAGE_SZ);
	sf_free_header* x = seg_free_list[index].head;
	cr_assert(x == NULL || (*x).header.block_size == PAGE_SZ, "Memory allocated despite invalid input.");

	sf_errno = 0;
	ret = sf_malloc(PAGE_SZ * 4 + 1);
	cr_assert_null(ret, "Return value is not null.");
	cr_assert(sf_errno == EINVAL, "sf_errno is not EINVAL.");
	cr_assert(x == NULL || (*x).header.block_size == PAGE_SZ, "Memory allocated despite invalid input.");

	void* memory = sf_malloc(sizeof(char));

	sf_errno = 0;
	ret = sf_realloc(memory, PAGE_SZ * 4 + 1);
	cr_assert_null(ret, "Return value is not null.");
	cr_assert(sf_errno == EINVAL, "sf_errno is not EINVAL.");
	cr_assert(x == NULL || (*x).header.block_size == PAGE_SZ - 32, "Memory allocated despite invalid input.");

}

// Tests that checks for invalid inputs for free
// Before heap_start
Test(sf_memsuite_student, check_invalid_input_free_1, .init = sf_mem_init, .fini = sf_mem_fini, .signal = SIGABRT) {
	sf_errno = 0;
    void *x = sf_malloc(sizeof(int));
    x -= SF_HEADER_SIZE/8 - 1;
    sf_free(x);
}

// After heap_end
Test(sf_memsuite_student, check_invalid_input_free_2, .init = sf_mem_init, .fini = sf_mem_fini, .signal = SIGABRT) {
	sf_errno = 0;
    void *x = sf_malloc(sizeof(int));
    x += PAGE_SZ;
    sf_free(x);
}

// Allocated bit in header is 0
Test(sf_memsuite_student, check_invalid_input_free_3, .init = sf_mem_init, .fini = sf_mem_fini, .signal = SIGABRT) {
	sf_errno = 0;
    void *x = sf_malloc(sizeof(int));
    sf_header* y = (sf_header*)(x - SF_HEADER_SIZE/8);
    (*y).allocated = 0;
    sf_free(x);
}

// Allocated bit in footer is 0
Test(sf_memsuite_student, check_invalid_input_free_4, .init = sf_mem_init, .fini = sf_mem_fini, .signal = SIGABRT) {
	sf_errno = 0;
    void *x = sf_malloc(sizeof(int));
    sf_header* y = (sf_header*)(x - SF_HEADER_SIZE/8);
    sf_footer* z = (sf_footer*)((void*)y + (*y).block_size*16 - SF_FOOTER_SIZE/8);
  	(*z).allocated = 0;
    sf_free(x);
}

// Requested_size, block_size, and padded bits don't make sense: padded bit should be 1.
Test(sf_memsuite_student, check_invalid_input_free_5, .init = sf_mem_init, .fini = sf_mem_fini, .signal = SIGABRT) {
	sf_errno = 0;
    void *x = sf_malloc(sizeof(int));
    sf_header* y = (sf_header*)(x - SF_HEADER_SIZE/8);
  	(*y).padded = 0;
    sf_free(x);
}

// Requested_size, block_size, and padded bits don't make sense: padded bit should be 0.
Test(sf_memsuite_student, check_invalid_input_free_6, .init = sf_mem_init, .fini = sf_mem_fini, .signal = SIGABRT) {
	sf_errno = 0;
    void *x = sf_malloc(sizeof(double)*2);
    sf_header* y = (sf_header*)(x - SF_HEADER_SIZE/8);
  	(*y).padded = 1;
    sf_free(x);
}

// Test that checks realloc for all the different types of lists
Test(sf_memsuite_student, max_malloc_realloc_free, .init = sf_mem_init, .fini = sf_mem_fini) {
	void* memory = sf_malloc(PAGE_SZ * 4 -16);
	cr_assert_null(seg_free_list[0].head);
	cr_assert_null(seg_free_list[1].head);
	cr_assert_null(seg_free_list[2].head);
	cr_assert_null(seg_free_list[3].head);
	memory = sf_realloc(memory, PAGE_SZ * 4 - LIST_1_MAX);
	cr_assert_not_null(seg_free_list[0].head);
	cr_assert_null(seg_free_list[1].head);
	cr_assert_null(seg_free_list[2].head);
	cr_assert_null(seg_free_list[3].head);
	memory = sf_realloc(memory, PAGE_SZ * 4 - LIST_2_MAX);
	cr_assert_null(seg_free_list[0].head);
	cr_assert_not_null(seg_free_list[1].head);
	cr_assert_null(seg_free_list[2].head);
	cr_assert_null(seg_free_list[3].head);
	memory = sf_realloc(memory, PAGE_SZ * 4 - LIST_3_MAX);
	cr_assert_null(seg_free_list[0].head);
	cr_assert_null(seg_free_list[1].head);
	cr_assert_not_null(seg_free_list[2].head);
	cr_assert_null(seg_free_list[3].head);
	memory = sf_realloc(memory, PAGE_SZ * 4 - LIST_4_MIN - LIST_1_MIN);
	cr_assert_null(seg_free_list[0].head);
	cr_assert_null(seg_free_list[1].head);
	cr_assert_null(seg_free_list[2].head);
	cr_assert_not_null(seg_free_list[3].head);
	memory = sf_realloc(memory, 0);
	cr_assert_null(seg_free_list[0].head);
	cr_assert_null(seg_free_list[1].head);
	cr_assert_null(seg_free_list[2].head);
	cr_assert_not_null(seg_free_list[3].head);
	sf_free_header* x = seg_free_list[3].head;
	cr_assert((*x).header.block_size*16 == PAGE_SZ * 4, "Free block size was not what was expected.");
}

// Test that traverses through free lists to see if both next and prev are set correctly
Test(sf_memsuite_student, check_malloc_split, .init = sf_mem_init, .fini = sf_mem_fini) {
	void* one = sf_malloc(sizeof(char));
	sf_malloc(sizeof(char));
	void* two = sf_malloc(sizeof(char));
	sf_malloc(sizeof(char));
	void* three = sf_malloc(sizeof(char));
	sf_malloc(sizeof(char));
	void* four = sf_malloc(sizeof(char));
	sf_malloc(sizeof(char));
	void* five = sf_malloc(sizeof(char));
	sf_malloc(sizeof(char));
	void* six = sf_malloc(sizeof(char));
	sf_malloc(sizeof(char));
	void* seven = sf_malloc(sizeof(char));
	sf_malloc(sizeof(char));
	void* eight = sf_malloc(sizeof(char));
	sf_malloc(sizeof(char));
	void* nine = sf_malloc(sizeof(char));
	sf_malloc(sizeof(char));
	void* ten = sf_malloc(sizeof(char));
	sf_malloc(sizeof(char));
	void* eleven = sf_malloc(sizeof(char));
	sf_malloc(sizeof(char));
	void* twelve = sf_malloc(sizeof(char));
	sf_malloc(sizeof(char));
	sf_free(one);
	sf_free(two);
	sf_free(three);
	sf_free(four);
	sf_free(five);
	sf_free(six);
	sf_free(seven);
	sf_free(eight);
	sf_free(nine);
	sf_free(ten);
	sf_free(eleven);
	sf_free(twelve);

	int next_counter = 1;
	sf_free_header *curr_header = (sf_free_header *)(seg_free_list[0].head);
	cr_assert_not_null(curr_header->next, "Next block is null.");
	while(next_counter <= 13 && curr_header->next != NULL){
		curr_header = curr_header->next;
		next_counter += 1;
	}
	cr_assert(next_counter == 12, "Unexpected number of blocks in free list traversal using next.");

	int prev_counter = 1;
	while(prev_counter <= 13 && curr_header->prev != NULL){
		curr_header = curr_header->prev;
		prev_counter += 1;
	}
	cr_assert(next_counter == 12, "Unexpected number of blocks in free list traversal using next.");
}
