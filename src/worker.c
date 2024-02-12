#include "worker.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <windows.h>

#include "microtar.h"

void create_dirs(const char* path) {
    char buffer[240];
    char* p = buffer;

    strncpy(buffer, path, 240);

    // Iterate over the path and create intermediate directories
    while (*p != '\0') {
        if (*p == '\\' || *p == '/') {
            // Null-terminate the string temporarily
            *p = '\0';

            // Create the directory
            CreateDirectoryA(buffer, NULL);

            *p = '\\';
        }
        p++;
    }
}


void replace_slashes(char *str) {
    while (*str != '\0') {
        if (*str == '/')
            *str = '\\';
        str++;
    }
}

int worker_thread(void *data) {
    struct SharedData *sharedData = (struct SharedData *)data;
    const char* output_dir = "Q:\\";
    const char* update_file = "Q:\\update.tar";

    mtar_t tar;
    mtar_header_t header;
    int ret;


    // Open file
    ret = mtar_open(&tar, update_file, "r");
    if (ret != MTAR_ESUCCESS) {
        SDL_LockMutex(sharedData->mutex);

        sharedData->progress = -1;
        strcpy(sharedData->error_message, mtar_strerror(ret));

        SDL_UnlockMutex(sharedData->mutex);
        return 0;
    }


    // Count total size
    size_t total_size = 0;
    size_t extracted_size = 0;

    while ((ret = mtar_read_header(&tar, &header)) == MTAR_ESUCCESS) {
        total_size += header.size;

        mtar_next(&tar);
    }

    mtar_rewind(&tar);

    char buffer[4096];
    char long_path[241];
    bool use_path = false;

    // Start extracting
    while ((ret = mtar_read_header(&tar, &header)) == MTAR_ESUCCESS) {
        // Long link
        if (strcmp(header.name, "././@LongLink") == 0) {
            // Read the path
            mtar_read_data(&tar, long_path, header.size);
            long_path[header.size] = '\0';
            use_path = true;
            mtar_next(&tar);
            continue;
        }

        char output_path[240];

        if (use_path) {
            strcpy(output_path, output_dir);
            strcat(output_path, long_path);
            replace_slashes(output_path);

            use_path = false;
            memset(long_path, 0, sizeof(long_path));
        } else {
            strcpy(output_path, output_dir);
            strcat(output_path, header.name);
            replace_slashes(output_path);
        }

        // Create directories if needed
        create_dirs(output_path);

        // Directory
        const char *suffix = "\\";
        if (strcmp(output_path + strlen(output_path) - strlen(suffix), suffix) == 0) {
            mtar_next(&tar);
            continue;
        }

        // Remove existing file
        DeleteFileA(output_path);

        // Extract the file
        FILE *destination_file = fopen(output_path, "wb");
        if (!destination_file) {
            SDL_LockMutex(sharedData->mutex);
            sharedData->progress = -1;
            strcpy(sharedData->error_message, output_path);
            SDL_UnlockMutex(sharedData->mutex);

            mtar_close(&tar);
            return 0;
        }

        size_t remaining = header.size;
        while (remaining > 0) {
            size_t chunk_size = (remaining < 4096) ? remaining : 4096;

            ret = mtar_read_data(&tar, buffer, chunk_size);
            if (ret != MTAR_ESUCCESS) {
                mtar_close(&tar);

                SDL_LockMutex(sharedData->mutex);
                sharedData->progress = -1;
                strcpy(sharedData->error_message, mtar_strerror(ret));
                SDL_UnlockMutex(sharedData->mutex);

                return 0;
            }

            // Write the chunk to the destination file
            fwrite(buffer, 1, chunk_size, destination_file);

            remaining -= chunk_size;
        }

        fclose(destination_file);

        extracted_size += header.size;
        double progress = (double)extracted_size / total_size * 100.0;

        SDL_LockMutex(sharedData->mutex);
        sharedData->progress = (int)progress;
        SDL_UnlockMutex(sharedData->mutex);

        mtar_next(&tar);
    }

    mtar_close(&tar);

    if (ret != MTAR_ENULLRECORD) {
        SDL_LockMutex(sharedData->mutex);
        sharedData->progress = -1;
        strcpy(sharedData->error_message, mtar_strerror(ret));
        SDL_UnlockMutex(sharedData->mutex);
        return 0;
    }

    DeleteFileA(update_file);

    SDL_LockMutex(sharedData->mutex);
    sharedData->progress = 102;
    SDL_UnlockMutex(sharedData->mutex);
}