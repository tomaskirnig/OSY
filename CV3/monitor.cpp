#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

void print_time() {
    time_t now = time(NULL);
    struct tm *local_time = localtime(&now);
    char time_buffer[80];
    strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", local_time);
    printf("%s ", time_buffer);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <file>\n", argv[0]);
        return 1;
    }

    const char* filename = argv[1];
    struct stat file_stat;
    off_t prev_size = 0;

    FILE *file;

    while (1) {
        // Get file size
        if (stat(filename, &file_stat) == 0) {
            off_t current_size = file_stat.st_size;

            if (current_size < prev_size) {
                // File was truncated
                print_time();
                printf("File was truncated. Old size: %ld, New size: %ld\n", (long)prev_size, (long)current_size);
            } else if (current_size > prev_size) {
                // File size increased, print new content
                file = fopen(filename, "r");
                if (file) {
                    fseek(file, prev_size, SEEK_SET);  // Go to our last position
                    char buffer[256];
                    while (fgets(buffer, sizeof(buffer), file) != NULL) {
                        printf("%s", buffer);
                    }
                    fclose(file);
                }
            }

            // Update previous size
            prev_size = current_size;
        } else {
            fprintf(stderr, "Error: Unable to stat file %s\n", filename);
            return 1;
        }

        sleep(1);  // Check every 1 second
    }

    return 0;
}
