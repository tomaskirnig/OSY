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
#include <vector>
#include <mutex>
#include <algorithm>

#define NUM_OF_CLIENTS 20
#define BUFFER_SIZE 256

#define STR_CLOSE "close"
#define STR_QUIT "quit"

// Shared list of client sockets and mutex for synchronization
std::vector<int> client_sockets;
pthread_mutex_t lock; 

//***************************************************************************
// log messages
#define LOG_ERROR               0       // errors
#define LOG_INFO                1       // information and notifications
#define LOG_DEBUG               2       // debug messages

int g_debug = LOG_INFO;

void log_msg(int t_log_level, const char *t_form, ...)
{
    const char *out_fmt[] = {
        "ERR: (%d-%s) %s\n",
        "INF: %s\n",
        "DEB: %s\n"};
    if (t_log_level && t_log_level > g_debug)
        return;
    char l_buf[1024];
    va_list l_arg;
    va_start(l_arg, t_form);
    vsprintf(l_buf, t_form, l_arg);
    va_end(l_arg);
    switch (t_log_level)
    {
    case LOG_INFO:
    case LOG_DEBUG:
        fprintf(stdout, out_fmt[t_log_level], l_buf);
        break;
    case LOG_ERROR:
        fprintf(stderr, out_fmt[t_log_level], errno, strerror(errno), l_buf);
        break;
    }
}

// Function to evaluate expression with '+' and '-' operations
int evaluate_expression(const char *expr, int *result)
{
    int value = 0;
    int num = 0;
    int sign = 1; // 1 for '+', -1 for '-'
    const char *p = expr;
    while (*p != '\0' && *p != '\n')
    {
        if (*p == ' ')
        {
            // Skip spaces
            p++;
        }
        else if (*p >= '0' && *p <= '9')
        {
            num = 0;
            while (*p >= '0' && *p <= '9')
            {
                num = num * 10 + (*p - '0');
                p++;
            }
            value += sign * num;
        }
        else if (*p == '+')
        {
            sign = 1;
            p++;
        }
        else if (*p == '-')
        {
            sign = -1;
            p++;
        }
        else
        {
            // Invalid character
            return -1;
        }
    }
    *result = value;
    return 0;
}

// Broadcast message to all connected clients
void broadcast_to_clients(const char *message) {
    pthread_mutex_lock(&lock);
    for (int client_sock : client_sockets) {
        write(client_sock, message, strlen(message));
    }
    pthread_mutex_unlock(&lock);
}

// Handle client communication
void* handle_client(void *client_socket_ptr) {
    int client_socket = *(int*)client_socket_ptr;
    free(client_socket_ptr);  // Free dynamically allocated memory
    char buffer[BUFFER_SIZE];

    while (1) {
        int len = read(client_socket, buffer, BUFFER_SIZE - 1);
        if (len <= 0) {
            log_msg(1, "Client disconnected.");
            break;
        }
        buffer[len - 1] = '\0';  // Null-terminate the received string

        // log_msg(1, "Received: %s", buffer);

        int result;
        if (evaluate_expression(buffer, &result) == 0) {
            char response[BUFFER_SIZE];
            snprintf(response, sizeof(response), "%s = %d\n", buffer, result);
            broadcast_to_clients(response);
        } else {
            const char *error_msg = "Error: Invalid expression\n";
            write(client_socket, error_msg, strlen(error_msg));
        }
    }

    // Remove client from the list upon disconnection
    pthread_mutex_lock(&lock);
    auto it = std::remove(client_sockets.begin(), client_sockets.end(), client_socket);
    client_sockets.erase(it, client_sockets.end());
    pthread_mutex_unlock(&lock);

    close(client_socket);
    return nullptr;
}

int main(int argc, char *argv[]) {
    if (argc <= 1) {
        printf("Usage: %s port_number\n", argv[0]);
        exit(1);
    }
    int port = atoi(argv[1]);

    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Failed to create socket");
        exit(1);
    }

    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_socket);
        exit(1);
    }

    if (listen(server_socket, 5) < 0) {
        perror("Listen failed");
        close(server_socket);
        exit(1);
    }

    // Enable port reusability
    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("Failed to set SO_REUSEADDR");
        close(server_socket);
        exit(1);
    }

    log_msg(1, "Server will listen on port: %d", port);

    pthread_mutex_init(&lock, 0);

    // Setting up poll to monitor both stdin and the socket
    struct pollfd fds[2];
    fds[0].fd = server_socket;
    fds[0].events = POLLIN;
    fds[1].fd = STDIN_FILENO;
    fds[1].events = POLLIN;

    char l_buf[BUFFER_SIZE];

    while (1) {
        int poll_res = poll(fds, 2, -1); // Wait indefinitely for an event

        if (poll_res < 0) {
            perror("Poll error");
            break;
        }

        // Check for incoming client connections
        if (fds[0].revents & POLLIN) {
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            int client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
            if (client_socket < 0) {
                perror("Failed to accept client");
                continue;
            }

            printf("Client connected: %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

            // Add client socket to the list
            pthread_mutex_lock(&lock);
            client_sockets.push_back(client_socket);
            pthread_mutex_unlock(&lock);

            // Create a thread for the new client
            int *client_sock_ptr = (int*)malloc(sizeof(int));
            *client_sock_ptr = client_socket;

            pthread_t thread;
            if (pthread_create(&thread, NULL, handle_client, client_sock_ptr) != 0) {
                perror("Failed to create thread");
                free(client_sock_ptr);
                close(client_socket);
            } else {
                pthread_detach(thread);
            }
        }

        // Check for input from stdin
        if (fds[1].revents & POLLIN) {
            char buffer[BUFFER_SIZE];
            int len = read(STDIN_FILENO, buffer, sizeof(buffer) - 1);
            if (len > 0) {
                buffer[len] = '\0';

                // Check if the input is "quit"
                if (strncmp(buffer, STR_QUIT, strlen(STR_QUIT)) == 0) {
                    printf("Quit command received. Shutting down server...\n");
                    break;
                }
            }
        }
    }

    // Close all client sockets and the server socket
    pthread_mutex_lock(&lock);
    for (int client_socket : client_sockets) {
        close(client_socket);
    }
    client_sockets.clear();
    pthread_mutex_unlock(&lock);

    close(server_socket);
    return 0;
}
