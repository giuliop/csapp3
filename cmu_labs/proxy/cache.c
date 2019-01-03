/*#define DEBUG*/

#include <stdlib.h>
#include <string.h>

#ifdef DEBUG
#include <stdio.h>
#endif

// A simple cache implemented with two data structures:
// 1. a hashmap holding pointers to list of nodes with same hash
// 2. a double linked list of the nodes ordered by last access to
//		implement a least recenlty used eviction policy

struct item
{
	long size;					// the size in bytes of the item
	char val[];					// the actual value
};

typedef struct node
{
	char * key;						// a string identifying the item	
	struct item * item;		// the item
	struct node * next;		// the next item in the hash bucket
	struct node * after;	// the next node in the access list
	struct node * before;	// the previous node in the access list
} node;

static struct cache
{
	long maxsize;			// max allowed cache size in bytes
	long size;				// current cache size in bytes
	long hashcount;		// # of hash buckets
	node * lru;				// least recently used cache item
	node * mru;				// most recently used cache elemtent
	node ** buckets;	// hash pointers to the lists of items
} c;


// private functions
static void mark_mru(node * n);
static long evict_lru();
static node * find_node(char * key);
static void remove_node(node * n);
static void free_node(node * n);
static unsigned long hash(char * str);

// lookup a key in the cache, return NULL if not found or a pointer to 
// the item struct otherwise
struct item * cache_lookup(char * key)
{
	node * n = find_node(key);
	if (! n)
		return NULL;

	mark_mru(n);
	return n->item;
}

// insert an item in the cache, evicting the lru item(s) if we exceed
// the cache size; will overwrite the key if present
// size is the size in bytes of the supplied val
void cache_insert(char * key, char * val, long size)
{
	if (size > c.maxsize)
		return;

	node * n = find_node(key);
	if (n) 
		remove_node(n);

	long newsize = c.size + size;
	while (newsize > c.maxsize)
		newsize -= evict_lru();

	n = malloc(sizeof(node));
	if (! n)
		return;

	// we put the node at the beginning of the relative hash list
	unsigned long h = hash(key);
	n->next = c.buckets[h];
	c.buckets[h] = n;
		
	n->key = malloc(sizeof(char) * (strlen(key) + 1));
	if (! n->key) {
		free(n);
		return;
	}
	strcpy(n->key, key);

	n->item = malloc(sizeof(long) + size);
	if (! n->item) {
		free(n->key);
		free(n);
		return;
	}
	n->item->size = size;
	memcpy(n->item->val, val, size);

	n->before = NULL;
	n->after = c.mru;
	if (c.mru)
		c.mru->before = n;
	c.mru = n;
	if (! c.lru)
		c.lru = n;
	c.size = newsize;
}

// initialize the cache; return 1 if success, 0 otherwise
int cache_init(long maxsize, long hashcount)
{
	c.maxsize = maxsize;
	c.size = 0;
	c.hashcount = hashcount;
	c.lru = c.mru = NULL;
	c.buckets = calloc(hashcount, sizeof(node *));
	if (! c.buckets)
		return 0;
	return 1;
}

// reset the cache, will require to reinitialize it
void cache_reset()
{
	node * n = c.mru;
	node * after;
	while (n) {
		after = n->after;
		free_node(n);
		n = after;
	}
	for (int i = 0; i < c.hashcount; ++i)
		c.buckets[i] = NULL;

	c.size = 0;
	c.lru = c.mru = NULL;

	free(c.buckets);
}

/****************************************************************************** 
PRIVATE FUNCTIONS SECTION
******************************************************************************/

// put the node at the beginning of the access list, i.e. make it the mru
static void mark_mru(node * n)
{
	if (c.mru == n)
		return;
	else {
		n->before->after = n->after;
	}

	if (n->after)
		n->after->before = n->before;

	n->after = c.mru;
	c.mru->before = n;
	c.mru = n;

	if (c.lru == n)
		c.lru = n->before;
	n->before = NULL;
}

// remove the lru item from the cache and return its size
static long evict_lru()
{
	if (! c.lru)
		return 0;
	long size = c.lru->item->size;
	remove_node(c.lru);
	return size;
}

// lookup a key in the cache, return NULL if not found or a pointer to
// the node otherwise
static node * find_node(char * key)
{
	node * n = c.buckets[hash(key)];
	while (n) {
		if (! strcmp(n->key, key)) {
			return n;
		}
		n = n->next;
	}
	return NULL;
}

// remove the node n from the cache
static void remove_node(node * n)
{
	if (! n)
		return;

	if (c.lru == n)
		c.lru = n->before;
	if (c.mru == n)
		c.mru = n->after;

	if (n->before)
		n->before->after = n->after;
	if (n->after)
		n->after->before = n->before;

	unsigned long h = hash(n->key);
	if (n == c.buckets[h])
		c.buckets[h] = n->next;
	else {
		for (node * i = c.buckets[h]; i->next; i = i->next) {
			if (i->next == n) {
				i->next = n->next;
				break;
			}
		}
	}

	c.size -= n->item->size;
	free_node(n);
}

// delete a node freeing up its memory
static void free_node(node * n)
{
	free(n->item);
	free(n->key);
	free(n);
}

// hash a string key
static unsigned long hash(char * str)
{
	unsigned long hash = 5381;
	int n;

	while ((n = *str++))
		hash = ((hash << 5) + hash) + n; /* hash * 33 + c */

	return hash % c.hashcount;
}


/****************************************************************************** 
DEBUG FACILITIES SECTION 
******************************************************************************/

// Main function launches an interactive shell program to test the cache

#ifdef DEBUG
// print a node to debug it
static void print_node(node * n)
{
	printf("Node %p\n", n);
	printf("key %s\t value %s\t size %ld\n", n->key, n->item->val, n->item->size);
	printf("next %p\t after %p\t before %p\t\n", n->next, n->after, n->before);
}

// print the state of the cache to debug it
static void print_cache()
{
	printf("\nmaxsize %ld \t size %ld \t hashcount %ld \n", c.maxsize, c.size,
			c.hashcount);
	printf("lru %p \t mru %p \n", c.lru, c.mru);

	printf("\nAccess list, from mru\n");
	for (node * n = c.mru; n; n = n->after)
		printf("%s \t", n->key);
	printf("\nAccess list, from lru\n");
	for (node * n = c.lru; n; n = n->before)
		printf("%s \t", n->key);

	printf("\n");
	for (int i = 0; i < c.hashcount; ++i) {
		printf("\nBucket %d\n", i);
		for (node * n = c.buckets[i]; n; n = n->next)
			print_node(n);
		printf("\n");
	}
}
// interact with the library to test it
#define MAXLINE 100
static void interactive_testing()
{
	char line[MAXLINE], cmd[MAXLINE], arg1[MAXLINE], arg2[MAXLINE];
	printf("\nWelcome to your cache. Please initialize it !\n");

	while (1) {
		cmd[0] = arg1[0] = arg2[0] = '\0';
		
		printf("\n > ");
		fflush(stdout);

		fgets(line, MAXLINE, stdin);
		sscanf(line, "%s %s %s", cmd, arg1, arg2);

		if ((! strcmp(cmd, "lookup")) || (! strcmp(cmd, "l"))) {
			printf("looking up key <%s>\n", arg1);
			struct item * x = cache_lookup(arg1);
			if (! x)
				printf("Key not present\n");
			else
				printf("Found value: <%s>\n", x->val);

		} else if ((! strcmp(cmd, "insert")) || (! strcmp(cmd, "i"))) {
			printf("Inserting key <%s>\n", arg1);
			printf("With value <%s>\n", arg2);
			cache_insert(arg1, arg2, strlen(arg2)+1);

		} else if ((! strcmp(cmd, "init")) || (! strcmp(cmd, "t"))) {
			long maxsize = strtol(arg1, NULL, 10);
			long hashcount = strtol(arg2, NULL, 10);
			printf("Initialzing cache with %ld bytes size and %ld hashcount\n",
					maxsize, hashcount);
			if (! cache_init(maxsize, hashcount))
				printf("Oh no, initialization did not succeed !\n");

		} else if ((! strcmp(cmd, "reset")) || (! strcmp(cmd, "r"))) {
			printf("Resetting the cache, remember to reinitialize it!");
			cache_reset();

		} else {
			printf("Unknown command, printing known commands:\n\n"
					   "Initialize the cache -> ini(t) byte-size hashcount\n"
					   "Insert an item       -> (i)nsert key value\n"
					   "Lookup an item       -> (l)ookup key\n"
					   "Reset the cache      -> (r)eset\n");
			continue;
		}
		print_cache();
	}
}

int main()
{
	interactive_testing();
	exit(0);
}
#endif
