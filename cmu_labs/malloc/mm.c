/*
	 Let's start implementing a single explicit free list, double linked.
	 Let's start focusing on 8-byte words and alignment

	 Allocated blocks are like this:

								|-|-|-|-|-|-|-|-|-|-|-|-|...|-|-|-|-|-|-|-|-|-|-|-|-|
	   Header	    | size of block w/out header, in bytes				|0|p|a|
		 pointer->	| start of data						      							      |
								|												 ...												|

	Free blocks are like this:

								|-|-|-|-|-|-|-|-|-|-|-|-|...|-|-|-|-|-|-|-|-|-|-|-|-|
	   Header	    | size of block w/out header, in bytes				|0|p|a|
		 pointer->	| pointer to previous free block   						      |
								| pointer to next free block      						      |
								|												 ...												|
	   Footer	    | size of block w/out header, in bytes				|0|p|a|

	where the bit 'a' is 1 if the block is allocated or 0 if free
				the bit 'p' is 1 if the previous block is allovated, 0 if free

	The heap starts with a 2 word start marker block made of header and
	footer (with a = 1 and p = 0) and ends with a 1 word end marker
	block made of only header (with a = 1 and p = 0)

 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "me",
    /* First member's full name */
    "me",
    /* First member's email address */
    "me",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

// define constants
typedef uint64_t * block_p;
#define WSIZE (sizeof(uint64_t))
#define DSIZE 2*WSIZE
#define NEW_HEAP_SIZE (1 << 12) // 4K

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGNMENT WSIZE
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)

// useful macros to manipulate blocks given block pointer bp
//set the size and allocation bits for a block
#define SET(bp, size, prev_alloc, alloc) \
	(* (block_p)(bp) = ((size) | (alloc) | ((prev_alloc)<<1)))

// set the allocation bits for current and previous block
#define SET_ALLOC(bp, alloc) ( * (block_p)(bp) &= ((~0) & alloc) )
#define SET_PREV_ALLOC(bp, alloc) (* (block_p)(bp) &= ((~0) & ((alloc)<<1)))

#define ALLOC 1
#define NOT_ALLOC 0
#define PREV_ALLOC 1
#define PREV_NOT_ALLOC 0

// compute the header/footer address of a block, the next/prev block address,
// get its size and allocation status for current and previous block
#define HDR(bp) ((block_p)(bp) - 1)
#define FTR(bp) ((char *)(bp) + GET_SIZE(HDR(bp)) - WSIZE)
#define NEXT_BLOCK(bp) ((char *)(bp) + GET_SIZE(HDR(bp)) + WSIZE)
#define PREV_BLOCK(bp) ((char *)(bp) - GET_SIZE((char *)(bp) - DSIZE) - WSIZE)

#define GET(p) (* (block_p)(p))
#define GET_SIZE(p) (GET(p) & ~0x3)

#define GET_ALLOC(bp) (* (block_p)(HDR(bp)) & 0x1)
#define GET_PREV_ALLOC(bp) ((* (block_p)(HDR(bp)) & 0x2 ) >> 1)

// Globals
static void * hp;			// points to first heap free block

// coalesce if possible
static block_p coalesce(block_p bp) {
	return bp;
}

// extend the heap by size and return a pointer to the new area
static block_p extend_heap(size_t size) {
	block_p bp; 
	size_t asize = ALIGN(size);	

	if ((bp = mem_sbrk(asize)) == (void *)-1)
		return NULL;

	// The former end marker block is not the header of the free block
	// pointed by bp; let's set header and footer for bp and the new
	// end marker block
	SET(HDR(bp), asize - WSIZE, PREV_NOT_ALLOC, ALLOC); // start marker header
	SET(FTR(bp), asize - WSIZE, PREV_NOT_ALLOC, ALLOC);// start marker footer
	SET(HDR(NEXT_BLOCK(bp)), 0, PREV_NOT_ALLOC, ALLOC);	// end marker header
	
	return coalesce(bp);
}

// mm_init - initialize the malloc package.
int mm_init(void) {
	// Create initial empty heap with only start and end marker blocks
	if ((hp = mem_sbrk(2 * WSIZE)) == (void *)-1)
		return -1;
	SET(hp, WSIZE, PREV_NOT_ALLOC, ALLOC);				// start marker header
	SET(hp + 1, WSIZE, PREV_NOT_ALLOC, ALLOC);		// start marker footer
	SET(hp + 2, 0, PREV_NOT_ALLOC, ALLOC);				// end marker header
	hp = extend_heap(NEW_HEAP_SIZE);
	if (! hp)
		return -1;
	return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void * mm_malloc(size_t size) {
	int newsize = ALIGN(size) + WSIZE;
	void *p = mem_sbrk(newsize);
	if (p == (void *)-1)
		return NULL;
	else {
		*(size_t *)p = size;
		return (void *)((char *)p + WSIZE);
	}
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr) {
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size) {
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;
    
    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;
    copySize = *(size_t *)((char *)oldptr - WSIZE);
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}

void error(char * s) {
	printf("%s", s);
	exit(-1);
}

/*int main() {*/
	/*mem_init();*/
	/*if (mm_init() < 0)*/
		/*error("mm_init failed");*/
	/*return 0;*/
	/*int * p = (int *)mm_malloc(sizeof(int));*/
	/*if (! p)*/
		/*error("mm_malloc failed");*/
	/** p = 4;*/
	/*assert(* p == 4);*/
	/*mm_free(p);*/
	/*return 0;*/
/*}*/
