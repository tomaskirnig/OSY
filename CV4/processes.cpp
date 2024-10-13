#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>

#define PIPES_CNT 2

// Generování náhodných čísel
void generate_numbers(int write_pipe, int N) {
    int num[2];
    printf("PID %d: Začátek generování čísel.\n", getpid());
    srand(time(NULL));
    for (int i = 0; N == -1 || i < N; i++) {
        num[0] = rand() % 101;  // <0 - 100>
        num[1] = rand() % 101;
        write(write_pipe, num, 2 * sizeof(int));  // Pošlese čísla do roury
        usleep(100000);  // Pauza 100 ms
    }
    close(write_pipe);
    printf("PID %d: Generování čísel ukončeno.\n", getpid());
}

// Čtení čísel z roury a jejich zobrazení
void add_numbers(int read_pipe, int write_pipe) {
    int num[2], sum;
    printf("PID %d: Začátek sčítání čísel.\n", getpid());
    while (read(read_pipe, num, 2 * sizeof(int)) > 0) {
        printf("PID %d: Čísla: %d, %d\n", getpid(), num[0], num[1]);
        sum = num[0] + num[1];
        write(write_pipe, &sum, sizeof(int));
    }
    close(read_pipe);
    close(write_pipe);
    printf("PID %d: Sčítání čísel ukončeno.\n", getpid());
}

void display_sum(int read_pipe) {
    int sum;
    printf("PID %d: Začátek zobrazení součtů.\n", getpid());
    while (read(read_pipe, &sum, sizeof(int)) > 0) {
        printf("PID %d: Součet: %d\n", getpid(), sum);
    }
    close(read_pipe);
    printf("PID %d: Zobrazení součtů ukončeno.\n", getpid());
}

int main(int argc, char *argv[]) {
    int N = -1;  
    if (argc == 2) {
        N = atoi(argv[1]);  
    }

    int pipes[PIPES_CNT][2];
    for(int i = 0; i < PIPES_CNT; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe");
            exit(1);
        }
    }

    // Vytvoření prvního potomka 
    if (fork() == 0) {
        close(pipes[0][0]);  // Zavře se čtecí konec roury v potomkovi
        close(pipes[1][0]);  // Zavře se čtecí konec roury v potomkovi
        close(pipes[1][1]);  // Zavře se čtecí konec roury v potomkovi
        generate_numbers(pipes[0][1], N);
        return 0;
    }

    // Vytvoření druhého potomka 
    if (fork() == 0) {
        close(pipes[0][1]);  // Zavřeme zapisovací konec roury v potomkovi
        close(pipes[1][0]);  // Zavřeme čtecí konec roury v potomkovi
        add_numbers(pipes[0][0], pipes[1][1]); 
        return 0; 
    }

    if(fork() == 0) {
        close(pipes[0][0]);
        close(pipes[0][1]);
        close(pipes[1][1]);
        display_sum(pipes[1][0]);
        return 0;
    }

    // Rodič: Zavřou se oba konce roury a čeká se na oba potomky
    close(pipes[0][0]);
    close(pipes[0][1]);
    close(pipes[1][0]);
    close(pipes[1][1]);
    wait(NULL);  // Čekání na jednoho potomka
    wait(NULL);  // Druhého potomka
    wait(NULL);  // Třetího potomka

    printf("Rodič: Potomci jsou hotovi.\n");
    return 0;
}
