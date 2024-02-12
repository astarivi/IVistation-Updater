#include <hal/debug.h>
#include <hal/xbox.h>
#include <hal/video.h>

#include <nxdk/mount.h>
#include <nxdk/path.h>

#include <stdbool.h>

#include <string.h>
#include <windows.h>

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>

#include "src/worker.h"
#include "src/image.h"
#include "src/font.h"
#include "src/lithiumx.h"

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480
#define MAX_TEXT_WIDTH 400

char targetPath[240];
struct SharedData sharedData;
SDL_Thread *thread;
SDL_Window *window;
SDL_Renderer *renderer;
SDL_Texture *image_texture = NULL;
TTF_Font *font = NULL;

void initialize() {
    XVideoSetMode(SCREEN_WIDTH, SCREEN_HEIGHT, 32, REFRESH_DEFAULT);

    // Initialize SDL
    if (SDL_Init(SDL_INIT_TIMER | SDL_INIT_EVENTS | SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) < 0) {
        debugPrint("SDL initialization failed! Exiting in 10s\n");
        Sleep(10000);
        XReboot();
    }

    // Initialize TTF
    if (TTF_Init() != 0) {
        debugPrint("TTF_Init failed: %s  Exiting in 10s\n", TTF_GetError());
        Sleep(10000);
        XReboot();
    }

    // Create window and renderer
    window = SDL_CreateWindow(
            "Updater",
            SDL_WINDOWPOS_UNDEFINED,
            SDL_WINDOWPOS_UNDEFINED,
            SCREEN_WIDTH,
            SCREEN_HEIGHT,
            SDL_WINDOW_SHOWN
    );

    renderer = SDL_CreateRenderer(window, -1, 0);

    // Initialize IMG module
    if (!(IMG_Init(IMG_INIT_JPG) & IMG_INIT_JPG)) {
        debugPrint("Couldn't intialize SDL_image.\n");
        Sleep(10000);
        SDL_VideoQuit();
        XReboot();
    }

    // Load image (ye we are not closing this)
    SDL_RWops* rw = SDL_RWFromConstMem(background_data, sizeof(background_data));
    SDL_Surface* image_surface = IMG_Load_RW(rw, 1);

    image_texture = SDL_CreateTextureFromSurface(
            renderer,
            image_surface
    );

    SDL_FreeSurface(image_surface);

    if (!image_texture) {
        debugPrint("Couldn't intialize image texture.\n");
        Sleep(10000);
        SDL_VideoQuit();
        XReboot();
    }

    // Load font
    SDL_RWops* frw = SDL_RWFromConstMem(font_data, sizeof(font_data));
    font = TTF_OpenFontRW(frw, 1, 18);

    if (font == NULL) {
        debugPrint("Couldn't load font: %s", TTF_GetError());
        Sleep(10000);
        XReboot();
    }

    // Initialize shared data with worker
    sharedData.mutex = SDL_CreateMutex();
    sharedData.progress = 0;

    // Mount root to Q:
    nxGetCurrentXbeNtPath(targetPath);
    *(strrchr(targetPath, '\\') + 1) = '\0';
    nxMountDrive('Q', targetPath);
    debugPrint("Mounted %s to Q:\n", targetPath);

    // Start worker thread
    thread = SDL_CreateThread(worker_thread, "tar_worker_thread", &sharedData);
}

void render_message(char* message) {
    // Clear renderer
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    // Render background image
    SDL_RenderCopy(renderer, image_texture, NULL, NULL);

    // Render message
    SDL_Color textColor = {255, 255, 255, 255};
    SDL_Surface* textSurface = TTF_RenderText_Blended_Wrapped(font, message, textColor, 520);
    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_FreeSurface(textSurface);
    SDL_Rect destinationRect = {60, 105, 0, 0};  // Replace x and y with your desired position
    SDL_QueryTexture(textTexture, NULL, NULL, &destinationRect.w, &destinationRect.h);
    SDL_RenderCopy(renderer, textTexture, NULL, &destinationRect);

    SDL_DestroyTexture(textTexture);

    SDL_RenderPresent(renderer);
}

void write_backup() {
    render_message(
            "Replacing IVistation with LithiumX.\n"
            "This may take a while. We will take you to LithiumX as soon as possible.\n\n"
            "Do not turn off your console."
    );

    DeleteFileA("Q:\\default.xbe");

    FILE *file = fopen("Q:\\default.xbe", "wb");

    if (file == NULL) {
        render_message(
                "Failed to replace IVisitation with LithiumX.\n"
                "Can not write to hard drive.\n\n"
                "...Rebooting in 5s..."
        );
        Sleep(5000);
        SDL_VideoQuit();
        SDL_Quit();
        XReboot();
    }

    fwrite(lithiumx, sizeof(unsigned char), sizeof(lithiumx) / sizeof(lithiumx[0]), file);

    SDL_VideoQuit();
    SDL_Quit();
    strcat(targetPath, "default.xbe");
    XLaunchXBE(targetPath);
}

void error_message(char* message) {
    render_message(message);
    SDL_GameController *pad = NULL;
    static SDL_Event e;

    // Wait for user input
    while (1) {
        SDL_PollEvent(&e);
        if (e.type == SDL_CONTROLLERDEVICEADDED) {
            SDL_GameController *new_pad = SDL_GameControllerOpen(e.cdevice.which);
            if (pad == NULL) {
                pad = new_pad;
            }
        }
        else if (e.type == SDL_CONTROLLERDEVICEREMOVED) {
            if (pad == SDL_GameControllerFromInstanceID(e.cdevice.which)) {
                pad = NULL;
            }
            SDL_GameControllerClose(SDL_GameControllerFromInstanceID(e.cdevice.which));
        }
        else if (e.type == SDL_CONTROLLERBUTTONDOWN) {
            if (e.cbutton.button == SDL_CONTROLLER_BUTTON_B) {
                SDL_VideoQuit();
                XReboot();
                break;
            } else if (e.cbutton.button == SDL_CONTROLLER_BUTTON_X) {
                write_backup();
                XReboot();
                break;
            }
        }
    }
}

int main(void) {
    initialize();

    while (1) {
        // Get worker info
        SDL_LockMutex(sharedData.mutex);

        int progress = sharedData.progress;
        char e_msg[500];
        strcpy(e_msg, sharedData.error_message);

        SDL_UnlockMutex(sharedData.mutex);

        // We're done
        if(progress == 102) {
            break;
        // Fatal error
        } else if(progress == -1) {
            char message_str[500];

            snprintf(message_str, sizeof(message_str),
                     "IVistation Updater v1.0 - FATAL ERROR\n"
                     "Error: %s\n"
                     "Perhaps taking a photo of this screen would be a wise thing to do.\n"
                     "You know what is even better?, sending the photo to astarivi in Discord.\n"
                     "If IVistation was your main dashboard, you may not have a dashboard anymore.\n\n"
                     "Press B to reboot at your own risk, or press X to replace IVistation with LithiumX.\n",
                     e_msg
             );

            // This won't return
            error_message(message_str);
        // We're initializing
        } else if(progress == 0) {
            render_message(
                "Welcome to IVistation Updater v1.0\n"
                "...Currently initializing files...\n"
                "This can take a while\n"
                "Do not turn off your console\n"
            );
        // Actual progress being made
        } else {
            char message_str[500];

            snprintf(message_str, sizeof(message_str),
                     "Installing update\n"
                     "Update progress: %d% \n"
                     "At times the progress may seem stuck, please be patient\n"
                     "Do not turn off your console\n",
                     progress
            );

            render_message(message_str);
        }

        Sleep(500);
    }

    render_message(
            "Update successfully installed\n"
            "...Taking you to IVistation in 5 seconds..."
    );

    Sleep(5000);

    SDL_VideoQuit();
    SDL_Quit();

    strcat(targetPath, "default.xbe");
    XLaunchXBE(targetPath);
}