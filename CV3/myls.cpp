#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>

#define MAX_FILES 100

void print_file_info(const char* path, bool show_size, bool show_time, bool show_rights) {
    struct stat file_stat;

    if (stat(path, &file_stat) != 0) {
        fprintf(stderr, "Error: File not found: %s\n", path);
        return;
    }

    // file size
    if (show_size) {
        printf("%12ld ", file_stat.st_size);
    }

    // last modification time
    if (show_time) {
        char timebuf[80];
        struct tm *timeinfo = localtime(&file_stat.st_mtime);
        strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", timeinfo);
        printf("%20s ", timebuf);
    }

    // file/dir permissions
    if (show_rights) {
        printf("%c%c%c%c%c%c%c%c%c%c ",
            S_ISDIR(file_stat.st_mode) ? 'd' : '-',   
            (file_stat.st_mode & S_IRUSR) ? 'r' : '-',
            (file_stat.st_mode & S_IWUSR) ? 'w' : '-',
            (file_stat.st_mode & S_IXUSR) ? 'x' : '-',
            (file_stat.st_mode & S_IRGRP) ? 'r' : '-',
            (file_stat.st_mode & S_IWGRP) ? 'w' : '-',
            (file_stat.st_mode & S_IXGRP) ? 'x' : '-',
            (file_stat.st_mode & S_IROTH) ? 'r' : '-',
            (file_stat.st_mode & S_IWOTH) ? 'w' : '-',
            (file_stat.st_mode & S_IXOTH) ? 'x' : '-');
    }

    // file name
    printf("%s\n", path);
}

int main(int argc, char* argv[]) {
    bool show_size = false;
    bool show_time = false;
    bool show_rights = false;

    // file names
    char* files[MAX_FILES];
    int file_count = 0;

    // not found files
    char* not_found_files[MAX_FILES];
    int not_found_count = 0;

    // main arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-s") == 0) {
            show_size = true;
        } else if (strcmp(argv[i], "-t") == 0) {
            show_time = true;
        } else if (strcmp(argv[i], "-r") == 0) {
            show_rights = true;
        } else {
            struct stat buffer;
            if (stat(argv[i], &buffer) != 0) {
                not_found_files[not_found_count] = argv[i];
                not_found_count++;
            } else {
                files[file_count] = argv[i];
                file_count++;
            }
        }
    }

    // Print not found files
    if (not_found_count > 0) {
        fprintf(stderr, "Files not found:\n");
        for (int i = 0; i < not_found_count; ++i) {
            fprintf(stderr, "%s\n", not_found_files[i]);
        }
    }

    printf("-------------------------------------------------\n");

    // Print information about found files
    for (int i = 0; i < file_count; ++i) {
        print_file_info(files[i], show_size, show_time, show_rights);
    }

    return 0;
}
