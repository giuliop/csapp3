#include "csapp.h"
#include <assert.h>

#define N 512
#define CORES 4

struct pack {
	int i;
	int (*M)[N];
	int (*X)[N];
};

void init_matrix(int M[N][N]) {
	for (int i = 0; i < N; ++i) {
		for (int j = 0; j < N; ++j) {
			M[i][j] = i * 10 + j;
		}
	}
}

void print_matrix(int M[N][N]) {
	for (int i = 0; i < N; ++i) {
		printf("[ ");
		for (int j = 0; j < N; ++j) {
			printf("%6d ", M[i][j]);
		}
		printf("]\n");
	}
}
	
void mult(int M[N][N], int X[N][N]) {
	for (int i = 0; i < N; ++i) {
		for (int j = 0; j < N; ++j) {
			int sum = 0;
			for (int z = 0; z < N; ++z) {
				sum += M[i][z] * M[z][j];
			}
			X[i][j] = sum;
		}
	}
}

void row_mult(int row, int M[N][N], int X[N][N]) {
	int i = row;
	for (int j = 0; j < N; ++j) {
		int sum = 0;
		for (int z = 0; z < N; ++z) {
			sum += M[i][z] * M[z][j];
		}
		X[i][j] = sum;
	}
}

void * f (void * varg) {
	struct pack * p = (struct pack *) varg;
	int nrows = N / CORES;
	int row1 = nrows * p->i;
	int rowlast = row1 + nrows;
	for (int i = row1; i < rowlast; ++i) {
		row_mult(i, p->M, p->X);
	}
	return NULL;
}


int main(int argc, char * argv[]) {
	if (argc != 2) {
		printf("usage: %s <single or multi>", argv[0]);
		exit(0);
	}
	assert(N % CORES == 0);
	pthread_t tid[CORES];
	struct pack pack[CORES];
	int M[N][N];
	int X[N][N];
	
	if (! strcmp(argv[1], "multi")) {
		init_matrix(M);
		for (int i = 0; i < CORES; ++i) {
			pack[i].i = i; pack[i].M = M; pack[i].X = X;
			Pthread_create(&tid[i], NULL, f, (void *) &pack[i]);
		}
		for (int i = 0; i < CORES; ++i) {
			Pthread_join(tid[i], NULL);
		}
	} else if (! strcmp(argv[1], "single")) {
		init_matrix(M);
		mult(M, X);
	} else
		printf("usage: %s <single or multi>", argv[0]);

	exit(0);
}
