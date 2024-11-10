#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <stdarg.h>
#include <poll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>

#define MAX_PLAYERS 5
#define BUFFER_SIZE 128

typedef struct {
    int socket;
    sem_t semafor;
} Misto;

Misto g_mista[MAX_PLAYERS];
char g_slovo[BUFFER_SIZE] = "a";
pthread_mutex_t g_slovo_mutex = PTHREAD_MUTEX_INITIALIZER;

// Function to initialize the player slots and semaphores
void initialize_g_mista() {
    for (int i = 0; i < MAX_PLAYERS; i++) {
        g_mista[i].socket = -1;
        if (i == 0) {
            sem_init(&g_mista[i].semafor, 0, 1); // First semaphore initialized to 1
        } else {
            sem_init(&g_mista[i].semafor, 0, 0); // Others initialized to 0
        }
    }
}

// Function to find the next active player
int dalsi(int P) {
    int PD = P;
    do {
        PD = (PD + 1) % MAX_PLAYERS;
        if (g_mista[PD].socket != -1) {
            return PD;
        }
    } while (PD != P);
    return P; // Only one player is left
}

// Thread function for each player
void* hrac(void* arg) {
    int P = *(int*)arg;
    free(arg);
    int client_socket = g_mista[P].socket;

    while (1) {
        sem_wait(&g_mista[P].semafor);
        
        // Send the current word to the client
        pthread_mutex_lock(&g_slovo_mutex);
        write(client_socket, "Current word: ", sizeof("Current word: "));
        write(client_socket, g_slovo, strlen(g_slovo));
        write(client_socket, "\n", 1);
        pthread_mutex_unlock(&g_slovo_mutex);

        char buf[BUFFER_SIZE];
        int len;

        while (1) {
            len = read(client_socket, buf, sizeof(buf) - 1);
            if (len <= 0) {
                // Client disconnected unexpectedly
                printf("Client at position %d disconnected unexpectedly.\n", P);
                g_mista[P].socket = -1;
                int PD = dalsi(P);
                sem_post(&g_mista[PD].semafor);
                close(client_socket);
                pthread_exit(NULL);
            }
            buf[len] = '\0';
            buf[strcspn(buf, "\r\n")] = '\0'; // Remove newline characters

            if (strcmp(buf, "*") == 0) {
                // Client wants to disconnect
                printf("Client at position %d wants to disconnect.\n", P);
                g_mista[P].socket = -1;
                int PD = dalsi(P);
                sem_post(&g_mista[PD].semafor);
                close(client_socket);
                pthread_exit(NULL);
            }

            pthread_mutex_lock(&g_slovo_mutex);
            char last_char = g_slovo[strlen(g_slovo) - 1];
            pthread_mutex_unlock(&g_slovo_mutex);

            if (buf[0] == last_char) {
                // Valid word; update g_slovo
                pthread_mutex_lock(&g_slovo_mutex);
                strcpy(g_slovo, buf);
                pthread_mutex_unlock(&g_slovo_mutex);
                break;
            } else {
                write(client_socket, "Invalid word. Try again.\n", 25);
            }
        }

        int PD = dalsi(P);
        sem_post(&g_mista[PD].semafor);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s port_number\n", argv[0]);
        exit(1);
    }

    int port = atoi(argv[1]);
    initialize_g_mista();

    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("socket failed");
        exit(1);
    }

    int optval = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    memset(&(server_addr.sin_zero), '\0', 8);

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(struct sockaddr)) < 0) {
        perror("bind failed");
        close(server_socket);
        exit(1);
    }

    if (listen(server_socket, MAX_PLAYERS) < 0) {
        perror("listen failed");
        close(server_socket);
        exit(1);
    }

    printf("Server is listening on port %d\n", port);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t sin_size = sizeof(struct sockaddr_in);
        int client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &sin_size);
        if (client_socket < 0) {
            perror("accept failed");
            continue;
        }

        // Find first free position P
        int P = -1;
        for (int i = 0; i < MAX_PLAYERS; i++) {
            if (g_mista[i].socket == -1) {
                P = i;
                break;
            }
        }
        if (P == -1) {
            printf("No free slot for new client. Rejecting connection.\n");
            close(client_socket);
            continue;
        }

        g_mista[P].socket = client_socket;

        // Create new thread
        pthread_t thread;
        int *arg = (int*)malloc(sizeof(*arg));
        if (arg == NULL) {
            perror("malloc failed");
            close(client_socket);
            continue;
        }
        *arg = P;
        if (pthread_create(&thread, NULL, hrac, arg) != 0) {
            perror("pthread_create failed");
            free(arg);
            close(client_socket);
            continue;
        }
        pthread_detach(thread);

        printf("Client connected at position %d\n", P);
    }

    close(server_socket);
    return 0;
}
