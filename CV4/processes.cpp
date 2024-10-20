#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include <string.h>

#define PIPES_CNT 3
#define BUFFER_SIZE 100

void generate_numbers(int write_pipe, int N) {
    printf("PID %d: Start generating numbers.\n", getpid());
    srand(getpid());
    
    char buffer[BUFFER_SIZE];
    for (int i = 0; i < N; i++) {
        int number = rand() % 1001;  // <0 - 1000>
        snprintf(buffer, BUFFER_SIZE, "%d. %d", i + 1, number);
        write(write_pipe, buffer, strlen(buffer) + 1);
        usleep(100000);  // Pause 100 ms
    }
    close(write_pipe);
    printf("PID %d: Number generation finished.\n", getpid());
}

void add_operation(int read_pipe, int write_pipe) {
    int cnt = 0;
    char buffer[BUFFER_SIZE], sign = '+';
    srand(getpid());
    printf("PID %d: Start adding operation.\n", getpid());
    while (read(read_pipe, buffer, BUFFER_SIZE) > 0) {
        int number;
        if(cnt % 2 == 0) {
            sign = '+';
        } else {
            sign = '-';
        }

        sscanf(buffer, "%*d. %d", &number);
        snprintf(buffer + strlen(buffer), BUFFER_SIZE - strlen(buffer), "%c%d", sign, rand() % 1001);
        write(write_pipe, buffer, strlen(buffer) + 1);
        cnt++;
    }
    close(read_pipe);
    close(write_pipe);
    printf("PID %d: Operation adding finished.\n", getpid());
}

void calculate_result(int read_pipe, int write_pipe) {
    char buffer[BUFFER_SIZE];
    printf("PID %d: Start calculating result.\n", getpid());
    
    while (read(read_pipe, buffer, BUFFER_SIZE) > 0) {
        int number1 = 0, number2 = 0;
        char operation;

        if (sscanf(buffer, "%*d. %d%c%d", &number1, &operation, &number2) == 3) {
            // printf("PID %d: Parsed numbers: %d %c %d\n", getpid(), number1, operation, number2);
            int result = 0;
            
            if (operation == '+') {
                result = number1 + number2;
            } else if(operation == '-') {
                result = number1 - number2;
            } else {
                fprintf(stderr, "Unsupported operation: %c\n", operation);
            }

            snprintf(buffer + strlen(buffer), BUFFER_SIZE - strlen(buffer), "=%d\n", result);
            write(write_pipe, buffer, strlen(buffer) + 1);
        } else {
            fprintf(stderr, "Error parsing expression: %s\n", buffer);
        }
    }

    close(read_pipe);
    close(write_pipe);
    printf("PID %d: Calculation finished.\n", getpid());
}

void display_results(int read_pipe) {
    char buffer[BUFFER_SIZE];
    printf("PID %d: Start displaying results.\n", getpid());
    while (read(read_pipe, buffer, BUFFER_SIZE) > 0) {
        printf("PID %d: Result: %s", getpid(), buffer);
    }
    close(read_pipe);
    printf("PID %d: Displaying results finished.\n", getpid());
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <number of values>\n", argv[0]);
        exit(1);
    }

    int N = atoi(argv[1]);
    int pipes[PIPES_CNT][2];

    for (int i = 0; i < PIPES_CNT; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe");
            exit(1);
        }
    }

    if (fork() == 0) {
        close(pipes[0][0]);
        close(pipes[1][0]);
        close(pipes[1][1]);
        close(pipes[2][0]);
        close(pipes[2][1]);
        generate_numbers(pipes[0][1], N);
        exit(0);
    }

    if (fork() == 0) {
        close(pipes[0][1]);
        close(pipes[1][0]);
        close(pipes[2][0]);
        close(pipes[2][1]);
        add_operation(pipes[0][0], pipes[1][1]);
        exit(0);
    }

    if (fork() == 0) {
        close(pipes[0][0]);
        close(pipes[0][1]);
        close(pipes[1][1]);
        close(pipes[2][0]);
        calculate_result(pipes[1][0], pipes[2][1]);
        exit(0);
    }

    close(pipes[0][0]);
    close(pipes[0][1]);
    close(pipes[1][0]);
    close(pipes[1][1]);
    close(pipes[2][1]);
    display_results(pipes[2][0]);

    for (int i = 0; i < 3; i++) {
        wait(NULL);
    }

    printf("Parent: All children are finished.\n");
    return 0;
}
