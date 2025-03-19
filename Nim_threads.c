#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>

// Global variables
int turnA = 1; // Player A starts
pthread_mutex_t lock;
pthread_cond_t turn_cond;

// Function to read the current token count from `game.dat`
int read_tokens() {
    int fd = open("game.dat", O_RDONLY);
    if (fd == -1) {
        perror("Error opening game.dat");
        exit(1);
    }
    
    int tokens;
    read(fd, &tokens, sizeof(int));
    close(fd);
    return tokens;
}

// Function to update `game.dat` with the new token count
void write_tokens(int tokens) {
    int fd = open("game.dat", O_WRONLY | O_TRUNC);
    if (fd == -1) {
        perror("Error opening game.dat");
        exit(1);
    }
    
    write(fd, &tokens, sizeof(int));
    close(fd);
}

// Player A function
void *thread_player_A(void *arg) {
    int *values = (int *)arg;
    int decrement = values[0]; // to handle the decrement of each thread

    while (1) {
        pthread_mutex_lock(&lock);

        // Wait until it's player A's turn
        while (turnA == 0) {
            pthread_cond_wait(&turn_cond, &lock); // wait for the turn and unlock the mutex 
        }

        // Read tokens from `game.dat`
        int counter = read_tokens();

        // Check if game is over
        if (counter <= 0) {
            pthread_mutex_unlock(&lock);
            break;
        }

        // If enough tokens are available, decrease them
        if (counter >= decrement) {
            printf("Player A decreasing %d\n", decrement);
            counter -= decrement;
            printf("Counter = %d\n", counter);
            write_tokens(counter);
        }

        // If the last token is taken, declare the winner
        if (counter <= 0) {
            printf("Player A wins!\n");
            pthread_cond_broadcast(&turn_cond);
            pthread_mutex_unlock(&lock);
            exit(0);
        }

        // Change turn to Player B
        turnA = 0;
        pthread_cond_broadcast(&turn_cond);

        pthread_mutex_unlock(&lock);
        usleep(10000); // Prevent starvation
    }

    free(values);
    return NULL;
}

// Player B function
void *thread_player_B(void *arg) {
    int *values = (int *)arg;
    int decrement = values[0];

    while (1) {
        pthread_mutex_lock(&lock);

        // Wait until it's player B's turn
        while (turnA == 1) {
            pthread_cond_wait(&turn_cond, &lock);
        }

        // Read tokens from `game.dat`
        int counter = read_tokens();

        // Check if game is over
        if (counter <= 0) {
            pthread_mutex_unlock(&lock);
            break;
        }

        // If enough tokens are available, decrease them
        if (counter >= decrement) {
            printf("Player B decreasing %d\n", decrement);
            counter -= decrement;
            printf("Counter = %d\n", counter);
            write_tokens(counter);
        }

        // If the last token is taken, declare the winner
        if (counter <= 0) {
            printf("Player B wins!\n");
            pthread_cond_broadcast(&turn_cond);
            pthread_mutex_unlock(&lock);
            exit(0);
        }

        // Change turn to Player A
        turnA = 1;
        pthread_cond_broadcast(&turn_cond);

        pthread_mutex_unlock(&lock);
        usleep(10000); // Prevent starvation
    }

    free(values);
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <number_of_tokens>\n", argv[0]);
        return 1;
    }

    // Initialize game.dat with the provided argument
    int counter = atoi(argv[1]);
    if (counter <= 0) {
        printf("Number of tokens must be positive.\n");
        return 1;
    }

    // Create `game.dat` and write initial tokens
    int fd = open("game.dat", O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd == -1) {
        perror("Error creating game.dat");
        return 1;
    }
    write(fd, &counter, sizeof(int));
    close(fd);

    // Initialize mutex and condition variable
    pthread_mutex_init(&lock, NULL);
    pthread_cond_init(&turn_cond, NULL);

    // Create 6 threads (3 for Player A, 3 for Player B)
    pthread_t threads[6];
    int i;
    for (i = 0; i < 6; i++) {
        int *values = malloc(sizeof(int)); 
        *values = (i % 3) + 1; // from 1 to 3

        if (i < 3) {
            pthread_create(&threads[i], NULL, thread_player_A, values);
        } else {
            pthread_create(&threads[i], NULL, thread_player_B, values);
        }
    }

    // Wait for all threads to finish
    for (i = 0; i < 6; i++) {
        pthread_join(threads[i], NULL);
    }

    // Clean up
    pthread_mutex_destroy(&lock);
    pthread_cond_destroy(&turn_cond);

    return 0;
}
