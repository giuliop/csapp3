#include "csapp.h"

void* thread(void* vargp);

int main(int argc, char* argv[]) {
	int n;

	if ((argc != 2) || (! (n = strtol(argv[1], NULL, 10)))) {
		printf("\nUsage is: %s number_of_threads\n", argv[0]);
		return 0;
	}

	pthread_t tid;
	for (int i = 0; i < n; i++) {
		int t = i+1;
		Pthread_create(&tid, NULL, thread, &t);
	}
	int ret = 1;
	pthread_exit(&ret);
}

void* thread(void* vargp) {
	printf("Hello, world! from thread %d\n", *(int* )vargp);
	return NULL;
}


