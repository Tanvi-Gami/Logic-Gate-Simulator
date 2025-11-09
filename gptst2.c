// logic_simulator_stage2.c
#include <SDL3/SDL.h>
#include <stdbool.h>
#include <stdio.h>

typedef enum {
    GATE_AND,
    GATE_OR,
    GATE_NOT,
    GATE_NAND,
    GATE_NOR,
    GATE_XOR
} GateType;

typedef struct {
    SDL_FRect rect;
    SDL_Color color;
    GateType type;
    bool selected;
} LogicGate;

#define MAX_GATES 100

// Gate names for display
const char *gateNames[] = {"AND", "OR", "NOT", "NAND", "NOR", "XOR"};

int main(int argc, char *argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_Log("SDL initialization failed: %s", SDL_GetError());
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow("Digital Logic Circuit Simulator",100, 100, 1000, 700, 0);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, NULL, SDL_RENDERER_ACCELERATED);

    if (!window || !renderer) {
        SDL_Log("Window/Renderer creation failed: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    LogicGate gates[MAX_GATES];
    int gateCount = 0;

    bool running = true;
    bool dragging = false;
    int draggedIndex = -1;
    float offsetX = 0, offsetY = 0;

    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT)
                running = false;

            // Add gate on key press (for now, 1–6)
            if (event.type == SDL_EVENT_KEY_DOWN && gateCount < MAX_GATES) {
                GateType type;
                switch (event.key.keysym.sym) {
                    case SDLK_1: type = GATE_AND; break;
                    case SDLK_2: type = GATE_OR; break;
                    case SDLK_3: type = GATE_NOT; break;
                    case SDLK_4: type = GATE_NAND; break;
                    case SDLK_5: type = GATE_NOR; break;
                    case SDLK_6: type = GATE_XOR; break;
                    default: type = -1; break;
                }
                if (type != -1) {
                    int x, y;
                    SDL_GetMouseState(&x, &y);
                    gates[gateCount].rect = (SDL_FRect){x - 40, y - 25, 80, 50};
                    gates[gateCount].color = (SDL_Color){200, 200, 0, 255};
                    gates[gateCount].type = type;
                    gates[gateCount].selected = false;
                    gateCount++;
                }
            }

            // Select/drag logic
            if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                int x = event.button.x;
                int y = event.button.y;
                for (int i = 0; i < gateCount; i++) {
                    if (SDL_PointInRectFloat(&(SDL_FPoint){x, y}, &gates[i].rect)) {
                        gates[i].selected = true;
                        dragging = true;
                        draggedIndex = i;
                        offsetX = x - gates[i].rect.x;
                        offsetY = y - gates[i].rect.y;
                    } else {
                        gates[i].selected = false;
                    }
                }
            }

            if (event.type == SDL_EVENT_MOUSE_BUTTON_UP) {
                dragging = false;
                draggedIndex = -1;
            }

            if (event.type == SDL_EVENT_MOUSE_MOTION && dragging && draggedIndex != -1) {
                int x = event.motion.x;
                int y = event.motion.y;
                gates[draggedIndex].rect.x = x - offsetX;
                gates[draggedIndex].rect.y = y - offsetY;
            }
        }

        // Rendering
        SDL_SetRenderDrawColor(renderer, 25, 25, 25, 255);
        SDL_RenderClear(renderer);

        for (int i = 0; i < gateCount; i++) {
            SDL_SetRenderDrawColor(renderer,
                                   gates[i].color.r,
                                   gates[i].color.g,
                                   gates[i].color.b,
                                   gates[i].color.a);
            SDL_RenderFillRect(renderer, &gates[i].rect);

            // Highlight if selected
            if (gates[i].selected) {
                SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
                SDL_RenderRect(renderer, &gates[i].rect);
            }

            // Draw gate name (optional, no SDL_ttf yet)
        }

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
