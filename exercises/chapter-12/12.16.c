#include "csapp.h"

void* thread(void* vargp);

int main(int argc, char* argv[]) {
	int n;

	if ((argc != 2) || (! (n = strtol(argv[1], NULL, 10)))) {
		printf("\nUsage is: %s number_of_threads\n", argv[0]);
		return 0;
	}

	pthread_t tid;
	for (long i = 0; i < n; i++) {
		Pthread_create(&tid, NULL, thread, (void*)(i+1));
	}
	int ret = 1;
	pthread_exit(&ret);
}

void* thread(void* vargp) {
	printf("Hello, world! from thread %ld\n", (long)vargp);
	return NULL;
}


