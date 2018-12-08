#include "csapp.h"
#define ITERATIONS 5
#define THREADS 4

int readers, waiting_writers;
sem_t readers_m, rw_m, waiting_writers_m;

void* reader(void* v) {
	long id = (long)v;
	for (int i = ITERATIONS; i>0; i--) {
		printf("reader %ld: waiting\n", id);
		while (1) {
			if (! waiting_writers) {
				printf("reader %ld: unblocked\n", id);
				break;
			}
		}

		P(&readers_m);
		readers++;
		if (readers == 1)
				P(&rw_m);
		V(&readers_m);

		// Critical section 
		printf("reader %ld: reading\n", id);

		P(&readers_m);
		readers--;
		if (readers == 0)
				V(&rw_m);
		V(&readers_m);
		printf("reader %ld: done\n", id);
	}
	return NULL;
}

void* writer(void* v) {
	long id = (long)v;
	for (int i = ITERATIONS; i>0; i--) {
		P(&waiting_writers_m);
		++waiting_writers;
		printf("writer %ld: waiting\n", id);
		V(&waiting_writers_m);

		P(&rw_m);
		// Critical section
		printf("writer %ld: writing\n", id);
		V(&rw_m);

		P(&waiting_writers_m);
		--waiting_writers;
		V(&waiting_writers_m);
		printf("writer %ld: done\n", id);
	}
	return NULL;
}

void init() {
	readers = 0;
	waiting_writers = 0;
	Sem_init(&readers_m, 0, 1);
	Sem_init(&rw_m, 0, 1);
	Sem_init(&waiting_writers_m, 0, 1);
}

int main() {
	long n = THREADS;
	pthread_t tid;
	init();

	for (; n > 0; n--) {
		Pthread_create(&tid, NULL, writer, (void*)(THREADS + 1 - n));
		Pthread_create(&tid, NULL, reader, (void*)(THREADS + 1 - n));
	}
	int ret = 1;
	pthread_exit(&ret);
}
