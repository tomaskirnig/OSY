//***************************************************************************
//
// Program example for labs in subject Operating Systems
//
// Petr Olivka, Dept. of Computer Science, petr.olivka@vsb.cz, 2017
//
// Modified Socket Server.
//
// This server can handle multiple clients simultaneously.
// For each connected client, it forks a new process.
// Each received line from the client is treated as a command to execute.
//
//***************************************************************************

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <stdarg.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h> // Added for wait
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#define STR_CLOSE   "close"
#define STR_QUIT    "quit"

//***************************************************************************
// log messages

#define LOG_ERROR               0       // errors
#define LOG_INFO                1       // information and notifications
#define LOG_DEBUG               2       // debug messages

// debug flag
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
    vsnprintf(l_buf, sizeof(l_buf), t_form, l_arg);
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

// Function to handle communication with a client
void handle_client(int l_sock_client)
{
    char l_buf[1024];

    while (1)
    {
        // Read data from client
        int l_len = read(l_sock_client, l_buf, sizeof(l_buf) - 1);
        if (l_len == 0)
        {
            log_msg(LOG_DEBUG, "Client closed socket!");
            close(l_sock_client);
            exit(0);
        }
        else if (l_len < 0)
        {
            log_msg(LOG_ERROR, "Unable to read data from client.");
            close(l_sock_client);
            exit(1);
        }
        else
        {
            l_buf[l_len] = '\0'; // Null-terminate the received string
            log_msg(LOG_DEBUG, "Received %d bytes from client: %s", l_len, l_buf);

            // Remove trailing newline character
            if (l_buf[l_len - 1] == '\n')
            {
                l_buf[l_len - 1] = '\0';
            }

            // Check for "close" command
            if (!strncasecmp(l_buf, STR_CLOSE, strlen(STR_CLOSE)))
            {
                log_msg(LOG_INFO, "Client sent 'close' request to close connection.");
                close(l_sock_client);
                exit(0);
            }

            // Split the command into arguments
            char *args[64];
            int argc = 0;
            char *token = strtok(l_buf, " ");
            while (token != NULL && argc < 63)
            {
                args[argc++] = token;
                token = strtok(NULL, " ");
            }
            args[argc] = NULL; // Null-terminate the arguments array

            if (argc == 0)
            {
                continue; // Empty command
            }

            // Fork a child process to execute the command
            pid_t pid = fork();
            if (pid < 0)
            {
                log_msg(LOG_ERROR, "Fork failed.");
                continue;
            }
            else if (pid == 0)
            {
                // Child process
                // Redirect stdout to the client socket
                dup2(l_sock_client, STDOUT_FILENO);
                dup2(l_sock_client, STDERR_FILENO);
                // Close unused file descriptors
                close(l_sock_client);

                // Execute the command
                execvp(args[0], args);

                // If execvp returns, it must have failed
                printf("Error: Command execution failed\n");
                exit(1);
            }
            else
            {
                // Parent process
                // Wait for the command to complete
                int status;
                waitpid(pid, &status, 0);
                if (WIFEXITED(status))
                {
                    log_msg(LOG_DEBUG, "Command executed with exit status %d", WEXITSTATUS(status));
                }
                else
                {
                    log_msg(LOG_DEBUG, "Command terminated abnormally");
                }
            }
        }
    }
}

//***************************************************************************

int main(int t_narg, char **t_args)
{
    if (t_narg <= 1)
    {
        printf("Usage: %s [-h -d] port_number\n", t_args[0]);
        exit(1);
    }

    int l_port = 0;

    // parsing arguments
    for (int i = 1; i < t_narg; i++)
    {
        if (!strcmp(t_args[i], "-d"))
            g_debug = LOG_DEBUG;

        if (!strcmp(t_args[i], "-h"))
        {
            printf(
                "\n"
                "  Socket server example.\n"
                "\n"
                "  Use: %s [-h -d] port_number\n"
                "\n"
                "    -d  debug mode \n"
                "    -h  this help\n"
                "\n",
                t_args[0]);

            exit(0);
        }

        if (*t_args[i] != '-' && !l_port)
        {
            l_port = atoi(t_args[i]);
            break;
        }
    }

    if (l_port <= 0)
    {
        log_msg(LOG_INFO, "Bad or missing port number %d!", l_port);
        exit(1);
    }

    log_msg(LOG_INFO, "Server will listen on port: %d.", l_port);

    // socket creation
    int l_sock_listen = socket(AF_INET, SOCK_STREAM, 0);
    if (l_sock_listen == -1)
    {
        log_msg(LOG_ERROR, "Unable to create socket.");
        exit(1);
    }

    in_addr l_addr_any = {INADDR_ANY};
    sockaddr_in l_srv_addr;
    l_srv_addr.sin_family = AF_INET;
    l_srv_addr.sin_port = htons(l_port);
    l_srv_addr.sin_addr = l_addr_any;

    // Enable the port number reusing
    int l_opt = 1;
    if (setsockopt(l_sock_listen, SOL_SOCKET, SO_REUSEADDR, &l_opt, sizeof(l_opt)) < 0)
        log_msg(LOG_ERROR, "Unable to set socket option!");

    // assign port number to socket
    if (bind(l_sock_listen, (const sockaddr *)&l_srv_addr, sizeof(l_srv_addr)) < 0)
    {
        log_msg(LOG_ERROR, "Bind failed!");
        close(l_sock_listen);
        exit(1);
    }

    // listening on set port
    if (listen(l_sock_listen, 5) < 0)
    {
        log_msg(LOG_ERROR, "Unable to listen on given port!");
        close(l_sock_listen);
        exit(1);
    }

    log_msg(LOG_INFO, "Enter 'quit' to quit server.");

    // list of fd sources
    pollfd l_read_poll[2];

    l_read_poll[0].fd = STDIN_FILENO;
    l_read_poll[0].events = POLLIN;
    l_read_poll[1].fd = l_sock_listen;
    l_read_poll[1].events = POLLIN;

    // go!
    while (1)
    {
        // select from fds
        int l_poll = poll(l_read_poll, 2, -1);

        if (l_poll < 0)
        {
            log_msg(LOG_ERROR, "Function poll failed!");
            exit(1);
        }

        if (l_read_poll[0].revents & POLLIN)
        { // data on stdin
            char buf[128];
            int len = read(STDIN_FILENO, buf, sizeof(buf));
            if (len < 0)
            {
                log_msg(LOG_DEBUG, "Unable to read from stdin!");
                exit(1);
            }

            buf[len] = '\0'; // Null-terminate the string
            log_msg(LOG_DEBUG, "Read %d bytes from stdin: %s", len, buf);
            // request to quit?
            if (!strncmp(buf, STR_QUIT, strlen(STR_QUIT)))
            {
                log_msg(LOG_INFO, "Request to 'quit' entered.");
                close(l_sock_listen);
                exit(0);
            }
        }

        if (l_read_poll[1].revents & POLLIN)
        { // new client?
            sockaddr_in l_rsa;
            socklen_t l_rsa_size = sizeof(l_rsa);
            // new connection
            int l_sock_client = accept(l_sock_listen, (sockaddr *)&l_rsa, &l_rsa_size);
            if (l_sock_client == -1)
            {
                log_msg(LOG_ERROR, "Unable to accept new client.");
                continue;
            }
            uint l_lsa = sizeof(l_srv_addr);
            // my IP
            getsockname(l_sock_client, (sockaddr *)&l_srv_addr, &l_lsa);
            log_msg(LOG_INFO, "My IP: '%s'  port: %d",
                    inet_ntoa(l_srv_addr.sin_addr), ntohs(l_srv_addr.sin_port));
            // client IP
            getpeername(l_sock_client, (sockaddr *)&l_srv_addr, &l_lsa);
            log_msg(LOG_INFO, "Client IP: '%s'  port: %d",
                    inet_ntoa(l_srv_addr.sin_addr), ntohs(l_srv_addr.sin_port));

            // Fork a new process to handle the client
            pid_t pid = fork();
            if (pid < 0)
            {
                log_msg(LOG_ERROR, "Fork failed.");
                close(l_sock_client);
                continue;
            }
            else if (pid == 0)
            {
                // Child process
                close(l_sock_listen); // Child doesn't need listening socket
                handle_client(l_sock_client);
                exit(0);
            }
            else
            {
                // Parent process
                close(l_sock_client); // Parent doesn't need client socket
                // Continue to accept new clients
            }
        }
    } // while (1)

    close(l_sock_listen);
    return 0;
}
