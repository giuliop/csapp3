/*#define DEBUG 1*/

/*
	 Let's start implementing a single explicit free list, double linked.
	 Let's start focusing on 8-byte words and alignment

	 Allocated blocks are like this:

								|-|-|-|-|-|-|-|-|-|-|-|-|...|-|-|-|-|-|-|-|-|-|-|-|-|
	   Header	    | size of block, in bytes				|0|p|a|
		 pointer->	| start of data						      							      |
								|												 ...												|
								|												 ...												|

	Free blocks are like this:

								|-|-|-|-|-|-|-|-|-|-|-|-|...|-|-|-|-|-|-|-|-|-|-|-|-|
	   Header	    | size of block, in bytes				|0|p|a|
		 pointer->	| pointer to previous free block   						      |
								| pointer to next free block      						      |
								|												 ...												|
	   Footer	    | size of block, in bytes				       

	where the bit 'a' is 1 if the block is allocated or 0 if free
				the bit 'p' is 1 if the previous block is allovated, 0 if free

	The heap starts with a 1 word start marker block made of header only
	(with a = 1) and ends with a 1 word end marker block made of header
	only (with a = 1 and p = 0)
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
    /* Team name */ "me",
    /* First member's full name */ "me",
    /* First member's email address */ "me",
    /* Second member's full name (leave blank if none) */ "",
    /* Second member's email address (leave blank if none) */ ""
};


// define constants
typedef uint64_t * block_p;
#define WSIZE 8
#define DSIZE 16
#define NEW_HEAP_SIZE (1 << 10) // 1K
#define MIN_BLOCK_SIZE 4*WSIZE	// header, prev, next, footer when free

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGNMENT WSIZE
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)
#define ADJ_SIZE(size) (((ALIGN(size)+WSIZE) < MIN_BLOCK_SIZE) ? \
	MIN_BLOCK_SIZE : (ALIGN(size)+WSIZE))// +1 word for header

//set the size and allocation bits for a pointer / block
#define SET_P(p, size, prev_alloc, alloc) \
	(* (block_p)(p) = ((size) | (alloc) | ((prev_alloc)<<1)))

#define SET_SIZE_P(p, size) \
	(* (block_p)(p) = (((* (block_p)(p)) & 0x3) | (size)))

#define SET(bp, size, prev_alloc, alloc) \
	SET_P(HDR(bp), size, prev_alloc, alloc) ; SET_SIZE_P(FTR(bp), size)

#define SET_HDR(bp, size, prev_alloc, alloc) \
	SET_P(HDR(bp), size, prev_alloc, alloc)

#define SET_FTR(bp) SET_SIZE_P(FTR(bp), GET_SIZE(bp))

#define SET_SIZE(bp, size) \
	SET_SIZE_P(HDR(bp), size); SET_SIZE_P(FTR(bp), size)

#define SET_ALLOC(bp) (* (block_p)(HDR(bp)) |= 0x1)
#define SET_NOT_ALLOC(bp) (* (block_p)(HDR(bp)) &= ~0x1)
#define SET_PREV_ALLOC(bp) (* (block_p)(HDR(bp)) |= 0x2)
#define SET_PREV_NOT_ALLOC(bp) (* (block_p)(HDR(bp)) &= ~0x2)

/*#define SETVAL(p, val) (* (block_p)(p) = (val))*/

#define ALLOC 1
#define NOT_ALLOC 0
#define PREV_ALLOC 1
#define PREV_NOT_ALLOC 0

// from a block pointer compute the address of header/footer, next/prev block
#define HDR(bp) ((block_p)(bp) - 1)
#define FTR(bp) ((char *)(bp) + GET_SIZE(bp) - DSIZE)
#define NEXT_BLOCK(bp) ((char *)(bp) + GET_SIZE(bp))
#define PREV_BLOCK(bp) ((char *)(bp) - GET_SIZE_P((char *)(bp) - DSIZE))

// read size and allocation status 
#define GET_P(p) (* (block_p)(p))
#define GET_SIZE_P(p) (GET_P(p) & ~0x3)
#define GET_ALLOC_P(p) (GET_P(p) & 0x1)
#define GET_PREV_ALLOC_P(p) ((GET_P(p) & 0x2) >> 1)

#define GET_SIZE(bp) GET_SIZE_P(HDR(bp))
#define GET_ALLOC(bp) GET_ALLOC_P(HDR(bp))
#define GET_PREV_ALLOC(bp) GET_PREV_ALLOC_P(HDR(bp))

// get and set pointer to next/prev free block
#define SET_PREV_FREE(bp, prev) (* (block_p *)(bp) = (block_p)(prev))
#define SET_NEXT_FREE(bp, next) (* ((block_p *)(bp) + 1) = (block_p)(next))
#define PREV_FREE(bp) (* (block_p *)(bp))
#define NEXT_FREE(bp) (* ((block_p *)(bp) + 1))

// Globals
static void * heap = NULL;						// points to first heap block
static void * first_free = NULL;			// points to first heap free block

// print a block
static void print_block(void * bp) {
	void * p = (block_p)bp - 1;
	printf("header   %p | size: %8ld  prev_alloc: %ld  alloc: %ld\n",
			p, GET_SIZE_P(p), GET_PREV_ALLOC_P(p), GET_ALLOC_P(p));
	if (GET_ALLOC_P(p) == NOT_ALLOC) {
		printf("prev  -> %p | %p\n", bp, PREV_FREE(bp));
		printf("next     %p | %p\n", (block_p)bp + 1, NEXT_FREE(bp));
		printf("footer   %p | size: %8ld\n",
				FTR(bp), GET_SIZE_P(FTR(bp)));
	}
	else if ((bp != heap+WSIZE) && (GET_SIZE(bp) != 0))
		printf("      -> %p |   \n", bp);
	printf("\n");
}

// print the heap
static void print_heap() {
	printf("\n________________________________HEAP"
			     "________________________________\n");
	printf("heap     %p, first_free  %p\n\n", heap, first_free);
	void * bp;
	for (bp = heap + WSIZE; GET_SIZE(bp) > 0; bp = NEXT_BLOCK(bp)) {
		print_block(bp);
	}
	print_block(bp); // print end marker block
}

// take bp out of the free list managing next, prev and first_free
static void free_remove(void * bp) {
	if (PREV_FREE(bp))
		SET_NEXT_FREE(PREV_FREE(bp), NEXT_FREE(bp));

	if (NEXT_FREE(bp))
		SET_PREV_FREE(NEXT_FREE(bp), PREV_FREE(bp));

	if (bp == first_free)
		first_free = NEXT_FREE(bp);
}

// insert bp in the free list after prev
static void free_insert_after (void * bp, void * prev) {
#ifdef DEBUG
		assert(bp);
#endif
	SET_PREV_FREE(bp, prev);
	SET_NEXT_FREE(bp, NEXT_FREE(prev));
	if (NEXT_FREE(prev))
		SET_PREV_FREE(NEXT_FREE(prev), bp);

	SET_NEXT_FREE(prev, bp);
}

// insert bp at start of free list
static void free_insert(void * bp) {
	SET_PREV_FREE(bp, NULL);
	SET_NEXT_FREE(bp, first_free);
	if (first_free)
		SET_PREV_FREE(first_free, bp);
	first_free = bp;
}

// merge two free blocks b1 and b2 and return b1
static void * merge(void * b1, void * b2) {
	free_remove(b2);
	size_t size = GET_SIZE(b1) + GET_SIZE(b2);
	SET_SIZE(b1, size);
	return b1;
}

// coalesce if possible
static void * coalesce(void * bp) {
	if (GET_PREV_ALLOC(bp) == PREV_NOT_ALLOC)
		bp = merge(PREV_BLOCK(bp), bp);
	if (GET_ALLOC(NEXT_BLOCK(bp)) == NOT_ALLOC)
		bp = merge(bp, NEXT_BLOCK(bp));
	return bp;
}

// extend the heap by size and return a pointer to the new area after
// the caller needs to take care of putting the block in the free list
// and coalescing if desired
static block_p extend_heap(size_t size) {
	void * bp; 
	if ((bp = mem_sbrk(size)) == (void *)-1)
		return NULL;

	// The former end marker block is now the header of the new free block
	SET(bp, size, GET_PREV_ALLOC(bp), NOT_ALLOC);

	// Set end marker
	SET_HDR(NEXT_BLOCK(bp), 0, PREV_NOT_ALLOC, ALLOC);
	
	return bp;
}

// mm_init - initialize the malloc package.
int mm_init(void) {
	first_free = NULL;
	// Create initial empty heap with only start and end marker blocks
	if ((heap = mem_sbrk(2 * WSIZE)) == (void *)-1)
		return -1;

	SET_P(heap, WSIZE, PREV_ALLOC, ALLOC);				// start marker header
	SET_P(heap + WSIZE, 0, PREV_ALLOC, ALLOC);		// end marker header

#ifdef DEBUG
	print_heap();
#endif 

	free_insert(extend_heap(NEW_HEAP_SIZE));

#ifdef DEBUG
	print_heap();
#endif 

	if (! first_free)
		return -1;
	return 0;
}

// Takes a free block and a size and splits the block in two, the first of
// size size and the second with the rest
void split (void * bp, size_t size) {
	size_t rest =  (GET_SIZE(bp) - size);

	// set header/footer first block
	SET_SIZE(bp, size);

	// set header/footer second block
	SET(NEXT_BLOCK(bp), rest, PREV_NOT_ALLOC, NOT_ALLOC);

	// insert splitted block in free list
	free_insert_after(NEXT_BLOCK(bp), bp);
}

// Takes a free block and a size and allocates the block after splitting it
// if it can create a free block out of it
void * place(void * bp, size_t size) {
	if ((GET_SIZE(bp) - size) >= MIN_BLOCK_SIZE)
		split(bp, size);

	free_remove(bp);

	SET_ALLOC(bp);
	SET_PREV_ALLOC(NEXT_BLOCK(bp));

#ifdef DEBUG
	print_heap();
#endif 

	return bp;
}

// return a block of size >= size to be used by the application
// return NULL if not possible
void * mm_malloc(size_t size) {

#ifdef DEBUG
		printf("malloc %ld\n", size);
#endif 

	if (size <= 0)
		return NULL;
		
	size_t asize = ADJ_SIZE(size);
	void * prev = NULL;
	void * bp = first_free;

	while (bp) {
		if (GET_SIZE(bp) >= asize)
			return place(bp, asize);
		prev = bp;
		bp = NEXT_FREE(bp);
	}
	// if no block available extend the heap
	bp = extend_heap(asize);
	
	if (! bp)
		return NULL;

	if (prev)
		free_insert_after(bp, prev);
	else
		free_insert(bp);

	return place(bp, asize);
}

// when we free a block we put it at the beginning of the free list
void mm_free(void *ptr) {
#ifdef DEBUG
	printf("free %p\n", ptr);
#endif 

	SET_NOT_ALLOC(ptr);
	SET_FTR(ptr);

	SET_PREV_NOT_ALLOC(NEXT_BLOCK(ptr));

	free_insert(ptr);
	ptr = coalesce(ptr);

#ifdef DEBUG
	print_heap();
#endif 
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
	mem_init();

	for (int i = 0; i < 12; ++i) {
	assert(mm_init() >= 0);
	char * a0 = (char *)mm_malloc(2040);
	char * a1 = (char *)mm_malloc(2040);
	mm_free(a1);
	char * a2 = (char *)mm_malloc(48);
	char * a3 = (char *)mm_malloc(4072);
	mm_free(a3);
	char * a4 = (char *)mm_malloc(4072);
	mm_free(a0);
	mm_free(a2);
	char * a5 = (char *)mm_malloc(4072);
	mm_free(a4);
	mm_free(a5);
	}
}

#ifdef DEBUG
void main() { test(); }
#endif
