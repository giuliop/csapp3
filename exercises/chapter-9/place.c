static void place(void *bp, size_t asize) {
	size_t block_size = GET_SIZE(HDRP(bp));
	size_t remainder_size = block_size - asize;
	if (remainder_size >= 4 * WSIZE) {
		// split block
		// resize current block
		PUT(HDRP(bp), PACK(asize, 1));
		PUT(FTRP(bp), PACK(asize, 1));
		// create new free block
		PUT(HDRP(NEXT_BLKP(bp)), PACK(remainder_size, 0));
		PUT(FTRP(NEXT_BLKP(bp)), PACK(remainder_size, 0));
	} else {
		// no splitting
		PUT(HDRP(bp), PACK(block_size, 1));
		PUT(FTRP(bp), PACK(block_size, 1));
	}
}
