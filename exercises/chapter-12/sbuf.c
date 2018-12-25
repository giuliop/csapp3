// a package to provide a FIFO buffer of ints with thread-safe access
#include "csapp.h"
#include "sbuf.h" 

struct sbuf_t {
	int max;			// number of max items
	int n;				// number of current items
	int * buf;		// array of items
	int start;		// position of the first item
	sem_t mutex;	// protect access
	sem_t slots;	// count avaiable slots
	sem_t items;	// count avaiable item
};

typedef struct sbuf_t sbuf_t;

// return a malloc'ed pointer to a initialized buffer able to hold n ints
sbuf_t * sbuf_init(int n) {
	sbuf_t * sb = Malloc(sizeof(sbuf_t));
	sb->buf = Calloc(n, sizeof(int));
	sb->max = n;
	sb->n = 0;
	sb->start = 0;
	Sem_init(&sb->mutex, 0, 1);
	Sem_init(&sb->slots, 0, n);
	Sem_init(&sb->items, 0, 0);
	return sb;
}

// return 1 if buffer full, 0 if not
int sbuf_isfull(sbuf_t * sb) {
	return (sb->n == sb->max);
}

// return 1 if buffer empty, 0 if not
int sbuf_isempty(sbuf_t * sb) {
	return (!sb->n);
}

// add item to buffer
void sbuf_insert(sbuf_t * sb, int x) {
	P(&sb->slots);
	P(&sb->mutex);
	int next = (sb->start + sb->n) % sb->max;
	*(sb->buf + next) = x;
	++(sb->n);
	V(&sb->mutex);
	V(&sb->items);
}

// pop item from buffer returning it
int sbuf_pop(sbuf_t * sb) {
	int x;
	P(&sb->items);
	P(&sb->mutex);
	x = *(sb->buf + sb->start);
	++(sb->start);
	if (sb->start == sb->max)
		sb->start = 0;
	--(sb->n);
	V(&sb->mutex);
	V(&sb->slots);
	return x;
}
