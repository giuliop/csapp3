// We start two threads: one to read from standard input and one with
// the timeout. We monitor two flags to see which thread is done first

#include "csapp.h"

int timeout_flag = 0;
int input_flag = 0;
pthread_t tid;

struct fgets_args {
	char * s;
	int size;
	FILE * stream;
};

void * timeout(void * varg) {
		sleep(5);
		timeout_flag = 1;
		return NULL;
}

void * input(void * varg) {
	struct fgets_args * a = (struct fgets_args *) varg;
	fgets(a->s, a->size, a->stream);
	input_flag = 1;
	return NULL;
}

char * tfgets(char * s, int size, FILE * stream) {
	struct fgets_args a = { .s = s,
													.size = size,
													.stream = stream };
	Pthread_create(&tid, NULL, timeout, NULL);
	Pthread_create(&tid, NULL, input, (void *) &a);
	while (1) {
		if (timeout_flag)
			return NULL;
		if (input_flag)
			return s;
	}
}

int main() {
	char buf[MAXLINE];

	if (tfgets(buf, MAXLINE, stdin) == NULL)
		printf("BOOM!\n");
	else
		printf("%s", buf);

	exit(0);
}
