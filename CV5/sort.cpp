#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

void error_and_exit(const char* msg) {
    fprintf(stderr, "%s\n", msg);
    exit(EXIT_FAILURE);
}

int main() {
    int pipes[2][2];

    // Create pipes
    for(int i = 0; i < 2; i++) {
        if (pipe(pipes[i]) == -1) {
            error_and_exit("pipe creation failed");
        }
    }

    // First child to execute 'sort'
    if (fork() == 0) {
        close(pipes[0][0]);
        close(pipes[1][0]);
        close(pipes[1][1]);

        int input_fd = open("names.txt", O_RDONLY);
        if (input_fd == -1) {
            error_and_exit("open names.txt failed");
        }

        // Redirect stdin to read from names.txt
        if (dup2(input_fd, STDIN_FILENO) == -1) {
            error_and_exit("dup2 for sort failed");
        }
        close(input_fd);

        // Redirect stdout to write to pipe[0][1]
        if (dup2(pipes[0][1], STDOUT_FILENO) == -1) {
            error_and_exit("dup2 for sort failed");
        }
        close(pipes[0][1]);

        // Execute 'sort'
        execlp("sort", "sort", NULL);
        error_and_exit("execlp sort failed");
    }

    // Second child to execute 'tr [a-z] [A-Z]'
    if (fork() == 0) {
        close(pipes[0][1]);
        close(pipes[1][0]);

        // Redirect stdin to read from pipe[0][0]
        if (dup2(pipes[0][0], STDIN_FILENO) == -1) {
            error_and_exit("dup2 for tr failed");
        }
        close(pipes[0][0]);

        // Redirect stdout to write to pipes[1][1]
        if (dup2(pipes[1][1], STDOUT_FILENO) == -1) {
            error_and_exit("dup2 for tr failed");
        }
        close(pipes[1][1]);

        // Execute 'tr [a-z] [A-Z]'
        execlp("tr", "tr", "[a-z]", "[A-Z]", NULL);
        error_and_exit("execlp tr failed");
    }

    // Third child to execute 'nl -s ". "'
    if (fork() == 0) {
        close(pipes[0][0]);
        close(pipes[0][1]);
        close(pipes[1][1]);

        // Redirect stdin to read from pipes[1][0]
        if (dup2(pipes[1][0], STDIN_FILENO) == -1) {
            error_and_exit("dup2 for nl failed");
        }

        close(pipes[1][0]);

        // Execute 'nl -s ". "'
        execlp("nl", "nl", "-s", ". ", NULL);
        error_and_exit("execlp nl failed");
    }

    close(pipes[0][0]);
    close(pipes[0][1]);
    close(pipes[1][0]);
    close(pipes[1][1]);

    wait(NULL);
    wait(NULL);
    wait(NULL);

    return 0;
}
