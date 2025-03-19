/*1. We want to do a multithreaded computation of normalising a vector u to make it
unitary, and storing it in unorm. The sequential code is given below as an example,
and consists on a first step computing the sum of its squares, and a second step
dividing each component of the vector by the square root of the norm. Notice that any
step 2 must be done only after step 1 has been fully completed.
void normaliseVectorSequential(double ∗u, double ∗u_norm , int N) {
// Step 1 : compute u_norm_2 := || v || ˆ 2
double u_norm_2 = 0 ;
for ( int i = 0 ; i < N ; ++i )
u_norm_2 += vNext [ i ] ∗ vNext [ i ] ;
// Step 2 : compute vNext = vNext / sqrt ( u_norm_2 )
for ( int i = 0 ; i < N ; ++i )
u_norm [ i ] = u [ i ] / sqrt ( u_norm_2 )
}
If you have problems with the sqrt, use -lm flag when you run the code in the
terminal.
a) Create a threaded version of the code above, creating a thread for each element of
the vector. In this exercise, you will create two sets of threads.
b) Implement a barrier, as seen in class. Remember that the barrier will block until all
threads have reached it, and then release all waiting threads simultaneously. You
can find the struct and functions of barriers that you need to implement in the
cheatsheet.
c) Use a barrier implementation of the normalisation, where both steps are done in a
single function. What are the advantages when compared to the solution of the first
part of this problem?*/

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h> 
#include <unistd.h>
#include <math.h>

#define N 4  // Corrected vector size

double u[N] = {2, 3, 4, 7};  // Input vector
double u_norm[N];  // Normalized vector
double u_norm_2 = 0;  // Sum of squares

pthread_mutex_t mutex;
pthread_barrier_t barrier_1, barrier_2;

// Thread function for normalization
void* normaliseVector(void* arg) {
    int index = *(int*)arg;

    pthread_barrier_wait(&barrier_1);
    // Step 1: Compute sum of squares
    pthread_mutex_lock(&mutex);
    u_norm_2 += u[index] * u[index];  // Corrected indexing
    pthread_mutex_unlock(&mutex);

    // Wait for all threads to reach the barrier before proceeding
    pthread_barrier_wait(&barrier_2);

    // Step 2: Compute normalized values
    u_norm[index] = u[index] / sqrt(u_norm_2);

    // Print results
    printf("Normalized[%d] = %f\n", index, u_norm[index]);
    pthread_mutex_unlock(&mutex);
    return NULL;
}

int main() {
    pthread_t threads[N];
    int index[N];

    // Initialize barrier and mutex
    pthread_barrier_init(&barrier_1, NULL, N);
    pthread_barrier_init(&barrier_2, NULL, N);
    pthread_mutex_init(&mutex, NULL);

    // Create threads
    for (int i = 0; i < N; i++) {
        index[i] = i;  // Assign index values
        pthread_create(&threads[i], NULL, normaliseVector, &index[i]);
    }

    // Wait for all threads to finish
    for (int i = 0; i < N; i++) {
        pthread_join(threads[i], NULL);
    }

    // Destroy barrier and mutex
    pthread_barrier_destroy(&barrier_1);
    pthread_barrier_destroy(&barrier_2);
    pthread_mutex_destroy(&mutex);

    return 0;
}
