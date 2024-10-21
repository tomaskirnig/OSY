#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <ctype.h>
#include <fcntl.h>

#define BUFFER_SIZE 1024

void error_and_exit(const char* msg) {
    fprintf(stderr, "%s\n", msg);
    exit(EXIT_FAILURE);
}

int main(int argc, char* argv[]) {
    if(argc != 3) {
        error_and_exit("Usage: ./sort <input_file> <character>");
    }

    char *filename = argv[1];
    char filter_char = argv[2][0];

    int pipes[3][2];

    // Create pipes
    for(int i = 0; i < 3; i++) {
        if (pipe(pipes[i]) == -1) {
            error_and_exit("pipe creation failed");
        }
    }

    // First child to mimics cat
    if (fork() == 0) {
        close(pipes[0][0]);
        close(pipes[1][0]);
        close(pipes[1][1]);
        close(pipes[2][0]);
        close(pipes[2][1]);

        int fd = open(filename, O_RDONLY);
        if (fd == -1) {
            error_and_exit("open file failed");
        }

        char buffer[BUFFER_SIZE];
        ssize_t bytes_read;
        ssize_t total_bytes = 0;

        while ((bytes_read = read(fd, buffer, BUFFER_SIZE)) > 0) {
            total_bytes += bytes_read;
            if (write(pipes[0][1], buffer, bytes_read) == -1) {
                error_and_exit("write to pipe[0][1] failed"); 
            }
        }

        close(fd);
        close(pipes[0][1]);

        // Print the total bytes to stderr
        fprintf(stderr, "Bytes passed: %ld\n", total_bytes);
        exit(EXIT_SUCCESS);

    }

    // Second child to execute 'sort'
    if (fork() == 0) {
        close(pipes[0][1]);
        close(pipes[1][0]);
        close(pipes[2][0]);
        close(pipes[2][1]);

        // Redirect stdin to read from pipes[0][0]
        if (dup2(pipes[0][0], STDIN_FILENO) == -1) {
            error_and_exit("dup2 for sort failed 1");
        }
        close(pipes[0][0]);

        // Redirect stdout to write to pipe[1][0]
        if (dup2(pipes[1][1], STDOUT_FILENO) == -1) {
            error_and_exit("dup2 for sort failed 2");
        }
        close(pipes[1][1]);

        // Execute 'sort'
        execlp("sort", "sort", NULL);
        error_and_exit("execlp sort failed");
    }

    // Third child to execute 'tr [a-z] [A-Z]'
    if (fork() == 0) {
        close(pipes[0][0]);
        close(pipes[0][1]);
        close(pipes[1][1]);
        close(pipes[2][0]);

        // Redirect stdout to write to pipes[2][1]
        if (dup2(pipes[2][1], STDOUT_FILENO) == -1) {
            error_and_exit("dup2 for grep failed");
        }
        close(pipes[2][1]);


        char buffer[BUFFER_SIZE];
        ssize_t bytes_read;
        char line[BUFFER_SIZE];
        int line_index = 0;

        while ((bytes_read = read(pipes[1][0], buffer, sizeof(buffer))) > 0) {
            for (int i = 0; i < bytes_read; i++) {
                if (buffer[i] == '\n') {
                    line[line_index] = '\0'; // End of the line 

                    // Check if the line contains the specified character
                    if (!strchr(line, toupper(filter_char)) && !strchr(line, tolower(filter_char))) {
                        // If it doesn't contain the character, write the line to stdout
                        write(STDOUT_FILENO, line, line_index);
                        write(STDOUT_FILENO, "\n", 1);
                    }

                    line_index = 0; // Reset line index for the next line
                } else {
                    line[line_index] = buffer[i];
                    line_index++;
                }
            }
        }

        close(pipes[1][0]);
        exit(EXIT_SUCCESS);
    }

    // Fourth child to execute 'nl -s ". "'
    if (fork() == 0) {
        close(pipes[0][0]);
        close(pipes[0][1]);
        close(pipes[1][0]);
        close(pipes[1][1]);
        close(pipes[2][1]);

        // Redirect stdin to read from pipes[2][0]
        if (dup2(pipes[2][0], STDIN_FILENO) == -1) {
            error_and_exit("dup2 for nl failed");
        }

        close(pipes[2][0]);

        // Execute 'nl -s ". "'
        execlp("nl", "nl", "-s", ". ", NULL);
        error_and_exit("execlp nl failed");
    }

    close(pipes[0][0]);
    close(pipes[0][1]);
    close(pipes[1][0]);
    close(pipes[1][1]);
    close(pipes[2][0]);
    close(pipes[2][1]);

    wait(NULL);
    wait(NULL);
    wait(NULL);
    wait(NULL);

    return 0;
}
