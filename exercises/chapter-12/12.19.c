#include "csapp.h"
#define ITERATIONS 10
#define THREADS 2

int readcnt;
sem_t mutex, w;

void* reader(void* v) {
	for (int i = ITERATIONS; i>0; i--) {
		P(&mutex);
		readcnt++;
		if (readcnt == 1)
				P(&w);
		V(&mutex);

		// Critical section 
		sleep(1);
		printf("reader\n");

		P(&mutex);
		readcnt--;
		if (readcnt == 0)
				V(&w);
		V(&mutex);
	}
	return NULL;
}

void* writer(void* v) {
	for (int i = ITERATIONS; i>0; i--) {
		while (1) {
			if (! readcnt) {
				break;
			}
		}
		V(&mutex);
		P(&w);

		// Critical section
		sleep(1);
		printf("writer\n");

		V(&w);
	}
	return NULL;
}

void init() {
	readcnt = 0;
	Sem_init(&mutex, 0, 1);
	Sem_init(&w, 0, 1);
}

int main() {
	int n = THREADS;
	pthread_t tid;
	init();

	for (; n > 0; n--) {
		Pthread_create(&tid, NULL, writer, NULL);
		Pthread_create(&tid, NULL, reader, NULL);
	}
	int ret = 1;
	pthread_exit(&ret);
}
