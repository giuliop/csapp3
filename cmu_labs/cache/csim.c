#define _GNU_SOURCE
#include "cachelab.h"
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <math.h>
#include <limits.h>

int hits;
int misses;
int evictions;

struct options {
	int valid;	  // 1 for valid options; 0 otherwise
	int h;				// help (optional); 1 for true, 0 for false
	int v;				// verbose (optional); 1 for true, 0 for false
	long s;			  // # of sets
	long E;				// # of lines
	long b;				// Number of block bits (2^b is block size)
	char * t;			// name of the valgrind trace to replay
};
// Parse the invocation options, return NULL if error or a filled struct
struct options parse_args(int argc, char * argv[]) {
	struct options opts = {.valid = 1,};
	for (int i = 1; i < argc; ++i) {
		if (strcmp(argv[i], "-h") == 0) {
			opts.h = 1;
		} else if (strcmp(argv[i], "-v") == 0) {
			opts.v = 1;
		} else if (strcmp(argv[i], "-s") == 0) {
			if (i == argc) {opts.valid = 0;}
			opts.s = strtol(argv[i+1], NULL, 10);
			if (errno == ERANGE) {opts.valid = 0;}
			++i;
		} else if (strcmp(argv[i], "-E") == 0) {
			if (i == argc) {opts.valid = 0;}
			opts.E = strtol(argv[i+1], NULL, 10);
			if (errno == ERANGE) {opts.valid = 0;}
			++i;
		} else if (strcmp(argv[i], "-b") == 0) {
			if (i == argc) {opts.valid = 0;}
			opts.b = strtol(argv[i+1], NULL, 10);
			if (errno == ERANGE) {opts.valid = 0;}
			++i;
		} else if (strcmp(argv[i], "-t") == 0) {
			if (i == argc) {opts.valid = 0;}
			opts.t = argv[i+1];
			if (errno == ERANGE) {opts.valid = 0;}
			++i;
		} else {
		opts.valid = 0;		// default clause
		}
	}
	if (!opts.s || !opts.E || !opts.b || !opts.t) {
		opts.valid = 0;
	} 
	return opts;
}

void print_options(struct options o) {
	printf("valid: %d\n", o.valid);
	printf("h : %d\nv : %d\ns : %ld\nE : %ld\nb : %ld\nt : %s\n",
			o.h, o.v, o.s, o.E, o.b, o.t);
}

void print_help(char * name) {
	printf("Usage: %s [-hv] -s <num> -E <num> -b <num> -t <file>\n", name);
	printf("Options:\n");
	printf("  -h         Print this help message.\n");
	printf("  -v         Optional verbose flag.\n");
	printf("  -s <num>   Number of set index bits.\n");
	printf("  -E <num>   Number of lines per set.\n");
	printf("  -b <num>   Number of block offset bits.\n");
	printf("  -t <file>  Trace file.\n\n");
	printf("Examples:\n");
	printf("  linux>  ./csim-ref -s 4 -E 1 -b 4 -t traces/yi.trace\n");
	printf("  linux>  ./csim-ref -v -s 8 -E 2 -b 4 -t traces/yi.trace\n");
}

struct line {
	int valid;
	unsigned long tag;
	unsigned long last_access;
};

struct cache {
	unsigned long  s;						// # bits per sets
	unsigned long  S;						// # sets
	unsigned long  E;						// # lines per set
	unsigned long  b;						// # of block bits (2^b is block size)
	unsigned long access;				// # of accesses to the cache
	struct line line[];					// array of S*E lines
};

// Create a new cache with S sets of E lines and 2^b block size
// and return a pointer to it
void init_cache(long s, long E, long b, struct cache * c) {
	c->s = s;
	c->S = (1 << s);
	c->E = E;
	c->b = b;
	c->access = 0;
	for (size_t n = 0; n < c->S*E; ++n) {
		c->line[n].valid = 0;
		c->line[n].tag = 0;
		c->line[n].last_access = 0;
	}
	return;
}

struct cache_address {
	unsigned long t;
	unsigned long s;
	unsigned long b;
};

struct mem_access {
	uint64_t address;
	char type;
};
	
// Check if the cache holds the address and updates the cache as needed
// together with the count of hits, misses and evictions
void try_cache(struct cache * c, struct cache_address caddr,
		struct mem_access m, int v) {
	if (v) printf("s: 0x%lx - t: 0x%lx - ", caddr.s, caddr.t);
	c->access += 1;
	size_t to_write;
	int evict = 1;
	unsigned long worst_access = ULONG_MAX;
	for (size_t i = caddr.s * c->E; i < caddr.s * c->E + c->E; ++i) {
		if ((c->line[i].tag == caddr.t) && c->line[i].valid) {
			c->line[i].last_access = c->access;
			++hits;
			if (v) printf("hit");
			if (m.type == 'M') {
				++hits;
				if (v) printf(" hit");
			}
			if (v) printf("\n");
			return;
		}
		if (evict) { 
			if (!c->line[i].valid) {
				evict = 0;
				to_write = i;
			} else if (c->line[i].last_access < worst_access) {
				worst_access = c->line[i].last_access;
				to_write = i;
			}
		}
	}
	++misses;
	if (v) printf("miss");
	c->line[to_write].tag = caddr.t;
	c->line[to_write].last_access = c->access;
	c->line[to_write].valid = 1;
	if (evict) {
		++evictions;
		if (v) printf(" evict");
	}
	if (m.type == 'M') {
		++hits;
		if (v) printf(" hit");
	}
	if (v) printf("\n");
	return;
}

// Transform an entry of a Valgrind trace into a data memory access
// return 0 if not successful, 1 if ok
int entry2mem_access(char * entry, struct mem_access * m, int v) {
	if (!entry || (strlen(entry) < 3)) return 0;
	if (entry[0]!=' ' || (entry[1]!='M' && entry[1]!='L' && entry[1]!='S') || 
			entry[2]!=' ') return 0;
	char * found = strchr(entry, ',');
	if (!found) return 0;
	int len = 2 + (found - entry - 3) + 1; // 0x + address + 0 terminator
	char address[len];
	address[0] = '0';
	address[1] = 'x';
	for (size_t i = 2; i < (len - 1); i++) {
		address[i] = entry[i+1];
	}
	address[len-1] = 0;
	if (v) printf("memory: %s - ", address);
	m->address = (uint64_t)strtol(address, NULL, 0);
	m->type = entry[1];
	if (!m->address && strcmp(address, "0x0")) return 0;
	return 1;
}

// Transofr a mem_access address into a cache address
struct cache_address mem2cache_address(uint64_t addr, struct cache * c) {
	struct cache_address caddr;
	caddr.b = addr & ((1 << c->b) - 1);			// last b bytes of addr
	caddr.s = (addr >> c->b) & (( 1 << c->s) - 1); // middle s bytes of addr
	caddr.t = addr >> (c->s + c->b);
	return caddr;
}

// Check an entry from a Valgrind trace against the cache
void check_entry(char * entry, struct cache * c, int v) {
	struct mem_access m;
	int address = entry2mem_access(entry, &m, v);
	if (!address) return;
	struct cache_address caddr = mem2cache_address(m.address, c);
	caddr.s &= (1 << c->s) - 1;
	try_cache(c, caddr, m, v);
	return;
}

int main(int argc, char * argv[]) {
	hits = 0;
	misses = 0;
	evictions = 0;
	// parse options
	struct options opts = parse_args(argc, argv);
	if (opts.h) {
		print_help(argv[0]);
		return 0;
	} else if (!opts.valid) {
		printf("%s: Missing required command line argument\n", argv[0]);
		print_help(argv[0]);
		return 0;
	}
	 /*init cache*/
	struct cache * cache;
	cache = malloc(sizeof(*cache) + (1 << opts.s) * opts.E * sizeof(*cache->line));
	init_cache(opts.s, opts.E, opts.b, cache);
	 /*read memory access and simulate them*/
	FILE *fp;
	char * entry = NULL;
	size_t len = 0;
	ssize_t read;
	fp = fopen(opts.t, "r");
	if (fp == NULL) {
		printf("%s: No such file or directory", opts.t);
		return 0;
	}
	while ((read = getline(&entry, &len, fp)) != -1) {
		if (opts.v) printf("%.*s - ", (int)strlen(entry)-1, entry);
		check_entry(entry, cache, opts.v);
	}
	fclose(fp);
	if (entry) free(entry);
	printSummary(hits, misses, evictions);
	free(cache);
	return 0;
}
