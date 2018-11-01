/*
	 Let's start implementing a single explicit free list, double linked.
	 Let's start focusing on 8-byte words and alignment

	 Allocated blocks are like this:

								|-|-|-|-|-|-|-|-|-|-|-|-|...|-|-|-|-|-|-|-|-|-|-|-|-|
	   Header	    | size of block, in bytes				|0|p|a|
		 pointer->	| start of data						      							      |
								|												 ...												|

	Free blocks are like this:

								|-|-|-|-|-|-|-|-|-|-|-|-|...|-|-|-|-|-|-|-|-|-|-|-|-|
	   Header	    | size of block, in bytes				|0|p|a|
		 pointer->	| pointer to previous free block   						      |
								| pointer to next free block      						      |
								|												 ...												|
	   Footer	    | size of block, in bytes				|0|0|a|

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
#define WSIZE 8
#define DSIZE 16
#define NEW_HEAP_SIZE (1 << 12) // 4K

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGNMENT WSIZE
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)
#define ADJ_SIZE(size) (ALIGN(size) + 3*WSIZE)// 3 words for header, next, prev

//set the size and allocation bits for a pointer
#define SET(p, size, prev_alloc, alloc) \
	(* (block_p)(p) = ((size) | (alloc) | ((prev_alloc)<<1)))
#define SET_ALLOC(p, alloc) ( * (block_p)(p) &= ((~0) & alloc) )
#define SET_PREV_ALLOC(p, alloc) (* (block_p)(p) &= ((~0) & ((alloc)<<1)))

#define SETVAL(p, val) (* (block_p)(p) = (val))

#define ALLOC 1
#define NOT_ALLOC 0
#define PREV_ALLOC 1
#define PREV_NOT_ALLOC 0

// from a block pointer compute the address of header/footer, next/prev block
#define HDR(bp) ((block_p)(bp) - 1)
#define FTR(bp) ((char *)(bp) + GET_SIZE(HDR(bp)) - DSIZE)
#define NEXT_BLOCK(bp) ((char *)(bp) + GET_SIZE(HDR(bp)))
#define PREV_BLOCK(bp) ((char *)(bp) - GET_SIZE((char *)(bp) - DSIZE))

// read size and allocation status from a pointer 
#define GET(p) (* (block_p)(p))
#define GET_SIZE(p) (GET(p) & ~0x3)
#define GET_ALLOC(p) (GET(p) & 0x1)
#define GET_PREV_ALLOC(p) ((GET(p) & 0x2) >> 1)

// get and set pointer to next/prev free block
#define SET_PREV_FREE(bp, prev) (* (block_p *)(bp) = (block_p)(prev))
#define SET_NEXT_FREE(bp, next) (* ((block_p *)(bp) + 1) = (block_p)(next))
#define PREV_FREE(bp) (* (block_p *)(bp))
#define NEXT_FREE(bp) (* ((block_p *)(bp) + 1))

// Globals
static void * hp;			// points to first heap free block

// coalesce if possible
static void * coalesce(void * bp) {
	return bp;
}

// extend the heap by size and return a pointer to the new area
// setting the previous free block to prev and the next to NULL
static block_p extend_heap(size_t size, void * prev) {
	void * bp; 

	if ((bp = mem_sbrk(size)) == (void *)-1)
		return NULL;

	SET_NEXT_FREE(bp, NULL);
	SET_PREV_FREE(bp, prev);
	if (prev)
		SET_NEXT_FREE(prev, bp);

	// The former end marker block is now the header of the free block
	// pointed by bp
	int prev_alloc = GET_PREV_ALLOC(HDR(bp));
	SET(HDR(bp), size, prev_alloc, ALLOC); // start marker header
	SET(FTR(bp), size, prev_alloc, ALLOC);// start marker footer
	SET(HDR(NEXT_BLOCK(bp)), 0, PREV_NOT_ALLOC, ALLOC);	// end marker header
	
	return coalesce(bp);
}

// mm_init - initialize the malloc package.
int mm_init(void) {
	// Create initial empty heap with only start and end marker blocks
	if ((hp = mem_sbrk(2 * WSIZE)) == (void *)-1)
		return -1;

	SET(hp, DSIZE, PREV_ALLOC, ALLOC);				// start marker header
	SET(hp + 1, DSIZE, PREV_ALLOC, ALLOC);		// start marker footer
	SET(hp + 2, 0, PREV_ALLOC, ALLOC);				// end marker header

	hp = extend_heap(NEW_HEAP_SIZE, NULL);

	if (! hp)
		return -1;
	return 0;
}

// Sets up free block bp of size size as an allocated block
void * place(void * bp, size_t size) {
	if (PREV_FREE(bp))
		SET_NEXT_FREE(PREV_FREE(bp), NEXT_FREE(bp));

	if (NEXT_FREE(bp))
		SET_PREV_FREE(NEXT_FREE(bp), PREV_FREE(bp));

	if (bp == hp)
		hp = NEXT_FREE(bp);

	SET_ALLOC((HDR(bp)), ALLOC);
	SET_PREV_ALLOC(HDR(NEXT_BLOCK(bp)), PREV_ALLOC);
	return bp;
}

// return a block of size >= size to be used by the application
// return NULL if not possible
void * mm_malloc(size_t size) {
	size_t asize = ADJ_SIZE(size);
	void * prev = NULL;
	void * bp = hp;

	while (bp) {
		if (GET_SIZE(HDR(bp)) >= asize)
			return place(bp, asize);
		prev = bp;
		bp = NEXT_FREE(bp);
	}
	// if no block available extend the heap
	bp = extend_heap(asize, prev);
	if (! bp)
		return NULL;
	if (! hp)
		hp = bp;
	return place(bp, asize);
}

// TBD implementation: when we free a block we put it at the beginning
// of the free list
void mm_free(void *ptr) {
	SET_NEXT_FREE(ptr, hp);
	SET_PREV_FREE(ptr, NULL);

	SET_ALLOC(HDR(ptr), NOT_ALLOC);
	SETVAL(FTR(ptr), GET(HDR(ptr)));
	SET_PREV_ALLOC(HDR(NEXT_BLOCK(ptr)), PREV_NOT_ALLOC);

	hp = coalesce(ptr);
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

void test() {
	// test macros
	for (int i = 0; i < 100; i++) {
		int j = ALIGN(i);
		assert(j >= i);
		assert(j % ALIGNMENT == 0);
		assert(j < (i + ALIGNMENT));
	}
	// allocate 12 words and make 2 blocks of 4 and 8 words
	void * p = malloc(12 * WSIZE);
	size_t size1 = 4 * WSIZE;
	size_t size2 = 8 * WSIZE;

	SET(p, size1, PREV_ALLOC, ALLOC);
	assert(GET_SIZE(p) == size1);
  assert(GET_PREV_ALLOC(p) == PREV_ALLOC);
  assert(GET_ALLOC(p) == ALLOC);
	SET(p, size1, PREV_NOT_ALLOC, NOT_ALLOC);
  assert(GET_PREV_ALLOC(p) == PREV_NOT_ALLOC);
  assert(GET_ALLOC(p) == NOT_ALLOC);
	p = (block_p)p + 1;
	SET(FTR(p), size1, PREV_NOT_ALLOC, NOT_ALLOC);
	assert(GET_SIZE((block_p)p+2) == size1);
	assert(GET_SIZE(FTR(p)) == size1);
	assert(GET_SIZE(HDR(p)) == GET_SIZE(FTR(p)));
	SET(HDR(NEXT_BLOCK(p)), size2, PREV_NOT_ALLOC, NOT_ALLOC);
	SET(FTR(NEXT_BLOCK(p)), size2, PREV_NOT_ALLOC, NOT_ALLOC);
	p = NEXT_BLOCK(p);
	assert(GET_SIZE(HDR(p)) == size2);
	assert(GET_SIZE(HDR(PREV_BLOCK(p))) == size1);
	p = PREV_BLOCK(p);
	SET_NEXT_FREE(p, NEXT_BLOCK(p));
	SET_PREV_FREE(NEXT_BLOCK(p), p);
	assert(NEXT_FREE(p) == (block_p)NEXT_BLOCK(p));
	assert(PREV_FREE(NEXT_BLOCK(p)) == p);

	mem_init();
	if (mm_init() < 0)
		error("mm_init failed");
	int * x = (int *)mm_malloc(sizeof(int));
	int * y = (int *)mm_malloc(sizeof(int));
	if (! x || ! y)
		error("mm_malloc failed");
	* x = 4;
	* y = 7;
	assert((* x) + (* y) == 11);
	mm_free(x);
	mm_free(y);

	p = (block_p)p - 1;
	free(p);
}


/*void main() { test(); }*/
