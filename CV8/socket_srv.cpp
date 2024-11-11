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
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>

#define MAX_IMAGES 5
#define BUFFER_SIZE 1024
#define CHUNK_SIZE 256

typedef struct {
    char *image_data;      
    sem_t semafor;         
    int size;      
    char image_name[BUFFER_SIZE];        
} Image;

Image g_images[MAX_IMAGES]; // Array of images
pthread_mutex_t g_images_mutex = PTHREAD_MUTEX_INITIALIZER;


//***************************************************************************
// log messages

#define LOG_ERROR               0       // errors
#define LOG_INFO                1       // information and notifications
#define LOG_DEBUG               2       // debug messages

// debug flag
int g_debug = LOG_INFO;

void log_msg( int t_log_level, const char *t_form, ... )
{
    const char *out_fmt[] = {
            "ERR: (%d-%s) %s\n",
            "INF: %s\n",
            "DEB: %s\n" };

    if ( t_log_level && t_log_level > g_debug ) return;

    char l_buf[ 1024 ];
    va_list l_arg;
    va_start( l_arg, t_form );
    vsprintf( l_buf, t_form, l_arg );
    va_end( l_arg );

    switch ( t_log_level )
    {
    case LOG_INFO:
    case LOG_DEBUG:
        fprintf( stdout, out_fmt[ t_log_level ], l_buf );
        break;

    case LOG_ERROR:
        fprintf( stderr, out_fmt[ t_log_level ], errno, strerror( errno ), l_buf );
        break;
    }
}

// Load image from file into memory (dummy function for illustration)
int load_image(const char *filename, Image *img) {
    printf("Loading image from file: %s\n", filename);

    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("Failed to open image file");
        return -1;
    }

    fseek(file, 0, SEEK_END);
    img->size = ftell(file);
    fseek(file, 0, SEEK_SET);

    img->image_data = (char*)malloc(img->size);
    if (!img->image_data) {
        fclose(file);
        perror("Failed to allocate memory for image");
        return -1;
    }

    fread(img->image_data, 1, img->size, file);
    fclose(file);

    // Store the filename in image_name
    // strncpy(img->image_name, filename, BUFFER_SIZE - 1);
    // img->image_name[BUFFER_SIZE - 1] = '\0';  // Ensure null-termination

    return 0;
}


// Find or load image by season name
Image* get_or_load_image(const char *season) {
    pthread_mutex_lock(&g_images_mutex);

    // Find the image
    for (int i = 0; i < MAX_IMAGES; i++) {
        if (g_images[i].image_data && strcmp(season, g_images[i].image_name) == 0) {
            pthread_mutex_unlock(&g_images_mutex);
            return &g_images[i];
        }
    }


    // Load the image if not found
    for (int i = 0; i < MAX_IMAGES; i++) {
        if (!g_images[i].image_data) {
            char filename[BUFFER_SIZE];
            snprintf(filename, sizeof(filename), "%s.jpg", season); 
            if (load_image(filename, &g_images[i]) == 0) {

                strncpy(g_images[i].image_name, season, BUFFER_SIZE - 1);
                g_images[i].image_name[BUFFER_SIZE - 1] = '\0';
                
                pthread_mutex_unlock(&g_images_mutex);
                return &g_images[i];
            }

            // If .jpg loading failed, try .png
            snprintf(filename, sizeof(filename), "%s.png", season); 
            if (load_image(filename, &g_images[i]) == 0) {

                strncpy(g_images[i].image_name, season, BUFFER_SIZE - 1);
                g_images[i].image_name[BUFFER_SIZE - 1] = '\0';

                pthread_mutex_unlock(&g_images_mutex);
                return &g_images[i];
            }

            // If both failed, return NULL
            pthread_mutex_unlock(&g_images_mutex);
            return NULL;

        }
    }

    pthread_mutex_unlock(&g_images_mutex);
    return NULL;
}

// Function to handle each client connection
void* client_handler(void *arg) {
    int client_socket = *(int*)arg;
    free(arg);

    while (1) {
        char buffer[BUFFER_SIZE];
        int len = read(client_socket, buffer, sizeof(buffer) - 1);
        if (len <= 0) {
            perror("Failed to read from client");
            close(client_socket);
            return NULL;
        }
        buffer[len] = '\0';

        // Parse image request
        if (strncmp(buffer, "#img ", 5) == 0) {
            char *season = buffer + 5;
            season[strcspn(season, "\r\n")] = 0;


            log_msg(LOG_INFO, "Client requested image for season: %s", season);

            // Load or get existing image
            Image *img = get_or_load_image(season);
            if (!img) {
                write(client_socket, "Error loading image\n", sizeof("Error loading image\n"));
                close(client_socket);
                return NULL;
            }

            // Wait for access to the image using semaphore
            sem_wait(&img->semafor);

            size_t sent = 0;
            // Send the image in chunks with a delay
            for (int i = 0; i < img->size; i += CHUNK_SIZE) {
                int chunk_size = (i + CHUNK_SIZE < img->size) ? CHUNK_SIZE : img->size - i;
                sent+= write(client_socket, img->image_data + i, chunk_size);
                printf("Sent %d %c\n", int((float)sent/img->size * 100.0f), '%');
                usleep(1000 * 1000 * 7/(img->size / CHUNK_SIZE));
            }

            sem_post(&img->semafor); // Release access to the image
        }else{
            write(client_socket, "Invalid command\n", sizeof("Invalid command\n"));
        }
    }

    close(client_socket);
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s port_number\n", argv[0]);
        exit(1);
    }
    int port = atoi(argv[1]);

    // Initialize image semaphores
    for (int i = 0; i < MAX_IMAGES; i++) {
        g_images[i].image_data = NULL;
        sem_init(&g_images[i].semafor, 0, 1); // Initialize semaphore to 1
    }

    // Create server socket
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("socket failed");
        exit(1);
    }

    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        close(server_socket);
        exit(1);
    }

    if (listen(server_socket, MAX_IMAGES) < 0) {
        perror("listen failed");
        close(server_socket);
        exit(1);
    }

    printf("Server is listening on port %d\n", port);

    // Accept connections from clients
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_addr_len);
        if (client_socket < 0) {
            perror("accept failed");
            continue;
        }

        int *client_sock_ptr = (int*)malloc(sizeof(int));
        *client_sock_ptr = client_socket;

        pthread_t thread;
        if (pthread_create(&thread, NULL, client_handler, client_sock_ptr) != 0) {
            perror("Failed to create thread for client");
            free(client_sock_ptr);
            close(client_socket);
        } else {
            pthread_detach(thread);
        }
    }

    close(server_socket);

    // Free image memory and destroy semaphores
    for (int i = 0; i < MAX_IMAGES; i++) {
        if (g_images[i].image_data) free(g_images[i].image_data);
        sem_destroy(&g_images[i].semafor);
    }

    return 0;
}
