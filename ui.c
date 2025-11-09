#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
int main(argc, char *argv[]){

        if (!SDL_Init(SDL_INIT_VIDEO)) {
        printf("Error: %s\n", SDL_GetError());
        return 1;
    }
    SDL_Window *window = SDL_CreateWindow("LOgic Gate Simulator", 800, 600, SDL_WINDOW_RESIZABLE);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, NULL, 0);
}