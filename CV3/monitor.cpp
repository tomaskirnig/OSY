#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#define MAX_FILES 50

void print_time() {
    time_t now = time(NULL);
    struct tm *local_time = localtime(&now);
    char time_buffer[80];
    strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", local_time);
    printf("%s ", time_buffer);
}

void print_header(const char* filename, off_t size) {
    printf("-----------------------------------------------------------------\n");
    print_time();
    printf("Velikost: %ld  Soubor: %s\n", (long)size, filename);
    printf("-----------------------------------------------------------------\n");
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <file/s>\n", argv[0]);
        return 1;
    }

    const char* filename;
    struct stat file_stat;
    off_t prev_size[MAX_FILES];
    int monitor_status[MAX_FILES]; // 1: monitor 0: do not monitor

    for(int i = 0; i < MAX_FILES; i++) {
        prev_size[i] = 0;
        monitor_status[i] = 1;
    }

    char* files[MAX_FILES];
    int file_count = 0;

    char* not_found_files[MAX_FILES];
    int not_found_count = 0;

    FILE *file;

    for(int i = 1; i < argc; i++) {
        if (stat(argv[i], &file_stat) == 0) {
             if (!(file_stat.st_mode & S_IXUSR)) files[file_count++] = argv[i];
        } else {
            not_found_files[not_found_count++] = argv[i];
        }
    }

    if (file_count == 0) {
        fprintf(stderr, "No files to monitor.\n");
        return -2;
    }

    while (1) {
        for(int i = 0; i < file_count; i++){
            filename = files[i];
            // Get file size
            if (stat(filename, &file_stat) == 0) {
                if (access(filename, W_OK) != 0) {
                    if (monitor_status[i] == 1) {
                        printf("File stoped %s being monitored.\n", filename);
                        monitor_status[i] = 0;  // Stop monitoring
                    }
                    continue; // Skip this file
                } else {
                    if (monitor_status[i] == 0) {
                        printf("File %s is being monitored again.\n", filename);
                        monitor_status[i] = 1;  // Resume monitoring
                    }
                }

                off_t current_size = file_stat.st_size;

                if (current_size < prev_size[i]) {
                    // File was truncated
                    print_time();
                    printf("File [%s] was truncated. Old size: %ld, New size: %ld\n", filename, (long)prev_size[i], (long)current_size);
                } else if (current_size > prev_size[i]) {
                    // File size increased, print new content
                    file = fopen(filename, "r");
                    if (file) {
                        fseek(file, prev_size[i], SEEK_SET);  // Go to our last position
                        char buffer[256];
                        print_header(filename, current_size);
                        while (fgets(buffer, sizeof(buffer), file) != NULL) {
                            printf("%s", buffer);
                        }
                        fclose(file);
                    }
                }
                
                // Update previous size
                prev_size[i] = current_size;
            } else {
                fprintf(stderr, "Error: Unable to stat file %s\n", filename);
                return 1;
            }
        }        
        sleep(1);  // Check every 1 second
    }

    return 0;
}
