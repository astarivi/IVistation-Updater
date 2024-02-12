#pragma once
#include <SDL.h>

struct SharedData {
    int progress;
    char error_message[500];
    SDL_mutex *mutex;
};

int worker_thread(void *data);