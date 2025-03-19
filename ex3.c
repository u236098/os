/*3. Write a program whose main will create seven processes that will search for “gold” in
a text file. The name of the file that each child will open will be passed through a pipe
( one pipe for each child process). Then, the children will search for appearances of
the substring “gold” in their file, and whenever they find it, they will send it to the main
through another pipe, indicating the position and filename where it was found (this
pipe shared among the different children). To pass information you can use the
structure GoldPosition below. Then, the main will read from the pipe all instances of
found gold, and write them in the standard ioutput. Remember to use the adequate
functions to compare and copy strings, and that strings need to have a ’\0’ at the end.
a) Create a function checkStringAtPos that, given a file descriptor, a position
and a string, returns 1 if the file contains that string at the given position, -1 if
the string would fall outside of the range of the file, and 0 otherwise.
b) Using the previous function, implement the described code for the main and
the children. Remember to properly close all pipes.
typedef struct { // you can modify i f you want
char fileName [100] ;
int pos ;
} Gold Position;*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

#define NUM_CHILDREN 7  // Number of child processes
#define MAX_FILENAME 100
#define TARGET_WORD "gold"

// Struct to store results
typedef struct {
    char fileName[MAX_FILENAME];
    int pos;
} GoldPosition;

// Function to check if the given position in a file contains "gold"
int checkStringAtPos(int fd, int pos, const char *target) {
    char buffer[strlen(target) + 1];  // Buffer to hold the read text
    lseek(fd, pos, SEEK_SET);  // Move file cursor to position

    int bytesRead = read(fd, buffer, strlen(target));  // Read from file
    if (bytesRead < strlen(target)) {
        return -1;  // Out of bounds
    }
    buffer[strlen(target)] = '\0';  // Null terminate for comparison

    if (strcmp(buffer, target) == 0) {
        return 1;  // Found "gold"
    }
    return 0;  // Not found
}

// Child process function
void childProcess(int readPipe, int writePipe) {
    char fileName[MAX_FILENAME];

    // Read filename from the parent process
    read(readPipe, fileName, MAX_FILENAME);

    // Open file
    int fd = open(fileName, O_RDONLY);
    if (fd < 0) {
        perror("Error opening file");
        exit(1);
    }

    int pos = 0;
    char buffer;
    while (read(fd, &buffer, 1) > 0) {  // Read file character by character
        if (checkStringAtPos(fd, pos, TARGET_WORD) == 1) {
            GoldPosition result;
            strcpy(result.fileName, fileName);
            result.pos = pos;

            // Send result to parent process
            write(writePipe, &result, sizeof(GoldPosition));
        }
        pos++;
    }

    close(fd);
    close(readPipe);
    close(writePipe);
    exit(0);
}

// Parent process function
int main() {
    int filePipes[NUM_CHILDREN][2];  // Pipes for sending file names to children
    int resultPipe[2];  // Pipe for receiving results from children

    // Create shared pipe for receiving results
    if (pipe(resultPipe) == -1) {
        perror("Error creating result pipe");
        exit(1);
    }

    // Create child processes
    for (int i = 0; i < NUM_CHILDREN; i++) {
        if (pipe(filePipes[i]) == -1) {
            perror("Error creating file pipe");
            exit(1);
        }

        pid_t pid = fork();
        if (pid < 0) {
            perror("Fork failed");
            exit(1);
        }

        if (pid == 0) {  // Child process
            close(filePipes[i][1]);  // Close write end of file pipe
            close(resultPipe[0]);  // Close read end of result pipe
            childProcess(filePipes[i][0], resultPipe[1]);  // Call child function
        } else {  // Parent process
            close(filePipes[i][0]);  // Close read end of file pipe
        }
    }

    // Send file names to children
    char *files[NUM_CHILDREN] = {"mine_0.txt","mine_1.txt", "mine_2.txt", "mine_3.txt", "mine_4.txt", "mine_5.txt", "mine_6.txt"};
    for (int i = 0; i < NUM_CHILDREN; i++) {
        write(filePipes[i][1], files[i], MAX_FILENAME);
        close(filePipes[i][1]);  // Close write end after sending filename
    }

    close(resultPipe[1]);  // Close write end of result pipe (parent only reads)

    // Read results from child processes
    GoldPosition result;
    printf("Occurrences of 'gold':\n");
    while (read(resultPipe[0], &result, sizeof(GoldPosition)) > 0) {
        printf("Found 'gold' in %s at position %d\n", result.fileName, result.pos);
    }

    close(resultPipe[0]);  // Close read end of result pipe

    // Wait for all child processes to finish
    for (int i = 0; i < NUM_CHILDREN; i++) {
        wait(NULL);
    }

    return 0;
}
