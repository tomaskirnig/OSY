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

struct ClientInfo {
    int socket;
    char* username;
};

#define NUM_OF_CLIENTS 20
#define BUFFER_SIZE 256

#define STR_CLOSE "#close"
#define STR_QUIT "#close"
#define STR_LIST "#list"

// Shared list of client sockets and mutex for synchronization
std::vector<ClientInfo> client_sockets;
pthread_mutex_t lock;

//***************************************************************************
// log messages
#define LOG_ERROR               0       // errors
#define LOG_INFO                1       // information and notifications
#define LOG_DEBUG               2       // debug messages

int g_debug = LOG_INFO;

void log_msg(int t_log_level, const char *t_form, ...) {
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
    switch (t_log_level) {
    case LOG_INFO:
    case LOG_DEBUG:
        fprintf(stdout, out_fmt[t_log_level], l_buf);
        break;
    case LOG_ERROR:
        fprintf(stderr, out_fmt[t_log_level], errno, strerror(errno), l_buf);
        break;
    }
}

// Broadcast message to all connected clients
void broadcast_to_clients(const char *message) {
    pthread_mutex_lock(&lock);
    for (const ClientInfo& client : client_sockets) {
        write(client.socket, message, strlen(message));
    }
    pthread_mutex_unlock(&lock);
}

// Handle client communication
void* handle_client(void *client_socket_ptr) {
    int client_socket = *(int*)client_socket_ptr;
    free(client_socket_ptr);
    char buffer[BUFFER_SIZE];

    // Find the client info in the vector
    ClientInfo* client_info = nullptr;
    pthread_mutex_lock(&lock);
    for (ClientInfo& client : client_sockets) {
        if (client.socket == client_socket) {
            client_info = &client;
            break;
        }
    }
    pthread_mutex_unlock(&lock);

    if (!client_info) {
        log_msg(LOG_ERROR, "Client info not found for socket %d", client_socket);
        close(client_socket);
        return nullptr;
    }

    while (1) {
        int len = read(client_socket, buffer, BUFFER_SIZE - 1);
        if (len <= 0) {
            log_msg(LOG_INFO, "Client '%s' disconnected.", client_info->username);
            break;
        }
        buffer[len] = '\0';

        // Remove trailing newline or return 
        if (len > 0 && (buffer[len - 1] == '\n')) {
            buffer[len - 1] = '\0';
        }

        log_msg(LOG_INFO, "Received message from '%s': %s", client_info->username, buffer);

        if (strncmp(STR_LIST, buffer, strlen(STR_LIST)) == 0) {
            pthread_mutex_lock(&lock);
            for (const ClientInfo& client : client_sockets) {
                char msg[BUFFER_SIZE];
                snprintf(msg, BUFFER_SIZE, "Client: %s\n", client.username);
                write(client_socket, msg, strlen(msg));
            }
            pthread_mutex_unlock(&lock);
        }else if (strncmp("#", buffer, 1) == 0) {
            pthread_mutex_lock(&lock); 
            for (ClientInfo& client : client_sockets) {
                if (strncmp(buffer + 1, client.username, strlen(client.username)) == 0) {
                    char msg[BUFFER_SIZE];
                    snprintf(msg, BUFFER_SIZE, "%s: %s\n", client_info->username, buffer + strlen(client.username) + 1);
                    write(client.socket, msg, strlen(msg));
                    break;
                }
            }
            pthread_mutex_unlock(&lock);
        }else {
            char msg[BUFFER_SIZE];
            snprintf(msg, BUFFER_SIZE, "%s: %s\n", client_info->username, buffer);
            broadcast_to_clients(msg);
        }
    }

    pthread_mutex_lock(&lock);
    auto it = std::find_if(client_sockets.begin(), client_sockets.end(),
                        [client_socket](const ClientInfo& client) { return client.socket == client_socket; });
    if (it != client_sockets.end()) {
        free(it->username);
        client_sockets.erase(it);
    }
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

    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("Failed to set SO_REUSEADDR");
        close(server_socket);
        exit(1);
    }

    log_msg(LOG_INFO, "Server will listen on port: %d", port);
    pthread_mutex_init(&lock, nullptr);

    struct pollfd fds[2];
    fds[0].fd = server_socket;
    fds[0].events = POLLIN;
    fds[1].fd = STDIN_FILENO;
    fds[1].events = POLLIN;

    while (1) {
        int poll_res = poll(fds, 2, -1);
        if (poll_res < 0) {
            perror("Poll error");
            break;
        }

        if (fds[0].revents & POLLIN) {
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            int client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
            if (client_socket < 0) {
                perror("Failed to accept client");
                continue;
            }

            write(client_socket, "Enter a username: ", sizeof("Enter a username: "));
            char buffer[BUFFER_SIZE];
            int len = read(client_socket, buffer, BUFFER_SIZE - 1);
            if (len <= 0) {
                log_msg(LOG_INFO, "Client disconnected.");
                close(client_socket);
                continue;
            }
            buffer[len - 1] = '\0';

            ClientInfo client = {client_socket, strdup(buffer)};
            pthread_mutex_lock(&lock);
            client_sockets.push_back(client);
            pthread_mutex_unlock(&lock);

            log_msg(LOG_INFO, "Client '%s' connected.\nOn port %d", buffer, ntohs(client_addr.sin_port));

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

        if (fds[1].revents & POLLIN) {
            char buffer[BUFFER_SIZE];
            int len = read(STDIN_FILENO, buffer, sizeof(buffer) - 1);
            if (len > 0) {
                buffer[len] = '\0';
                if (strncmp(buffer, STR_QUIT, strlen(STR_QUIT)) == 0) {
                    printf("Quit command received. Shutting down server...\n");
                    break;
                }else if (strncmp(buffer, STR_LIST, strlen(STR_LIST)) == 0) {
                    pthread_mutex_lock(&lock);
                    for (const ClientInfo& client : client_sockets) {
                        printf("Client: %s\n", client.username);
                    }
                    pthread_mutex_unlock(&lock);
                }
            }
        }
    }

    pthread_mutex_lock(&lock);
    for (ClientInfo& client : client_sockets) {
        free(client.username);
        close(client.socket);
    }
    client_sockets.clear();
    pthread_mutex_unlock(&lock);

    close(server_socket);
    pthread_mutex_destroy(&lock);
    return 0;
}