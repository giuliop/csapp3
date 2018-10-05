/* 
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */ 
#include <stdio.h>
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. 
 */

void trans(int m, int n, int jj, int ii, int M, int N, int A[N][M], int B[M][N]) {
	m += jj;
	n += ii;
	for (; jj<m; jj++) {
		for (int i=ii; i<n; i++) {
			B[jj][i] = A[i][jj];
		}
	}
}

void copy(int m, int n, int j1, int i1, int j2, int i2, int M, int N, int A[N][M], int B[M][N]) {
	m += j1;
	n += i1;
	for (; j1<m; j1++, j2++) {
		int i11=i1;
		int i22=i2;
		for (; i11<n; i11++, i22++) {
			B[i22][j2] = A[i11][j1];
		}
	}
}

void copytrans(int m, int n, int j1, int i1, int j2, int i2, int M, int N, int A[N][M], int B[M][N]) {
	for (int nn = 0; nn<n; ++nn) {
		for (int mm = 0; mm<m; ++mm) {
			B[i2+mm][j2+nn] = A[i1+nn][j1+mm];
		}
	}
}

void trans_diag_4x4(int j, int i, int M, int N, int A[N][M], int B[M][N]) {
	int t1, t2, t3, t4, t5, t6, t7, t8;
	t1 = A[i+2][j];
	t2 = A[i+2][j+1];
	t3 = A[i+3][j];
	t4 = A[i+3][j+1];
	t5 = A[i+2][j+2];
	t6 = A[i+2][j+3];
	t7 = A[i+3][j+2];
	t8 = A[i+3][j+3];
	trans(2, 2, j+2, i, M, N, A, B); 
	B[j+2][i+2] = t5;
	B[j+3][i+2] = t6;
	B[j+2][i+3] = t7;
	B[j+3][i+3] = t8;
	t5 = A[i][j];
	t6 = A[i][j+1];
	t7 = A[i+1][j];
	t8 = A[i+1][j+1];
	B[j][i+2] = t1;
	B[j+1][i+2] = t2;
	B[j][i+3] = t3;
	B[j+1][i+3] = t4;
	B[j][i] = t5;
	B[j+1][i] = t6;
	B[j][i+1] = t7;
	B[j+1][i+1] = t8;
}

char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
	if (M == 32 && N == 32) {
		for (int i=0; i<N; i+=8) {
			for (int j=0; j<M; j+=8) {
				trans(4, 4, j+4, i, M, N, A, B); 
				trans_diag_4x4(j, i, M, N, A, B); 
				trans(4, 4, j, i+4, M, N, A, B); 
				trans_diag_4x4(j+4, i+4, M, N, A, B); 
			}
		}
	}
	if (M == 64 && N == 64) {
		for (int i=0; i<N; i+=8) {
			for (int j=0; j<M; j+=8) {
				if (i == j && i < 48) {
					copy(8, 4, j, i, 48, 56, M, N, A, B);
					copy(8, 4, j, i+4, 56, 60, M, N, A, B);
					copytrans(4, 4, 48, 56, i, j, M, N, B, B);
					copytrans(4, 4, 56, 60, j+4, i, M, N, B, B);
					copytrans(4, 4, 52, 56, j, i+4, M, N, B, B);
					copytrans(4, 4, 60, 60, i+4, j+4, M, N, B, B);
				} else {
						trans_diag_4x4(j, i, M, N, A, B); 
					int jj = 60;
					for (int z=0; z<4; ++z) {
						int t1 = A[z+i][j+4];
						int t2 = A[z+i][j+5];
						int t3 = A[z+i][j+6];
						int t4 = A[z+i][j+7];
						trans(4, 1, j, z+i+4, M, N, A, B);
						if (j == 56 && i < 56) jj = 52;
						B[z+60][jj] = t1;
						B[z+60][jj+1] = t2;
						B[z+60][jj+2] = t3;
						B[z+60][jj+3] = t4;
					}
					copytrans(4, 4, jj, 60, i, j+4, M, N, B, B);
					trans_diag_4x4(j+4, i+4, M, N, A, B); 
				}
			}
		}
	}
	if (M == 61 && N == 67) {
		int i, j;
		for (i = 0; i < 64; i+=8) {	// will miss 3 lines
			for (j = 0; j < 56; j+=8) {	// will miss 5 columns
				trans_diag_4x4(j+4, i, M, N, A, B); 
				trans_diag_4x4(j, i, M, N, A, B); 
				trans_diag_4x4(j, i+4, M, N, A, B); 
				trans_diag_4x4(j+4, i+4, M, N, A, B); 
			}
			// last 5 columns
			trans(1, 4, j+4, i, M, N, A, B); 
			trans_diag_4x4(j, i, M, N, A, B); 
			trans(5, 4, j, i+4, M, N, A, B); 
		}
		// last 3 lines
		// last 5 columns
		trans(5, 3, 56, i, M, N, A, B); 
		// first columns
		for (j = 0; j < 56; j+=8) {	// will miss 5 columns
			trans(8, 3, j, i, M, N, A, B); 
		}
	}
	return;
}

// You can define additional transpose functions below.

char transpose_8x8_desc[] = "Transpose 8x8 block";
void transpose_8x8(int M, int N, int A[N][M], int B[M][N])
{
	for (int ii=0; ii<N; ii+=4) {
		for (int jj=0; jj<M; jj+=4) {
			for (int i=ii; i<ii+4; i++) {
				for (int j=jj; j<jj+4; j++) {
					B[j][i] = A[i][j];
				}
			}
		}
	}
}


/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
	/* Register your solution function */
	registerTransFunction(transpose_submit, transpose_submit_desc); 

	/* Register any additional transpose functions */
	//	registerTransFunction(transpose_8x8, transpose_8x8_desc);

}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
	int i, j;

	for (i = 0; i < N; i++) {
		for (j = 0; j < M; ++j) {
			if (A[i][j] != B[j][i]) {
				return 0;
			}
		}
	}
	return 1;
}
