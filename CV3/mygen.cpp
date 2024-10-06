#include <stdio.h>
#include <stdlib.h>
// #include <stdbool.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>

volatile sig_atomic_t running = 1;

// Handle Ctrl+C signal
void handle_sigint(int sig) {
    running = 0;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s N L\n", argv[0]);
        return 1;
    }

    int N = atoi(argv[1]);  // Number of numbers per line
    int L = atoi(argv[2]);  // Number of lines per minute

    if (N <= 0 || L <= 0) {
        fprintf(stderr, "N and L must be positive integers.\n");
        return 1;
    }

    // Convert L (lines per minute) to microseconds for the sleep function
    int sleep_time = 60000000 / L;  // microseconds per line

    srand(time(NULL)); 

    // Set up signal handler for Ctrl+C
    signal(SIGINT, handle_sigint);

    while (running) {
        for (int i = 0; i < N; ++i) {
            printf("%d ", rand() % 1000);  //  <0 ; 999>
        }
        printf("\n");
        fflush(stdout);  // Ensure the output is written to the file immediately
        usleep(sleep_time);  // Sleep for calculated microseconds between lines
    }

    return 0;
}
