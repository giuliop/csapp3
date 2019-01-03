// A simple web cache


// lookups return a pointer to an item struct
struct item
{
	long size;			// the size in bytes of the item
	char * val;			// the actual value
};


// lookup a key in the cache, return NULL if not found or a pointer to 
// the item struct otherwise
struct item * cache_lookup(char * key)

// insert an item in the cache, evicting the lru item(s) if we exceed
// the cache size; will overwrite the key if present
// size is the size in bytes of the supplied val
void cache_insert(char * key, char * val, long size)

// initialize the cache; return 1 if success, 0 otherwise
int cache_init(long maxsize, long hashcount)

// reset the cache, will require to reinitialize it
void cache_reset();
