/*4. In this exercise, you will implement a multi-process program that uses pipes for inter-
process communication. The task is to read a series of numbers from a file, encoded

in ASCII as a string, convert them to a binary-encoded integer, and write them in a
second file, The code will be concurrent, using a father and a child processes. The
father will read numbers one by one from the input file, convert them to integer and
pass them through a pipe to the son. The son will read from the pipe and write each
int to the output file. The two processes will end when the input file has been
completely read, and all integers have been processed. The path of the input and
output files is given as the first and second command-line arguments.
a) Write a function that reads a string from a source identified by a filedescriptor, until a
”\0” byte marking the end of the string is read. You cannot use lseek, and you cannot
read ahead of the end of the string.

b) Implement the rest of the program, using the previous function. Make sure that you
avoid deadlocks related to the pipe communication.
Hint: remember that you can use sscanf or atoi to convert from string to integer and that you
can use “hexdump -e '1/4 "%d\n"' output.bin” for displaying the content of a binary file in
human-readable form.*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

// Buffer size for reading strings
#define BUFFER_SIZE 256

// Function to read a string from a file descriptor (ASCII input)
int readStringFromFD(int fd, char *buffer, int maxSize) {
    int index = 0;
    char ch;
    while (index < maxSize - 1) {
        int bytesRead = read(fd, &ch, 1);
        if (bytesRead <= 0) {
            break;  // End of file or read error
        }
        if (ch == '\0') {
            break;  // Stop at null terminator
        }
        buffer[index++] = ch;
    }
    buffer[index] = '\0';  // Null-terminate the string
    return index;  // Return number of bytes read
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <input_file> <output_file>\n", argv[0]);
        return 1;
    }

    char *inputFile = argv[1];
    char *outputFile = argv[2];

    int pipeFD[2]; // Pipe for communication
    if (pipe(pipeFD) == -1) {
        perror("Pipe failed");
        exit(1);
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("Fork failed");
        exit(1);
    }

    if (pid == 0) {
        // **Child Process (Writer)**
        close(pipeFD[1]);  // Close unused write end

        int outFD = open(outputFile, O_CREAT | O_WRONLY | O_TRUNC, 0644);
        if (outFD < 0) {
            perror("Error opening output file");
            exit(1);
        }

        int num;
        while (read(pipeFD[0], &num, sizeof(int)) > 0) {
            write(outFD, &num, sizeof(int));  // Write integer to binary file
        }

        close(pipeFD[0]);  // Close read end of pipe
        close(outFD);  // Close output file
        exit(0);
    } else {
        // **Parent Process (Reader)**
        close(pipeFD[0]);  // Close unused read end

        int inFD = open(inputFile, O_RDONLY);
        if (inFD < 0) {
            perror("Error opening input file");
            exit(1);
        }

        char buffer[BUFFER_SIZE];
        while (readStringFromFD(inFD, buffer, BUFFER_SIZE) > 0) {
            int num = atoi(buffer);  // Convert string to integer
            write(pipeFD[1], &num, sizeof(int));  // Send integer to child
        }

        close(inFD);  // Close input file
        close(pipeFD[1]);  // Close write end of pipe

        wait(NULL);  // Wait for child process to finish
        printf("Conversion complete. Output saved to %s\n", outputFile);
    }

    return 0;
}
