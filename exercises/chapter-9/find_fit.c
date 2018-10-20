static void * find_fit(size_t asize) {
	size_t size;
	void * bp = NEXT_BLKP(heap_listp);			// address of first block
	while (size = GET_SIZE(HDRP(bp))) {						// false if epilogue_block
		if ( (! GET_ALLOC(HDRP(bp))) && (size >= asize) )
			return bp;
		bp = NEXT_BLKP(bp);
	}
	return NULL;
}

