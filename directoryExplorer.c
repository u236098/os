#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

// Check if the filename has a .dat extension
int is_dat_file(const char *filename) {
    int len = strlen(filename);
    return (len > 4 && strcmp(filename + len - 4, ".dat") == 0); // Check if the last 4 characters are ".dat"
}

// Child process: Reads integers from a .dat file, sums them, and writes the sum to the pipe
void process_dat_file(const char *filepath, int pipe_fd) { 
    int fd = open(filepath, O_RDONLY);
    if (fd < 0) {
        perror("Error opening file");
        exit(1);
    }

    int num, sum = 0;
    while (read(fd, &num, sizeof(int)) > 0) { // Read integers from the file and sum them
        sum += num;
    }

    close(fd); 
    write(pipe_fd, &sum, sizeof(int)); // Send sum to the parent through the pipe

    close(pipe_fd);  //Close the write end of the pipe in the child process
    exit(0);
}

// Parent process: Explore the directory and fork child processes for each .dat file
void explore_directory(const char *dir_path, int pipe_fd) {
    DIR *dir = opendir(dir_path);
    if (!dir) {
        perror("Error opening directory");
        exit(1);
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue; // Skip current and parent directories **We added this to avoid infinite loop and unnecessary processing
        }

        char full_path[1024];
        sprintf(full_path, "%s/%s", dir_path, entry->d_name); // put the full path in the buffer full_path

        if (is_dat_file(entry->d_name)) { // Check if it's a .dat file
            pid_t pid = fork(); // Fork a child process
            if (pid < 0) { 
                perror("Error creating child process");
                exit(1);
            } else if (pid == 0) { 
                process_dat_file(full_path, pipe_fd); // Child process reads the .dat file and sends the sum to the parent
            }
        }
    }

    closedir(dir); 
    return;         
}

int main(int argc, char *argv[]) { 
    if (argc != 2) {
        printf("Usage: %s <directory_path>\n", argv[0]);
        return 1;
    }

    int fd[2]; // Pipe for communication
    if (pipe(fd) == -1) {
        perror("Error creating pipe");
        return 1;
    }

    explore_directory(argv[1], fd[1]); // Start processing without waiting for the child processes to finish

    close(fd[1]); //Parent closes the write end

    // Parent reads sums from the pipe
    int total_sum = 0, sum;
    while (read(fd[0], &sum, sizeof(int)) > 0) { 
        total_sum += sum;
    }

    close(fd[0]); //Parent closes the read end

    // Wait for all child processes to finish
    while (wait(NULL) > 0); // because wait returns -1 when there are no more child processes to wait for

    printf("Total sum of integers in .dat files: %d\n", total_sum);

    return 0;
}
