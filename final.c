#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include "logicgates.h"
#include <stdlib.h>
#include "truth_table.h"

// Fullscreen dimensions
#define WINDOW_WIDTH 1400
#define WINDOW_HEIGHT 800
#define GATE_WIDTH 120
#define GATE_HEIGHT 80
#define MARGIN 50
#define PIN_LENGTH 20
#define PIN_RADIUS 8
#define PALETTE_WIDTH 200
#define MAX_GATES 50
#define MAX_WIRES 100
#define BUTTON_WIDTH 180
#define BUTTON_HEIGHT 40
#define BUTTON_X (WINDOW_WIDTH - BUTTON_WIDTH - 20)  // 20px from right edge
#define BUTTON_Y 20

typedef struct {
    const char* name;
    SDL_FRect rect;
    SDL_Color color;
    SDL_Color selected_color;
    int inputs;
    int outputs;
    bool is_selected;
    bool is_dragging;
    int drag_offset_x;
    int drag_offset_y;
    bool in_palette;
    int id;
    int* input_values;
    int output_value;
    int gate_type;
} LogicGate;

typedef struct {
    int from_gate_id;
    int from_pin_index;
    int to_gate_id;
    int to_pin_index;
    SDL_Color color;
} Wire;

void draw_text(SDL_Renderer* renderer, const char* text, float x, float y, SDL_Color color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    
    float current_x = x;
    
    for (int i = 0; text[i] != '\0'; i++) {
        char c = text[i];
        
        switch (c) {
            case 'A':
                SDL_RenderLine(renderer, current_x, y+10, current_x+4, y);  //diagonal line 
                SDL_RenderLine(renderer, current_x+4, y, current_x+8, y+10); // diagonal line
                SDL_RenderLine(renderer, current_x+1, y+5, current_x+7, y+5); // horizontal line 
                break;
            case 'B':
                SDL_RenderLine(renderer, current_x, y, current_x, y+10); 
                SDL_RenderLine(renderer, current_x, y, current_x+6, y);
                SDL_RenderLine(renderer, current_x, y+5, current_x+6, y+5);
                SDL_RenderLine(renderer, current_x, y+10, current_x+6, y+10);
                SDL_RenderLine(renderer, current_x+6, y+1, current_x+8, y+2);
                SDL_RenderLine(renderer, current_x+8, y+2, current_x+8, y+4);
                SDL_RenderLine(renderer, current_x+8, y+4, current_x+6, y+5);
                SDL_RenderLine(renderer, current_x+6, y+6, current_x+8, y+8);
                SDL_RenderLine(renderer, current_x+8, y+8, current_x+8, y+9);
                SDL_RenderLine(renderer, current_x+8, y+9, current_x+6, y+10);
                break;
            case 'D':
                SDL_RenderLine(renderer, current_x, y, current_x, y+10); 
                SDL_RenderLine(renderer, current_x, y, current_x+6, y);
                SDL_RenderLine(renderer, current_x, y+10, current_x+6, y+10);
                SDL_RenderLine(renderer, current_x+6, y+1, current_x+8, y+3);
                SDL_RenderLine(renderer, current_x+8, y+3, current_x+8, y+7);
                SDL_RenderLine(renderer, current_x+8, y+7, current_x+6, y+9);
                break;
            case 'E':
                SDL_RenderLine(renderer, current_x, y, current_x, y+10);
                SDL_RenderLine(renderer, current_x, y, current_x+8, y);
                SDL_RenderLine(renderer, current_x, y+5, current_x+6, y+5);
                SDL_RenderLine(renderer, current_x, y+10, current_x+8, y+10);
                break;
            case 'F':
                SDL_RenderLine(renderer, current_x, y, current_x, y+10);
                SDL_RenderLine(renderer, current_x, y, current_x+8, y);
                SDL_RenderLine(renderer, current_x, y+5, current_x+6, y+5);
                break;
            case 'G':
                SDL_RenderLine(renderer, current_x+8, y+2, current_x+8, y+8);
                SDL_RenderLine(renderer, current_x+8, y+8, current_x+6, y+10);
                SDL_RenderLine(renderer, current_x+6, y+10, current_x+2, y+10);
                SDL_RenderLine(renderer, current_x+2, y+10, current_x, y+8);
                SDL_RenderLine(renderer, current_x, y+8, current_x, y+2);
                SDL_RenderLine(renderer, current_x, y+2, current_x+2, y);
                SDL_RenderLine(renderer, current_x+2, y, current_x+6, y);
                SDL_RenderLine(renderer, current_x+6, y, current_x+8, y+2);
                SDL_RenderLine(renderer, current_x+4, y+5, current_x+8, y+5);
                break;
            case 'I':
                SDL_RenderLine(renderer, current_x, y, current_x+8, y);
                SDL_RenderLine(renderer, current_x+4, y, current_x+4, y+10);
                SDL_RenderLine(renderer, current_x, y+10, current_x+8, y+10);
                break;
            case 'N':
                SDL_RenderLine(renderer, current_x, y, current_x, y+10);
                SDL_RenderLine(renderer, current_x, y, current_x+8, y+10);
                SDL_RenderLine(renderer, current_x+8, y, current_x+8, y+10);
                break;
            case 'O':
                SDL_RenderLine(renderer, current_x+4, y, current_x+8, y+4);
                SDL_RenderLine(renderer, current_x+8, y+4, current_x+8, y+6);
                SDL_RenderLine(renderer, current_x+8, y+6, current_x+4, y+10);
                SDL_RenderLine(renderer, current_x+4, y+10, current_x, y+6);
                SDL_RenderLine(renderer, current_x, y+6, current_x, y+4);
                SDL_RenderLine(renderer, current_x, y+4, current_x+4, y);
                break;
            case 'P':
                SDL_RenderLine(renderer, current_x, y, current_x, y+10);
                SDL_RenderLine(renderer, current_x, y, current_x+6, y);
                SDL_RenderLine(renderer, current_x+6, y, current_x+8, y+2);
                SDL_RenderLine(renderer, current_x+8, y+2, current_x+8, y+4);
                SDL_RenderLine(renderer, current_x+8, y+4, current_x+6, y+6);
                SDL_RenderLine(renderer, current_x+6, y+6, current_x, y+6);
                break;
            case 'R':
                SDL_RenderLine(renderer, current_x, y, current_x, y+10);
                SDL_RenderLine(renderer, current_x, y, current_x+6, y);
                SDL_RenderLine(renderer, current_x+6, y, current_x+8, y+2);
                SDL_RenderLine(renderer, current_x+8, y+2, current_x+8, y+4);
                SDL_RenderLine(renderer, current_x+8, y+4, current_x+6, y+6);
                SDL_RenderLine(renderer, current_x+6, y+6, current_x, y+6);
                SDL_RenderLine(renderer, current_x+2, y+6, current_x+8, y+10);
                break;
            case 'T':
                SDL_RenderLine(renderer, current_x, y, current_x+8, y);
                SDL_RenderLine(renderer, current_x+4, y, current_x+4, y+10);
                break;
            case 'U':
                SDL_RenderLine(renderer, current_x, y, current_x, y+8);
                SDL_RenderLine(renderer, current_x, y+8, current_x+4, y+10);
                SDL_RenderLine(renderer, current_x+4, y+10, current_x+8, y+8);
                SDL_RenderLine(renderer, current_x+8, y+8, current_x+8, y);
                break;
            case 'X':
                SDL_RenderLine(renderer, current_x, y, current_x+8, y+10);
                SDL_RenderLine(renderer, current_x+8, y, current_x, y+10);
                break;
            case 'L':
                SDL_RenderLine(renderer, current_x, y, current_x, y+10);
                SDL_RenderLine(renderer, current_x, y+10, current_x+8, y+10);
                break;
            case 'H':
                SDL_RenderLine(renderer, current_x, y, current_x, y+10);
                SDL_RenderLine(renderer, current_x+8, y, current_x+8, y+10);
                SDL_RenderLine(renderer, current_x, y+5, current_x+8, y+5);
                break;
        }
        
        current_x += 12;
    }
}

void get_pin_position(LogicGate gate, bool is_output, int pin_index, float* x, float* y) {
    if (is_output) {
        *x = gate.rect.x + gate.rect.w;
        *y = gate.rect.y + (gate.rect.h / (gate.outputs + 1)) * (pin_index + 1);
    } else {
        *x = gate.rect.x;
        *y = gate.rect.y + (gate.rect.h / (gate.inputs + 1)) * (pin_index + 1);
    }
}

void draw_pins(SDL_Renderer* renderer, LogicGate gate) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    
    for (int i = 0; i < gate.inputs; i++) {
        float pin_x, pin_y;
        get_pin_position(gate, false, i, &pin_x, &pin_y);
        
        SDL_RenderLine(renderer, pin_x - PIN_LENGTH, pin_y, pin_x, pin_y);
        
        for (int r = 0; r <= PIN_RADIUS * 2; r++) {
            for (int c = 0; c <= PIN_RADIUS * 2; c++) {
                float dx = c - PIN_RADIUS;
                float dy = r - PIN_RADIUS;
                if (dx * dx + dy * dy <= PIN_RADIUS * PIN_RADIUS) {
                    SDL_RenderPoint(renderer, pin_x - PIN_LENGTH + dx, pin_y + dy);
                }
            }
        }
    }
    
    for (int i = 0; i < gate.outputs; i++) {
        float pin_x, pin_y;
        get_pin_position(gate, true, i, &pin_x, &pin_y);
        
        SDL_RenderLine(renderer, pin_x, pin_y, pin_x + PIN_LENGTH, pin_y);
        
        for (int r = 0; r <= PIN_RADIUS * 2; r++) {
            for (int c = 0; c <= PIN_RADIUS * 2; c++) {
                float dx = c - PIN_RADIUS;
                float dy = r - PIN_RADIUS;
                if (dx * dx + dy * dy <= PIN_RADIUS * PIN_RADIUS) {
                    SDL_RenderPoint(renderer, pin_x + PIN_LENGTH + dx, pin_y + dy);
                }
            }
        }
    }
}

void draw_logic_gate(SDL_Renderer* renderer, LogicGate gate) {
    SDL_Color draw_color = gate.is_selected ? gate.selected_color : gate.color;
    
    if (!gate.in_palette && (gate.gate_type == 6 || gate.gate_type == 7)) {
        if (gate.output_value == 1) {
            draw_color = (SDL_Color){0, 200, 0, 255};
        } else {
            draw_color = (SDL_Color){200, 0, 0, 255};
        }
    }
    
    SDL_SetRenderDrawColor(renderer, draw_color.r, draw_color.g, draw_color.b, 255);
    SDL_RenderFillRect(renderer, &gate.rect);
    
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    if (gate.is_selected) {
        SDL_FRect thick_border = {
            gate.rect.x - 2, gate.rect.y - 2,
            gate.rect.w + 4, gate.rect.h + 4
        };
        SDL_RenderRect(renderer, &thick_border);
    } else {
        SDL_RenderRect(renderer, &gate.rect);
    }
    
    if (!gate.in_palette) {
        draw_pins(renderer, gate);
    }
    
    float text_x = gate.rect.x + (gate.rect.w - (strlen(gate.name) * 12)) / 2;
    float text_y = gate.rect.y + gate.rect.h / 2 - 5;
    
    SDL_Color text_color = {255, 255, 255, 255};
    draw_text(renderer, gate.name, text_x, text_y, text_color);
    
    if (!gate.in_palette && (gate.gate_type == 6 || gate.gate_type == 7)) {
        char value_str[2];
        sprintf(value_str, "%d", gate.output_value);
        SDL_Color value_color = {255, 255, 255, 255};
        draw_text(renderer, value_str, gate.rect.x + 10, gate.rect.y + 10, value_color);
    }
}

void draw_wires(SDL_Renderer* renderer, Wire* wires, int wire_count, LogicGate* gates, int gate_count) {
    for (int i = 0; i < wire_count; i++) {
        Wire wire = wires[i];
        
        LogicGate* from_gate = NULL;
        LogicGate* to_gate = NULL;
        
        for (int j = 0; j < gate_count; j++) {
            if (gates[j].id == wire.from_gate_id) from_gate = &gates[j];
            if (gates[j].id == wire.to_gate_id) to_gate = &gates[j];
            if (from_gate && to_gate) break;
        }
        
        if (from_gate && to_gate) {
            float from_x, from_y, to_x, to_y;
            get_pin_position(*from_gate, true, wire.from_pin_index, &from_x, &from_y);
            get_pin_position(*to_gate, false, wire.to_pin_index, &to_x, &to_y);
            
            from_x += PIN_LENGTH;
            to_x -= PIN_LENGTH;
            
            SDL_SetRenderDrawColor(renderer, wire.color.r, wire.color.g, wire.color.b, 255);
            SDL_RenderLine(renderer, from_x, from_y, to_x, to_y);
        }
    }
}

void compute_gate_output(LogicGate* gate) {
    switch(gate->gate_type) {
        case 0:
            if (gate->inputs >= 2) {
                gate->output_value = AND(gate->input_values[0], gate->input_values[1]);
            }
            break;
        case 1:
            if (gate->inputs >= 2) {
                gate->output_value = OR(gate->input_values[0], gate->input_values[1]);
            }
            break;
        case 2:
            if (gate->inputs >= 1) {
                gate->output_value = NOT(gate->input_values[0]);
            }
            break;
        case 3:
            if (gate->inputs >= 2) {
                gate->output_value = NAND(gate->input_values[0], gate->input_values[1]);
            }
            break;
        case 4:
            if (gate->inputs >= 2) {
                gate->output_value = NOR(gate->input_values[0], gate->input_values[1]);
            }
            break;
        case 5:
            if (gate->inputs >= 2) {
                gate->output_value = XOR(gate->input_values[0], gate->input_values[1]);
            }
            break;
        case 6:
            break;
        case 7:
            if (gate->inputs >= 1) {
                gate->output_value = gate->input_values[0];
            }
            break;
        default:
            gate->output_value = 0;
            break;
    }
}

void propagate_signals(void* gates_ptr, int gate_count, void* wires_ptr, int wire_count) {
    LogicGate* gates = (LogicGate*)gates_ptr;
    Wire* wires = (Wire*)wires_ptr;
    
    // Reset all gate values (except INPUT gates)
    for (int i = 0; i < gate_count; i++) {
        if (!gates[i].in_palette && gates[i].gate_type != 6) { // Not palette and not INPUT
            gates[i].output_value = 0;
            if (gates[i].input_values) {
                for (int j = 0; j < gates[i].inputs; j++) {
                    gates[i].input_values[j] = 0;
                }
            }
        }
    }
    
    bool changed;
    int max_iterations = 100;
    int iterations = 0;
    
    do {
        changed = false;
        iterations++;
        
        // Propagate signals through wires
        for (int w = 0; w < wire_count; w++) {
            Wire wire = wires[w];
            
            LogicGate* from_gate = NULL;
            LogicGate* to_gate = NULL;
            for (int i = 0; i < gate_count; i++) {
                if (gates[i].id == wire.from_gate_id) from_gate = &gates[i];
                if (gates[i].id == wire.to_gate_id) to_gate = &gates[i];
                if (from_gate && to_gate) break;
            }
            
            if (from_gate && to_gate && to_gate->input_values && 
                wire.to_pin_index < to_gate->inputs) {
                
                int old_value = to_gate->input_values[wire.to_pin_index];
                to_gate->input_values[wire.to_pin_index] = from_gate->output_value;
                
                if (old_value != to_gate->input_values[wire.to_pin_index]) {
                    changed = true;
                }
            }
        }
        
        // Compute gate outputs
        for (int i = 0; i < gate_count; i++) {
            if (!gates[i].in_palette) {
                int old_output = gates[i].output_value;
                compute_gate_output(&gates[i]);
                if (old_output != gates[i].output_value) {
                    changed = true;
                }
            }
        }
        
    } while (changed && iterations < max_iterations);
    
    if (iterations >= max_iterations) {
        printf("Warning: Signal propagation reached maximum iterations\n");
    }
}

void draw_palette(SDL_Renderer* renderer) {
    SDL_SetRenderDrawColor(renderer, 220, 220, 220, 255);
    SDL_FRect palette_bg = {0, 0, PALETTE_WIDTH, WINDOW_HEIGHT};
    SDL_RenderFillRect(renderer, &palette_bg);
    
    SDL_SetRenderDrawColor(renderer, 150, 150, 150, 255);
    SDL_RenderLine(renderer, PALETTE_WIDTH, 0, PALETTE_WIDTH, WINDOW_HEIGHT);
    
    SDL_Color title_color = {0, 0, 0, 255};
    draw_text(renderer, "GATE PALETTE", 20, 20, title_color);
    SDL_RenderLine(renderer, 10, 40, PALETTE_WIDTH - 10, 40);
}

int create_gate_in_workspace(LogicGate* gates, int* gate_count, const char* name, SDL_Color color, SDL_Color selected_color, int inputs, int outputs, float x, float y, int id) {
    if (*gate_count >= MAX_GATES) return -1;
    
    int gate_type = 0;
    if (strcmp(name, "AND") == 0) gate_type = 0;
    else if (strcmp(name, "OR") == 0) gate_type = 1;
    else if (strcmp(name, "NOT") == 0) gate_type = 2;
    else if (strcmp(name, "NAND") == 0) gate_type = 3;
    else if (strcmp(name, "NOR") == 0) gate_type = 4;
    else if (strcmp(name, "XOR") == 0) gate_type = 5;
    else if (strcmp(name, "INPUT") == 0) gate_type = 6;
    else if (strcmp(name, "OUTPUT") == 0) gate_type = 7;
    
    gates[*gate_count] = (LogicGate){
        name,
        {x, y, GATE_WIDTH, GATE_HEIGHT},
        color,
        selected_color,
        inputs,
        outputs,
        false,
        false,
        0, 0,
        false,
        id,
        NULL,
        0,
        gate_type
    };
    
    if (inputs > 0) {
        gates[*gate_count].input_values = malloc(inputs * sizeof(int));
        for (int i = 0; i < inputs; i++) {
            gates[*gate_count].input_values[i] = 0;
        }
    }
    
    return (*gate_count)++;
}

bool is_point_near_pin(LogicGate gate, float point_x, float point_y, bool* is_output, int* pin_index) {
    for (int i = 0; i < gate.inputs; i++) {
        float pin_x, pin_y;
        get_pin_position(gate, false, i, &pin_x, &pin_y);
        pin_x -= PIN_LENGTH;
        
        float distance = sqrtf((point_x - pin_x) * (point_x - pin_x) + 
                             (point_y - pin_y) * (point_y - pin_y));
        if (distance <= PIN_RADIUS + 5) {
            *is_output = false;
            *pin_index = i;
            return true;
        }
    }
    
    for (int i = 0; i < gate.outputs; i++) {
        float pin_x, pin_y;
        get_pin_position(gate, true, i, &pin_x, &pin_y);
        pin_x += PIN_LENGTH;
        
        float distance = sqrtf((point_x - pin_x) * (point_x - pin_x) + 
                             (point_y - pin_y) * (point_y - pin_y));
        if (distance <= PIN_RADIUS + 5) {
            *is_output = true;
            *pin_index = i;
            return true;
        }
    }
    
    return false;
}


// Function to draw the truth table button
void draw_truth_table_button(SDL_Renderer* renderer) {
    SDL_Color button_color = {100, 150, 255, 255};
    SDL_Color text_color = {255, 255, 255, 255};
    SDL_Color border_color = {0, 0, 0, 255};
    
    // Draw button background
    SDL_SetRenderDrawColor(renderer, button_color.r, button_color.g, button_color.b, 255);
    SDL_FRect button_rect = {BUTTON_X, BUTTON_Y, BUTTON_WIDTH, BUTTON_HEIGHT};
    SDL_RenderFillRect(renderer, &button_rect);
    
    // Draw button border
    SDL_SetRenderDrawColor(renderer, border_color.r, border_color.g, border_color.b, 255);
    SDL_RenderRect(renderer, &button_rect);
    
    // Draw button text (centered)
    float text_x = BUTTON_X + (BUTTON_WIDTH - 11*12) / 2; // "TRUTH TABLE" has 11 chars
    float text_y = BUTTON_Y + (BUTTON_HEIGHT - 10) / 2;
    draw_text(renderer, "TRUTH TABLE", text_x, text_y, text_color);
}


int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }
    
    // Create fullscreen window
    SDL_Window* window = SDL_CreateWindow("Logic Circuit Simulator - Fullscreen Mode!",
                                         WINDOW_WIDTH, WINDOW_HEIGHT,
                                         SDL_WINDOW_RESIZABLE);
    if (!window) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);
    if (!renderer) {
        printf("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    
    LogicGate gates[MAX_GATES];
    int gate_count = 0;
    Wire wires[MAX_WIRES];
    int wire_count = 0;
    int next_gate_id = 1;
    
    LogicGate palette_gates[] = {
        {"AND",     {20, 60, GATE_WIDTH-20, GATE_HEIGHT-20}, 
         {70, 130, 180, 255}, {120, 180, 230, 255}, 2, 1, false, false, 0, 0, true, 0},
        {"OR",      {20, 160, GATE_WIDTH-20, GATE_HEIGHT-20}, 
         {220, 100, 80, 255}, {255, 150, 100, 255}, 2, 1, false, false, 0, 0, true, 0},
        {"NOT",     {20, 260, GATE_WIDTH-20, GATE_HEIGHT-20}, 
         {85, 160, 70, 255}, {135, 210, 120, 255}, 1, 1, false, false, 0, 0, true, 0},
        {"NAND",    {20, 360, GATE_WIDTH-20, GATE_HEIGHT-20}, 
         {180, 120, 200, 255}, {230, 170, 255, 255}, 2, 1, false, false, 0, 0, true, 0},
        {"NOR",     {20, 460, GATE_WIDTH-20, GATE_HEIGHT-20}, 
         {210, 160, 60, 255}, {255, 210, 110, 255}, 2, 1, false, false, 0, 0, true, 0},
        {"XOR",     {20, 560, GATE_WIDTH-20, GATE_HEIGHT-20}, 
         {60, 180, 160, 255}, {110, 230, 210, 255}, 2, 1, false, false, 0, 0, true, 0},
        {"INPUT",   {20, 660, GATE_WIDTH-20, GATE_HEIGHT-20}, 
         {150, 100, 100, 255}, {200, 150, 150, 255}, 0, 1, false, false, 0, 0, true, 0},
        {"OUTPUT",  {20, 760, GATE_WIDTH-20, GATE_HEIGHT-20}, 
         {100, 150, 100, 255}, {150, 200, 150, 255}, 1, 0, false, false, 0, 0, true, 0}
    };
    
    for (int i = 0; i < sizeof(palette_gates) / sizeof(palette_gates[0]); i++) {
        gates[gate_count++] = palette_gates[i];
    }
    
    bool wiring_mode = false;
    int source_gate_id = -1;
    int source_pin_index = -1;
    float temp_wire_end_x = 0, temp_wire_end_y = 0;
    
    bool running = true;
    SDL_Event event;
    bool creating_new_gate = false;
    LogicGate new_gate_template;
    int selected_palette_index = -1;
    
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
            else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                if (event.button.button == SDL_BUTTON_LEFT) {
                    float mouse_x = event.button.x;
                    float mouse_y = event.button.y;

                    // Check if truth table button was clicked
                    if (mouse_x >= BUTTON_X && mouse_x <= BUTTON_X + BUTTON_WIDTH && mouse_y >= BUTTON_Y && mouse_y <= BUTTON_Y + BUTTON_HEIGHT) {
                        printf("Truth table button clicked! Generating truth table...\n");
                        generate_truth_table((void*)gates, gate_count, (void*)wires, wire_count);
                        continue; // Skip other click handling since button was clicked
                    }
                    
                    if (mouse_x < PALETTE_WIDTH) {
                        for (int i = 0; i < gate_count; i++) {
                            gates[i].is_selected = false;
                        }
                        
                        for (int i = 0; i < gate_count; i++) {
                            if (gates[i].in_palette && 
                                mouse_x >= gates[i].rect.x && 
                                mouse_x <= gates[i].rect.x + gates[i].rect.w &&
                                mouse_y >= gates[i].rect.y && 
                                mouse_y <= gates[i].rect.y + gates[i].rect.h) {
                                
                                gates[i].is_selected = true;
                                selected_palette_index = i;
                                creating_new_gate = true;
                                
                                new_gate_template = gates[i];
                                new_gate_template.in_palette = false;
                                new_gate_template.rect.w = GATE_WIDTH;
                                new_gate_template.rect.h = GATE_HEIGHT;
                                new_gate_template.rect.x = mouse_x;
                                new_gate_template.rect.y = mouse_y;
                                new_gate_template.is_dragging = true;
                                new_gate_template.drag_offset_x = 0;
                                new_gate_template.drag_offset_y = 0;
                                new_gate_template.id = next_gate_id++;
                                break;
                            }
                        }
                    } else {
                        if (wiring_mode) {
                            for (int i = 0; i < gate_count; i++) {
                                if (!gates[i].in_palette) {
                                    bool is_output;
                                    int pin_index;
                                    if (is_point_near_pin(gates[i], mouse_x, mouse_y, &is_output, &pin_index)) {
                                        if (!is_output) {
                                            if (wire_count < MAX_WIRES) {
                                                wires[wire_count++] = (Wire){
                                                    source_gate_id,
                                                    source_pin_index,
                                                    gates[i].id,
                                                    pin_index,
                                                    {0, 0, 0, 255}
                                                };
                                            }
                                            wiring_mode = false;
                                            break;
                                        }
                                    }
                                }
                            }
                            wiring_mode = false;
                        } else {
                            bool input_gate_clicked = false;
                            for (int i = 0; i < gate_count; i++) {
                                if (!gates[i].in_palette && gates[i].gate_type == 6) {
                                    if (mouse_x >= gates[i].rect.x && 
                                        mouse_x <= gates[i].rect.x + gates[i].rect.w &&
                                        mouse_y >= gates[i].rect.y && 
                                        mouse_y <= gates[i].rect.y + gates[i].rect.h) {
                                        
                                        gates[i].output_value = !gates[i].output_value;
                                        input_gate_clicked = true;
                                        printf("INPUT gate %d toggled to: %d\n", gates[i].id, gates[i].output_value);
                                        break;
                                    }
                                }
                            }
                            
                            if (input_gate_clicked) {
                                continue;
                            }
                            
                            bool pin_clicked = false;
                            for (int i = 0; i < gate_count; i++) {
                                if (!gates[i].in_palette) {
                                    bool is_output;
                                    int pin_index;
                                    if (is_point_near_pin(gates[i], mouse_x, mouse_y, &is_output, &pin_index)) {
                                        if (is_output) {
                                            wiring_mode = true;
                                            source_gate_id = gates[i].id;
                                            source_pin_index = pin_index;
                                            temp_wire_end_x = mouse_x;
                                            temp_wire_end_y = mouse_y;
                                            pin_clicked = true;
                                            break;
                                        }
                                    }
                                }
                            }
                            
                            if (!pin_clicked) {
                                for (int i = 0; i < gate_count; i++) {
                                    gates[i].is_selected = false;
                                }
                                
                                for (int i = 0; i < gate_count; i++) {
                                    if (!gates[i].in_palette && 
                                        mouse_x >= gates[i].rect.x && 
                                        mouse_x <= gates[i].rect.x + gates[i].rect.w &&
                                        mouse_y >= gates[i].rect.y && 
                                        mouse_y <= gates[i].rect.y + gates[i].rect.h) {
                                        
                                        gates[i].is_selected = true;
                                        gates[i].is_dragging = true;
                                        gates[i].drag_offset_x = mouse_x - gates[i].rect.x;
                                        gates[i].drag_offset_y = mouse_y - gates[i].rect.y;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
            }
            else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP) {
                if (event.button.button == SDL_BUTTON_LEFT) {
                    if (creating_new_gate) {
                        if (event.button.x > PALETTE_WIDTH) {
                            create_gate_in_workspace(gates, &gate_count, 
                                                   new_gate_template.name,
                                                   new_gate_template.color,
                                                   new_gate_template.selected_color,
                                                   new_gate_template.inputs,
                                                   new_gate_template.outputs,
                                                   new_gate_template.rect.x,
                                                   new_gate_template.rect.y,
                                                   new_gate_template.id);
                        }
                        creating_new_gate = false;
                        if (selected_palette_index != -1) {
                            gates[selected_palette_index].is_selected = false;
                            selected_palette_index = -1;
                        }
                    }
                    
                    for (int i = 0; i < gate_count; i++) {
                        gates[i].is_dragging = false;
                    }
                }
            }
            else if (event.type == SDL_EVENT_MOUSE_MOTION) {
                if (wiring_mode) {
                    temp_wire_end_x = event.motion.x;
                    temp_wire_end_y = event.motion.y;
                }
                else if (creating_new_gate) {
                    new_gate_template.rect.x = event.motion.x;
                    new_gate_template.rect.y = event.motion.y;
                } else {
                    for (int i = 0; i < gate_count; i++) {
                        if (gates[i].is_dragging && !gates[i].in_palette) {
                            gates[i].rect.x = event.motion.x - gates[i].drag_offset_x;
                            gates[i].rect.y = event.motion.y - gates[i].drag_offset_y;
                        }
                    }
                }
            }
        }
        
        // CRITICAL: Call signal propagation
        propagate_signals((void*)gates, gate_count, (void*)wires, wire_count);
        
        SDL_SetRenderDrawColor(renderer, 240, 240, 240, 255);
        SDL_RenderClear(renderer);
        draw_truth_table_button(renderer);
        draw_palette(renderer);
        
        draw_wires(renderer, wires, wire_count, gates, gate_count);
        
        for (int i = 0; i < gate_count; i++) {
            if (gates[i].in_palette) {
                draw_logic_gate(renderer, gates[i]);
            }
        }
        
        for (int i = 0; i < gate_count; i++) {
            if (!gates[i].in_palette) {
                draw_logic_gate(renderer, gates[i]);
            }
        }
        
        if (creating_new_gate) {
            draw_logic_gate(renderer, new_gate_template);
        }
        
        if (wiring_mode) {
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
            for (int i = 0; i < gate_count; i++) {
                if (gates[i].id == source_gate_id) {
                    float start_x, start_y;
                    get_pin_position(gates[i], true, source_pin_index, &start_x, &start_y);
                    start_x += PIN_LENGTH;
                    SDL_RenderLine(renderer, start_x, start_y, temp_wire_end_x, temp_wire_end_y);
                    break;
                }
            }
        }
        
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }
    
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    return 0;
}