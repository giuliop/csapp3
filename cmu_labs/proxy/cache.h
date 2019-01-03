// A simple web cache

struct item
{
	long size;					// the size in bytes of the item
	char * val;					// the actual value
};

// lookup a key in the cache, return NULL if not found or a pointer to 
// the item struct otherwise
struct item * cache_lookup(char * key)

// insert an item in the cache, evicting the lru item(s) if we exceed
// the cache size; will overwrite the key if present
void cache_insert(char * key, struct item * x)

// initialize the cache; return 1 if success, 0 otherwise
int cache_init(long maxsize, long hashcount)

// empty the cash freeing all memory
void cache_empty()

// reset the cache, will require to reinitialize it
void cache_reset();
