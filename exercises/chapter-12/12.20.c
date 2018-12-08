#include "csapp.h"

#define ITERATIONS 5
#define THREADS 10
#define MAXREADERS 2

sem_t mutex, readcnt;

void* reader(void* v) {
	int semcount;
	for (int i = ITERATIONS; i>0; i--) {

		sem_getvalue(&readcnt, &semcount);
		if (semcount == MAXREADERS)		// we are the first
			P(&mutex);

		P(&readcnt);
		// Critical section 
		/*sleep(1);*/
		printf("reader\n");
		V(&readcnt);

		sem_getvalue(&readcnt, &semcount);
		if (semcount == MAXREADERS)		// we are the last
			V(&mutex);
	}
	return NULL;
}

void* writer(void* v) {
	for (int i = ITERATIONS; i>0; i--) {
		P(&mutex);

		// Critical section
		/*sleep(1);*/
		printf("writer\n");

		V(&mutex);
	}
	return NULL;
}

void init() {
	Sem_init(&mutex, 0, 1);
	Sem_init(&readcnt, 0, MAXREADERS);
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
