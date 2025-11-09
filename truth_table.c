#include "truth_table.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define TRUTH_TABLE_WIDTH 800
#define TRUTH_TABLE_HEIGHT 600
#define CELL_WIDTH 60
#define CELL_HEIGHT 30
#define HEADER_HEIGHT 40
#define MARGIN 20
#define MAX_GATES 50  

// Define the structures locally since we can't include circuit_visual.h
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

// Forward declare the propagate_signals function (defined in your main file)
void propagate_signals(void* gates_ptr, int gate_count, void* wires_ptr, int wire_count);
void propagate_signals(void* gates_ptr, int gate_count, void* wires_ptr, int wire_count);

// Function to find all INPUT gates in the circuit
int find_input_gates(void* gates_ptr, int gate_count, int* input_gate_indices) {
    LogicGate* gates = (LogicGate*)gates_ptr;
    int count = 0;
    for (int i = 0; i < gate_count; i++) {
        if (!gates[i].in_palette && gates[i].gate_type == 6) { // INPUT gate
            input_gate_indices[count++] = i;
        }
    }
    return count;
}

// Function to find all OUTPUT gates in the circuit
int find_output_gates(void* gates_ptr, int gate_count, int* output_gate_indices) {
    LogicGate* gates = (LogicGate*)gates_ptr;
    int count = 0;
    for (int i = 0; i < gate_count; i++) {
        if (!gates[i].in_palette && gates[i].gate_type == 7) { // OUTPUT gate
            output_gate_indices[count++] = i;
        }
    }
    return count;
}

// Function to simulate circuit with given input values
void simulate_circuit_with_inputs(void* gates_ptr, int gate_count, void* wires_ptr, int wire_count, 
                                 int* input_values, int* output_values) {
    LogicGate* gates = (LogicGate*)gates_ptr;
    Wire* wires = (Wire*)wires_ptr;
    
    // Create a temporary copy of the circuit to avoid modifying the original
    LogicGate* temp_gates = malloc(gate_count * sizeof(LogicGate));
    for (int i = 0; i < gate_count; i++) {
        temp_gates[i] = gates[i];
        if (temp_gates[i].input_values && temp_gates[i].inputs > 0) {
            temp_gates[i].input_values = malloc(temp_gates[i].inputs * sizeof(int));
            for (int j = 0; j < temp_gates[i].inputs; j++) {
                temp_gates[i].input_values[j] = 0;
            }
        }
        temp_gates[i].output_value = 0;
    }
    
    // Set input values
    int input_gate_indices[MAX_GATES];
    int num_inputs = find_input_gates(temp_gates, gate_count, input_gate_indices);
    for (int i = 0; i < num_inputs; i++) {
        temp_gates[input_gate_indices[i]].output_value = input_values[i];
    }
    
    // Propagate signals
    propagate_signals((void*)temp_gates, gate_count, (void*)wires, wire_count);
    
    // Read output values
    int output_gate_indices[MAX_GATES];
    int num_outputs = find_output_gates(temp_gates, gate_count, output_gate_indices);
    for (int i = 0; i < num_outputs; i++) {
        output_values[i] = temp_gates[output_gate_indices[i]].output_value;
    }
    
    // Cleanup
    for (int i = 0; i < gate_count; i++) {
        if (temp_gates[i].input_values && !gates[i].in_palette) {
            free(temp_gates[i].input_values);
        }
    }
    free(temp_gates);
}

// Text drawing function for truth table
void draw_table_text(SDL_Renderer* renderer, const char* text, float x, float y, SDL_Color color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    
    float current_x = x;
    for (int i = 0; text[i] != '\0'; i++) {
        char c = text[i];
        // Simple character drawing
        switch(c) {
            case '0':
            case '1':
                SDL_RenderLine(renderer, current_x+2, y+5, current_x+8, y+5);
                SDL_RenderLine(renderer, current_x+2, y+15, current_x+8, y+15);
                SDL_RenderLine(renderer, current_x+2, y+5, current_x+2, y+15);
                SDL_RenderLine(renderer, current_x+8, y+5, current_x+8, y+15);
                if (c == '0') {
                    // Draw diagonal for 0
                    SDL_RenderLine(renderer, current_x+2, y+5, current_x+8, y+15);
                }
                break;
            case 'I':
                SDL_RenderLine(renderer, current_x+2, y+5, current_x+8, y+5);
                SDL_RenderLine(renderer, current_x+5, y+5, current_x+5, y+15);
                SDL_RenderLine(renderer, current_x+2, y+15, current_x+8, y+15);
                break;
            case 'O':
                SDL_RenderLine(renderer, current_x+2, y+5, current_x+8, y+5);
                SDL_RenderLine(renderer, current_x+8, y+5, current_x+8, y+15);
                SDL_RenderLine(renderer, current_x+2, y+15, current_x+8, y+15);
                SDL_RenderLine(renderer, current_x+2, y+5, current_x+2, y+15);
                break;
            default:
                // Draw simple rectangle for other characters
                SDL_RenderLine(renderer, current_x+2, y+5, current_x+8, y+5);
                SDL_RenderLine(renderer, current_x+2, y+15, current_x+8, y+15);
                SDL_RenderLine(renderer, current_x+2, y+5, current_x+2, y+15);
                SDL_RenderLine(renderer, current_x+8, y+5, current_x+8, y+15);
                break;
        }
        current_x += 12;
    }
}

// Function to draw the truth table window
void draw_truth_table_window(void* gates_ptr, int gate_count, int num_inputs, int num_outputs,
                           int* input_gate_indices, int* output_gate_indices) {
    LogicGate* gates = (LogicGate*)gates_ptr;
    
    SDL_Window* table_window = SDL_CreateWindow("Truth Table",
                                               TRUTH_TABLE_WIDTH, TRUTH_TABLE_HEIGHT,
                                               SDL_WINDOW_RESIZABLE);
    if (!table_window) {
        printf("Truth table window could not be created!\n");
        return;
    }
    
    SDL_Renderer* table_renderer = SDL_CreateRenderer(table_window, NULL);
    if (!table_renderer) {
        printf("Truth table renderer could not be created!\n");
        SDL_DestroyWindow(table_window);
        return;
    }
    
    int num_combinations = 1 << num_inputs; // 2^num_inputs
    
    bool table_running = true;
    SDL_Event table_event;
    
    while (table_running) {
        while (SDL_PollEvent(&table_event)) {
            if (table_event.type == SDL_EVENT_QUIT) {
                table_running = false;
            }
        }
        
        // Clear the table window
        SDL_SetRenderDrawColor(table_renderer, 255, 255, 255, 255);
        SDL_RenderClear(table_renderer);
        
        // Draw table headers
        SDL_Color header_color = {0, 0, 0, 255};
        SDL_Color cell_color = {0, 0, 0, 255};
        
        // Input headers
        for (int i = 0; i < num_inputs; i++) {
            char header[10];
            sprintf(header, "I%d", i+1);
            draw_table_text(table_renderer, header, MARGIN + i * CELL_WIDTH, MARGIN, header_color);
        }
        
        // Output headers
        for (int i = 0; i < num_outputs; i++) {
            char header[10];
            sprintf(header, "O%d", i+1);
            draw_table_text(table_renderer, header, 
                           MARGIN + (num_inputs + i) * CELL_WIDTH, MARGIN, header_color);
        }
        
        // Draw table grid and data
        for (int row = 0; row < num_combinations; row++) {
            // Calculate input values for this combination
            int input_values[MAX_GATES];
            for (int i = 0; i < num_inputs; i++) {
                input_values[i] = (row >> (num_inputs - 1 - i)) & 1;
            }
            
            // Simulate circuit with these inputs
            int output_values[MAX_GATES];
            simulate_circuit_with_inputs(gates, gate_count, NULL, 0, input_values, output_values);
            
            // Draw input values
            for (int col = 0; col < num_inputs; col++) {
                char value[2];
                sprintf(value, "%d", input_values[col]);
                draw_table_text(table_renderer, value, 
                               MARGIN + col * CELL_WIDTH, 
                               HEADER_HEIGHT + row * CELL_HEIGHT, cell_color);
            }
            
            // Draw output values
            for (int col = 0; col < num_outputs; col++) {
                char value[2];
                sprintf(value, "%d", output_values[col]);
                draw_table_text(table_renderer, value, 
                               MARGIN + (num_inputs + col) * CELL_WIDTH, 
                               HEADER_HEIGHT + row * CELL_HEIGHT, cell_color);
            }
            
            // Draw horizontal grid lines
            SDL_SetRenderDrawColor(table_renderer, 200, 200, 200, 255);
            SDL_RenderLine(table_renderer, MARGIN, HEADER_HEIGHT + row * CELL_HEIGHT, 
                          MARGIN + (num_inputs + num_outputs) * CELL_WIDTH, 
                          HEADER_HEIGHT + row * CELL_HEIGHT);
        }
        
        // Draw vertical grid lines
        for (int col = 0; col <= num_inputs + num_outputs; col++) {
            SDL_SetRenderDrawColor(table_renderer, 200, 200, 200, 255);
            SDL_RenderLine(table_renderer, MARGIN + col * CELL_WIDTH, MARGIN, 
                          MARGIN + col * CELL_WIDTH, 
                          HEADER_HEIGHT + num_combinations * CELL_HEIGHT);
        }
        
        SDL_RenderPresent(table_renderer);
        SDL_Delay(16);
    }
    
    SDL_DestroyRenderer(table_renderer);
    SDL_DestroyWindow(table_window);
}

// Main function to generate truth table
void generate_truth_table(void* gates_ptr, int gate_count, void* wires_ptr, int wire_count) {
    LogicGate* gates = (LogicGate*)gates_ptr;
    Wire* wires = (Wire*)wires_ptr;
    
    printf("Generating truth table...\n");
    
    // Find all input and output gates
    int input_gate_indices[MAX_GATES];
    int num_inputs = find_input_gates(gates, gate_count, input_gate_indices);
    
    int output_gate_indices[MAX_GATES];
    int num_outputs = find_output_gates(gates, gate_count, output_gate_indices);
    
    if (num_inputs == 0) {
        printf("No INPUT gates found in the circuit!\n");
        return;
    }
    
    if (num_outputs == 0) {
        printf("No OUTPUT gates found in the circuit!\n");
        return;
    }
    
    printf("Found %d input gates and %d output gates\n", num_inputs, num_outputs);
    
    // Open truth table window
    draw_truth_table_window((void*)gates, gate_count, num_inputs, num_outputs, 
                       input_gate_indices, output_gate_indices);
}