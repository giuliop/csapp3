// a package to provide a FIFO buffer of ints with thread-safe access

struct sbuf_t;
typedef struct sbuf_t sbuf_t;

// init the buffer to hold n ints
sbuf_t * sbuf_init(int n);

// return 1 if buffer full, 0 if not
int sbuf_isfull(sbuf_t * sb);

// return 1 if buffer empty, 0 if not
int sbuf_isempty(sbuf_t * sb);

// add item to buffer
void sbuf_insert(sbuf_t * sb, int x);

// pop item from buffer returning it
int sbuf_pop(sbuf_t * sb);
