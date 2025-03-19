/*2. The objective of this exercise is to implement a program that concurrently increases
and decreases a counter using threads. It involves a thread increasing the counter a
random number whenever it reaches 0, and three decreasing threads decreasing the
counter, each one decreasing the counter by 1, 5 and 10 units at a time respectively.
A code for the increase part is already given below, and consists of a single thread that
adds a random number to the counter reaches 0. Assume that there is a global variable
end, that will set to one to mark that the program will end when the counter reaches 0.
You do not need to worry about race conditions related to the end variable, and you
can assume it will change due to an external event.
int counter = 0 ;

int end = 0 ;
int increment ( void ∗ a ) {
while ( end == 0 ) {
if ( counter == 0 ){
counter += rand ( ) \%1000 ; // add a a random number
}
}
a) Are there any race conditions in the increment thread, when the decrement
threads are executing? If so, identify the variables involved, the critical region
(in lines) and which synchronisation tool you would use.
b) Implement the decrease function, that will decrease the counter by 1, 5 and 10
units at a time, depending on the thread executing it. Avoid race conditions
and starvation.
c) Implement the main program, that will create the 3 decreasing threads and the
increment thread, and then waits until all threads finish.*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

pthread_mutex_t mutex;
pthread_cond_t cond;
int counter = 0;
int end = 0;  // 1 when the program should terminate

// Increment function: Adds a random number when counter == 0
void *increment(void *arg) {
    while (1) {
        pthread_mutex_lock(&mutex);

        if (end == 1) {  // Stop the increment if the program is ending
            pthread_mutex_unlock(&mutex);
            return NULL;
        }

        if (counter == 0) {
            counter += rand() % 1000 + 1; // Add a random number (1 to 1000)
            printf("[Increment] Counter increased to: %d\n", counter);
            pthread_cond_broadcast(&cond);  // Wake up all decrement threads
        }

        pthread_mutex_unlock(&mutex);
        usleep(50000);  // Prevent CPU overuse
    }
    return NULL;
}

// Decrement function: Subtracts 1, 5, or 10 when counter > 0
void *decrement(void *arg) {
    int decrement_value = *(int*)arg;
    while (1) {
        pthread_mutex_lock(&mutex);

        while (counter == 0 && end == 0) {  // Wait until counter > 0
            pthread_cond_wait(&cond, &mutex);
        }

        if (end == 1) {  // If end is set, exit the thread
            pthread_mutex_unlock(&mutex);
            return NULL;
        }

        if (counter >= decrement_value) {
            counter -= decrement_value;
            printf("[Decrement] Counter decreased by %d, now: %d\n", decrement_value, counter);
        } else {
            counter = 0;  // Ensure counter never goes negative
        }

        if (counter == 0) {
            printf("[INFO] Counter reached 0. Ending program...\n");
            end = 1;  // Signal all threads to stop
            pthread_cond_broadcast(&cond);  // Wake up all threads to exit
        }

        pthread_mutex_unlock(&mutex);
        usleep(50000);  // Prevent CPU overuse
    }
    return NULL;
}

int main() {
    pthread_t inc_thread, dec_threads[3];
    int values[3] = {1, 5, 10};  // Decrement values

    // Initialize mutex and condition variable
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);

    // Create increment thread
    pthread_create(&inc_thread, NULL, &increment, NULL);

    // Create decrement threads
    for (int i = 0; i < 3; i++) {  
        pthread_create(&dec_threads[i], NULL, &decrement, &values[i]);
    }

    // Wait for decrement threads
    for (int i = 0; i < 3; i++) {
        pthread_join(dec_threads[i], NULL);
    }

    // Stop the increment thread
    end = 1;
    pthread_cond_broadcast(&cond);  // Wake up the increment thread to exit
    pthread_join(inc_thread, NULL);

    // Destroy mutex and condition variable
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);

    printf("[INFO] Program terminated successfully.\n");
    return 0;
}
