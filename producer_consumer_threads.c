#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include "semaphore.h" 

#define BUFFER_SIZE 10  // Buffer size

// Buffers
int buffer_1[BUFFER_SIZE];
int buffer_2[BUFFER_SIZE];
int count_1 = 0, count_2 = 0;
int in_1 = 0, out_1 = 0;
int in_2 = 0, out_2 = 0;

// Mutexes and condition variables
pthread_mutex_t mutex_1;
pthread_mutex_t mutex_2;
pthread_mutex_t print_mutex;  // For correct output order
pthread_cond_t print_done;  // Condition to sync producer & consumer

// Semaphores
Semaphore full_1, empty_1;
Semaphore full_2, empty_2;

int stop = 0; 
int print_ready = 1;  //To sync producer and consumer

// Correct Fibonacci function
int fib(int n) {
    if (n == 0) return 0;
    if (n == 1) return 1;
    
    int fib_i_1 = 1; // F(1)
    int fib_i_2 = 0; // F(0)
    int fib_i;   // This will hold the current Fibonacci number
    
    for (int i = 2; i <= n; ++i) { // Start from 2 since we already know F(0) and F(1)
        fib_i = fib_i_1 + fib_i_2; // Calculate the current Fibonacci number
        fib_i_2 = fib_i_1;         // Update fib_i_2 to the previous fib_i_1
        fib_i_1 = fib_i;           // Update fib_i_1 to the current fib_i
    }
    return fib_i; // Return the nth Fibonacci number
}

// Producer: Reads numbers from user
void* producer(void* arg) {
    char input[100];
    while (1) {
        // Wait for consumer to print before asking for a new number
        pthread_mutex_lock(&print_mutex);
        while (!print_ready) {
            pthread_cond_wait(&print_done, &print_mutex);
        }
        print_ready = 0;  // Block until the next Fibonacci result is printed
        pthread_mutex_unlock(&print_mutex);

        // Now ask for input
        printf("Enter number (or 'exit' to stop): ");
        scanf("%s", input);

        if (strcmp(input, "exit") == 0) {
            stop = 1;
            sem_post(&full_1);  // Wake up consumer-producer
            sem_post(&full_2);  // Wake up consumer
            return NULL; 
        }

        int num = atoi(input);

        sem_wait(&empty_1);  // Wait for space in buffer_1
        pthread_mutex_lock(&mutex_1); 
        buffer_1[in_1] = num; 
        in_1 = (in_1 + 1) % BUFFER_SIZE; // ensuring the buffer is circular
        count_1++;
        pthread_mutex_unlock(&mutex_1);
        sem_post(&full_1);
    }
}

// Consumer-Producer: Reads buffer_1, computes Fibonacci, writes to buffer_2
void* consumer_producer(void* arg) {
    while (1) {
        sem_wait(&full_1);
        pthread_mutex_lock(&mutex_1);

        // Check if the producer has finished
        if (count_1 == 0 && stop) {
            pthread_mutex_unlock(&mutex_1);
            return NULL;
        }

        int num = buffer_1[out_1];
        out_1 = (out_1 + 1) % BUFFER_SIZE;
        count_1--;
        pthread_mutex_unlock(&mutex_1);
        sem_post(&empty_1);

        // Compute Fibonacci
        int fib_result = fib(num);

        sem_wait(&empty_2);
        pthread_mutex_lock(&mutex_2);
        buffer_2[in_2] = fib_result;
        in_2 = (in_2 + 1) % BUFFER_SIZE;
        count_2++;
        pthread_mutex_unlock(&mutex_2);
        sem_post(&full_2);
    }
}

// Consumer: Reads buffer_2 and prints results
void* consumer(void* arg) {
    while (1) {
        sem_wait(&full_2);
        pthread_mutex_lock(&mutex_2);

        if (count_2 == 0 && stop) {
            pthread_mutex_unlock(&mutex_2);
            return NULL;
        }

        int result = buffer_2[out_2];
        out_2 = (out_2 + 1) % BUFFER_SIZE;
        count_2--;
        pthread_mutex_unlock(&mutex_2);
        sem_post(&empty_2);

        // Print result
        printf("Fibonacci result: %d\n", result);

        // Signal the producer that it can ask for new input
        pthread_mutex_lock(&print_mutex);
        print_ready = 1;
        pthread_cond_signal(&print_done);
        pthread_mutex_unlock(&print_mutex);
    }
}

int main() {
    pthread_t producer_thread, consumer_producer_thread, consumer_thread; // Threads

    // Initialize mutexes and condition variables
    pthread_mutex_init(&mutex_1, NULL);
    pthread_mutex_init(&mutex_2, NULL);
    pthread_mutex_init(&print_mutex, NULL); 
    pthread_cond_init(&print_done, NULL);  

    // Initialize semaphores
    sem_init(&full_1, 0);
    sem_init(&empty_1, BUFFER_SIZE);
    sem_init(&full_2, 0);
    sem_init(&empty_2, BUFFER_SIZE);

    // Create threads
    pthread_create(&producer_thread, NULL, producer, NULL);
    pthread_create(&consumer_producer_thread, NULL, consumer_producer, NULL);
    pthread_create(&consumer_thread, NULL, consumer, NULL);

    // Wait for threads to finish
    pthread_join(producer_thread, NULL);
    pthread_join(consumer_producer_thread, NULL);
    pthread_join(consumer_thread, NULL);

    // Destroy semaphores and mutexes
    sem_destroy(&full_1);
    sem_destroy(&empty_1);
    sem_destroy(&full_2);
    sem_destroy(&empty_2);
    pthread_mutex_destroy(&mutex_1);
    pthread_mutex_destroy(&mutex_2);
    pthread_mutex_destroy(&print_mutex);
    pthread_cond_destroy(&print_done); 

    printf("Program terminated successfully.\n");
    return 0;
}
