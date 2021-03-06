/**
* This version is stamped on May 10, 2016
*
* Contact:
*   Louis-Noel Pouchet <pouchet.ohio-state.edu>
*   Tomofumi Yuki <tomofumi.yuki.fr>
*
* Web address: http://polybench.sourceforge.net
*/
/* cholesky.c: this file is part of PolyBench/C */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <pthread.h>
// #include "pthread_barrier.h"
#include "util.h"
#include "papi.h"

/* Include polybench common header. */
#include <polybench.h>

/* Include benchmark-specific header. */
#include "cholesky.h"

pthread_barrier_t barrier;
pthread_mutex_t lock;

double **I, **O;
double **Aux;
int size;
int cut;
int nthreads;
int world_rank;

/* Array initialization. */
static void init_array(int n, DATA_TYPE POLYBENCH_2D(A,N,N,n,n)){
I = (double **)malloc(n * sizeof(double));
O = (double **)calloc(n, sizeof(double));
Aux = (double **)malloc(n * sizeof(double));
int i, j;
for (i = 0; i < n; i++){
I[i] = (double*)malloc(n * sizeof(double));
O[i] = (double*)calloc(n, sizeof(double));
Aux[i] = (double*)malloc(n * sizeof(double));
for (j = 0; j <= i; j++){
A[i][j] = (DATA_TYPE)(-j % n) / n + 1;
I[i][j] = (DATA_TYPE)(-j % n) / n + 1;
}
for (j = i+1; j < n; j++) {
A[i][j] = 0;
I[i][j] = A[i][j];
}
A[i][i] = 1;
I[i][i] = A[i][i];
}

/* Make the matrix positive semi-definite. */
int r,s,t;

for (r = 0; r < n; ++r)
for (s = 0; s < n; ++s)
Aux[r][s] = 0;
for (t = 0; t < n; ++t)
for (r = 0; r < n; ++r)
for (s = 0; s < n; ++s)
Aux[r][s] += I[r][t] * I[s][t];
for (r = 0; r < n; ++r)
for (s = 0; s < n; ++s){
	I[r][s] = Aux[r][s];
}
free2D(Aux);
}


/* DCE code. Must scan the entire live-out data.
Can be used also to check the correctness of the output. */
static void print_array(int n, DATA_TYPE POLYBENCH_2D(A,N,N,n,n)){
int i, j;

POLYBENCH_DUMP_START;
POLYBENCH_DUMP_BEGIN("A");
for (i = 0; i < n; i++)
	for (j = 0; j <= i; j++) {
		if ((i * n + j) % 20 == 0) fprintf (POLYBENCH_DUMP_TARGET, "\n");
		fprintf (POLYBENCH_DUMP_TARGET, DATA_PRINTF_MODIFIER, A[i][j]);
	}
	POLYBENCH_DUMP_END("A");
	POLYBENCH_DUMP_FINISH;
}

static void kernel_cholesky_row(int id){

	int i, j, k;

	int start = id * cut;
	int end = minimum(start + cut, size);

	for (k = 0; k < size; k++) {

		if(id == 0){
			I[k][k] = SQRT_FUN(I[k][k]);
			for (j = 0; j < k; j++) {
				I[k][j] /= I[j][j];
				I[j][k] = I[k][j];
			}
		}

		pthread_barrier_wait(&barrier);

		for(i = start + (k + 1); i < end + k + 1; i++){
			for(j = i; j < size; j++){
				I[i][j] -=  I[k][i] *  I[k][j];
				I[j][i] = I[i][j];
			}
		}
		pthread_barrier_wait(&barrier);
	}
	pthread_barrier_wait(&barrier);

	for(i = start; i < end; i++){
		for(j = i + 1; j < size; j++){
			I[i][j] = 0.0;
		}
	}
}

void cholesky_mpi(){

	/* Calculate trail for threads */
	cut = (int)ceil(((float)size) / nthreads);

	/*Initialize MPI*/
	MPI_Init(NULL, NULL);

	/*Initialize World Size*/
	int world_size;
	MPI_Comm_size(MPI_COMM_WORLD, &world_size);
	/*Initialize World Rank*/
	MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

	pthread_t thread[nthreads];

	int arg[nthreads];

	// for (int i = 0; i < nthreads; i++) {
	// 	arg[i] = i;
	// 	pthread_create(&thread[i], NULL, kernel_cholesky_row, &arg[i]);
	// }

	// for (int i = 0; i < nthreads; i++) {
	// 	pthread_join(thread[i], NULL);
	// }

	/*Distribution of tasks*/
	if(world_rank == 0){
		for(int i = 1; i < world_size; i++){
			printf("I'm world_rank %d and i'm a task to world_rank = %d\n", world_rank);
			MPI_Send(&i, 1, MPI_INT, i, 1, MPI_COMM_WORLD);
		}


	} 

	kernel_cholesky_row(world_rank);
	/*Receiving tasks*/
	else{
		int i;
		printf("I'm receiving a task as world_rank = %d\n", world_rank);
		MPI_Recv(&i, 1, MPI_INT, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);		
	}

	// /*Receiving answers of tasks*/
	// for(int i = 1; i < world_size; i++){

	// 	MPI_Recv(&i, 1, MPI_INT, i, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);	
	// }

	MPI_Finalize();
}



int main(int argc, char** argv){

	// if(argc < 2){
	// 	printf("The program must have an argument to be executed. ./cholesky_pthread.out $nthreads\n");
	// 	return -1;
	// }

	nthreads = atoi(argv[1]);

	/* Retrieve problem size. */
	size = N;

	/* Variable declaration/allocation. */
	POLYBENCH_2D_ARRAY_DECL(A, DATA_TYPE, N, N, size, size);

	/* Initialize array(s). */
	init_array (size, POLYBENCH_ARRAY(A));

	/* Start timer. */
	polybench_start_instruments;
	// printMatrix(I, size);

	pthread_barrier_init(&barrier, NULL, nthreads);

	// BEGINTIME();

	cholesky_mpi();
	// kernel_cholesky (n, POLYBENCH_ARRAY(A));
	// printf("ELAPSED TIME: ");
	// ENDTIME();
	// printMatrix(I, size);

	// printMatrix(I, size);

	/* Stop and print timer. */
	polybench_stop_instruments;
	polybench_print_instruments;

	/* Prevent dead-code elimination. All live-out data must be printed
	by the function call in argument. */
	polybench_prevent_dce(print_array(size, POLYBENCH_ARRAY(A)));

	/* Be clean. */
	POLYBENCH_FREE_ARRAY(A);
	free2D(I);
	free2D(O);

	return 0;
}
