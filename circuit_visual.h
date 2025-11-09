#ifndef CIRCUIT_VISUAL_H
#define CIRCUIT_VISUAL_H

#include <SDL3/SDL.h>
#include <stdbool.h>

#define MAX_GATES 50
#define MAX_WIRES 100

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

// Function declarations from your visual circuit
void propagate_signals(LogicGate* gates, int gate_count, Wire* wires, int wire_count);
void compute_gate_output(LogicGate* gate);

#endif