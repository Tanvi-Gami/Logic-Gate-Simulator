#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#define MAX_GATES 100
#define MAX_INPUTS 10
#define MAX_OUTPUTS 10
#define MAX_WIRES 200
#define WORKSPACE_WIDTH 80
#define WORKSPACE_HEIGHT 24

// Gate types
typedef enum {
    GATE_NOT,
    GATE_AND,
    GATE_OR,
    GATE_XOR,
    GATE_NAND,
    GATE_NOR,
    GATE_INPUT,
    GATE_OUTPUT
} GateType;

// Gate structure
typedef struct {
    int id;
    GateType type;
    int x, y;  // Position in workspace
    int input1, input2;  // IDs of input gates or -1 if not connected
    int output;  // Current output value (0 or 1)
    char label[20];
} Gate;

// Wire structure
typedef struct {
    int id;
    int from_gate;  // Source gate ID
    int from_pin;   // 1 or 2 for input gates
    int to_gate;    // Destination gate ID
    int to_pin;     // 1 or 2 for input gates
} Wire;

// Circuit structure
typedef struct {
    Gate gates[MAX_GATES];
    Wire wires[MAX_WIRES];
    int gate_count;
    int wire_count;
    int input_gates[MAX_INPUTS];
    int output_gates[MAX_OUTPUTS];
    int input_count;
    int output_count;
} Circuit;

// Undo/Redo action structure
typedef struct Action {
    Circuit circuit;
    char description[50];
    struct Action* next;
    struct Action* prev;
} Action;

// Global variables
Circuit current_circuit;
Action* undo_stack = NULL;
Action* redo_stack = NULL;
int next_gate_id = 1;
int next_wire_id = 1;

// Function prototypes
void initialize_circuit();
void display_menu();
void display_workspace();
void add_gate(GateType type, int x, int y);
void delete_gate(int gate_id);
void add_wire(int from_gate, int to_gate, int to_pin);
void delete_wire(int wire_id);
void evaluate_circuit();
int evaluate_gate(int gate_id);
void generate_truth_table();
void toggle_input(int input_index);
void display_error(const char* message);
int detect_loops();
void save_action(const char* description);
void undo();
void redo();
void save_circuit(const char* filename);
void load_circuit(const char* filename);

// Logic gate evaluation functions
int gate_not(int input) { return !input; }
int gate_and(int input1, int input2) { return input1 && input2; }
int gate_or(int input1, int input2) { return input1 || input2; }
int gate_xor(int input1, int input2) { return input1 != input2; }
int gate_nand(int input1, int input2) { return !(input1 && input2); }
int gate_nor(int input1, int input2) { return !(input1 || input2); }

// Initialize a new circuit
void initialize_circuit() {
    memset(&current_circuit, 0, sizeof(Circuit));
    current_circuit.gate_count = 0;
    current_circuit.wire_count = 0;
    current_circuit.input_count = 0;
    current_circuit.output_count = 0;
    next_gate_id = 1;
    next_wire_id = 1;
    
    // Free undo/redo stacks
    while (undo_stack) {
        Action* temp = undo_stack;
        undo_stack = undo_stack->next;
        free(temp);
    }
    while (redo_stack) {
        Action* temp = redo_stack;
        redo_stack = redo_stack->next;
        free(temp);
    }
}

// Display main menu
void display_menu() {
    printf("\n=== Digital Logic Circuit Simulator ===\n");
    printf("1. Add Gate\n");
    printf("2. Add Wire\n");
    printf("3. Delete Gate\n");
    printf("4. Delete Wire\n");
    printf("5. Toggle Input\n");
    printf("6. Evaluate Circuit\n");
    printf("7. Generate Truth Table\n");
    printf("8. Display Workspace\n");
    printf("9. Undo\n");
    printf("10. Redo\n");
    printf("11. Save Circuit\n");
    printf("12. Load Circuit\n");
    printf("13. Clear Workspace\n");
    printf("0. Exit\n");
    printf("Choose an option: ");
}

// Display the workspace with ASCII graphics
void display_workspace() {
    char workspace[WORKSPACE_HEIGHT][WORKSPACE_WIDTH];
    
    // Initialize workspace with spaces
    for (int y = 0; y < WORKSPACE_HEIGHT; y++) {
        for (int x = 0; x < WORKSPACE_WIDTH; x++) {
            workspace[y][x] = ' ';
        }
    }
    
    // Draw wires
    for (int i = 0; i < current_circuit.wire_count; i++) {
        Wire wire = current_circuit.wires[i];
        Gate from_gate, to_gate;
        
        // Find gates
        for (int j = 0; j < current_circuit.gate_count; j++) {
            if (current_circuit.gates[j].id == wire.from_gate) from_gate = current_circuit.gates[j];
            if (current_circuit.gates[j].id == wire.to_gate) to_gate = current_circuit.gates[j];
        }
        
        // Simple line drawing (would be more complex in a real implementation)
        int from_x = from_gate.x + 5;
        int from_y = from_gate.y + 1;
        int to_x = to_gate.x;
        int to_y = to_gate.y + 1;
        
        // Draw horizontal and vertical lines
        if (from_y == to_y) {
            for (int x = from_x; x <= to_x; x++) {
                if (x >= 0 && x < WORKSPACE_WIDTH && from_y >= 0 && from_y < WORKSPACE_HEIGHT) {
                    workspace[from_y][x] = '-';
                }
            }
        } else {
            // Draw vertical segment
            for (int y = from_y; y <= to_y; y++) {
                if (from_x >= 0 && from_x < WORKSPACE_WIDTH && y >= 0 && y < WORKSPACE_HEIGHT) {
                    workspace[y][from_x] = '|';
                }
            }
            // Draw horizontal segment
            for (int x = from_x; x <= to_x; x++) {
                if (x >= 0 && x < WORKSPACE_WIDTH && to_y >= 0 && to_y < WORKSPACE_HEIGHT) {
                    workspace[to_y][x] = '-';
                }
            }
        }
        
        // Draw connection points
        if (from_x >= 0 && from_x < WORKSPACE_WIDTH && from_y >= 0 && from_y < WORKSPACE_HEIGHT) {
            workspace[from_y][from_x] = '+';
        }
        if (to_x >= 0 && to_x < WORKSPACE_WIDTH && to_y >= 0 && to_y < WORKSPACE_HEIGHT) {
            workspace[to_y][to_x] = '+';
        }
    }
    
    // Draw gates
    for (int i = 0; i < current_circuit.gate_count; i++) {
        Gate gate = current_circuit.gates[i];
        int x = gate.x;
        int y = gate.y;
        
        if (x >= 0 && x < WORKSPACE_WIDTH - 6 && y >= 0 && y < WORKSPACE_HEIGHT - 3) {
            // Draw gate box
            workspace[y][x] = '+';
            workspace[y][x+6] = '+';
            workspace[y+2][x] = '+';
            workspace[y+2][x+6] = '+';
            
            for (int dx = 1; dx < 6; dx++) {
                workspace[y][x+dx] = '-';
                workspace[y+2][x+dx] = '-';
            }
            
            workspace[y+1][x] = '|';
            workspace[y+1][x+6] = '|';
            
            // Draw gate label
            char label[3];
            switch (gate.type) {
                case GATE_NOT: strcpy(label, "NOT"); break;
                case GATE_AND: strcpy(label, "AND"); break;
                case GATE_OR: strcpy(label, "OR"); break;
                case GATE_XOR: strcpy(label, "XOR"); break;
                case GATE_NAND: strcpy(label, "NAND"); break;
                case GATE_NOR: strcpy(label, "NOR"); break;
                case GATE_INPUT: strcpy(label, "IN"); break;
                case GATE_OUTPUT: strcpy(label, "OUT"); break;
            }
            
            workspace[y+1][x+2] = label[0];
            if (strlen(label) > 1) workspace[y+1][x+3] = label[1];
            if (strlen(label) > 2) workspace[y+1][x+4] = label[2];
            
            // Draw output value if evaluated
            if (gate.type == GATE_INPUT || gate.type == GATE_OUTPUT) {
                workspace[y+1][x+1] = gate.output ? '1' : '0';
            }
            
            // Draw gate ID
            char id_str[3];
            sprintf(id_str, "%d", gate.id);
            workspace[y+3][x+2] = id_str[0];
            if (strlen(id_str) > 1) workspace[y+3][x+3] = id_str[1];
        }
    }
    
    // Print workspace
    printf("\n");
    for (int y = 0; y < WORKSPACE_HEIGHT; y++) {
        for (int x = 0; x < WORKSPACE_WIDTH; x++) {
            printf("%c", workspace[y][x]);
        }
        printf("\n");
    }
    
    // Print gate list
    printf("\nGates in circuit:\n");
    for (int i = 0; i < current_circuit.gate_count; i++) {
        Gate gate = current_circuit.gates[i];
        const char* type_str;
        switch (gate.type) {
            case GATE_NOT: type_str = "NOT"; break;
            case GATE_AND: type_str = "AND"; break;
            case GATE_OR: type_str = "OR"; break;
            case GATE_XOR: type_str = "XOR"; break;
            case GATE_NAND: type_str = "NAND"; break;
            case GATE_NOR: type_str = "NOR"; break;
            case GATE_INPUT: type_str = "INPUT"; break;
            case GATE_OUTPUT: type_str = "OUTPUT"; break;
        }
        printf("ID: %d, Type: %s, Pos: (%d,%d), Output: %d\n", 
               gate.id, type_str, gate.x, gate.y, gate.output);
    }
    
    // Print wire list
    if (current_circuit.wire_count > 0) {
        printf("\nWires in circuit:\n");
        for (int i = 0; i < current_circuit.wire_count; i++) {
            Wire wire = current_circuit.wires[i];
            printf("ID: %d, From: %d, To: %d (pin %d)\n", 
                   wire.id, wire.from_gate, wire.to_gate, wire.to_pin);
        }
    }
}

// Add a gate to the circuit
void add_gate(GateType type, int x, int y) {
    if (current_circuit.gate_count >= MAX_GATES) {
        display_error("Cannot add more gates. Maximum limit reached.");
        return;
    }
    
    Gate new_gate;
    new_gate.id = next_gate_id++;
    new_gate.type = type;
    new_gate.x = x;
    new_gate.y = y;
    new_gate.input1 = -1;
    new_gate.input2 = -1;
    new_gate.output = 0;
    
    // Set default label
    switch (type) {
        case GATE_NOT: strcpy(new_gate.label, "NOT"); break;
        case GATE_AND: strcpy(new_gate.label, "AND"); break;
        case GATE_OR: strcpy(new_gate.label, "OR"); break;
        case GATE_XOR: strcpy(new_gate.label, "XOR"); break;
        case GATE_NAND: strcpy(new_gate.label, "NAND"); break;
        case GATE_NOR: strcpy(new_gate.label, "NOR"); break;
        case GATE_INPUT: 
            strcpy(new_gate.label, "IN");
            current_circuit.input_gates[current_circuit.input_count++] = new_gate.id;
            break;
        case GATE_OUTPUT: 
            strcpy(new_gate.label, "OUT");
            current_circuit.output_gates[current_circuit.output_count++] = new_gate.id;
            break;
    }
    
    current_circuit.gates[current_circuit.gate_count++] = new_gate;
    save_action("Add gate");
    printf("Added gate ID %d of type %s at position (%d,%d)\n", 
           new_gate.id, new_gate.label, x, y);
}

// Delete a gate from the circuit
void delete_gate(int gate_id) {
    int found = 0;
    for (int i = 0; i < current_circuit.gate_count; i++) {
        if (current_circuit.gates[i].id == gate_id) {
            // Remove from input/output lists if needed
            if (current_circuit.gates[i].type == GATE_INPUT) {
                for (int j = 0; j < current_circuit.input_count; j++) {
                    if (current_circuit.input_gates[j] == gate_id) {
                        for (int k = j; k < current_circuit.input_count - 1; k++) {
                            current_circuit.input_gates[k] = current_circuit.input_gates[k+1];
                        }
                        current_circuit.input_count--;
                        break;
                    }
                }
            }
            if (current_circuit.gates[i].type == GATE_OUTPUT) {
                for (int j = 0; j < current_circuit.output_count; j++) {
                    if (current_circuit.output_gates[j] == gate_id) {
                        for (int k = j; k < current_circuit.output_count - 1; k++) {
                            current_circuit.output_gates[k] = current_circuit.output_gates[k+1];
                        }
                        current_circuit.output_count--;
                        break;
                    }
                }
            }
            
            // Remove the gate
            for (int j = i; j < current_circuit.gate_count - 1; j++) {
                current_circuit.gates[j] = current_circuit.gates[j+1];
            }
            current_circuit.gate_count--;
            found = 1;
            break;
        }
    }
    
    if (!found) {
        display_error("Gate not found.");
        return;
    }
    
    // Remove all wires connected to this gate
    for (int i = 0; i < current_circuit.wire_count; i++) {
        if (current_circuit.wires[i].from_gate == gate_id || 
            current_circuit.wires[i].to_gate == gate_id) {
            delete_wire(current_circuit.wires[i].id);
            i--;  // Adjust index after deletion
        }
    }
    
    save_action("Delete gate");
    printf("Deleted gate ID %d and its connections\n", gate_id);
}

// Add a wire between gates
void add_wire(int from_gate, int to_gate, int to_pin) {
    if (current_circuit.wire_count >= MAX_WIRES) {
        display_error("Cannot add more wires. Maximum limit reached.");
        return;
    }
    
    // Check if gates exist
    int from_exists = 0, to_exists = 0;
    GateType to_type;
    
    for (int i = 0; i < current_circuit.gate_count; i++) {
        if (current_circuit.gates[i].id == from_gate) from_exists = 1;
        if (current_circuit.gates[i].id == to_gate) {
            to_exists = 1;
            to_type = current_circuit.gates[i].type;
        }
    }
    
    if (!from_exists || !to_exists) {
        display_error("One or both gates not found.");
        return;
    }
    
    // Check for NOT gate input constraints
    if (to_type == GATE_NOT && to_pin == 2) {
        display_error("NOT gate has only one input (use pin 1).");
        return;
    }
    
    // Check if wire already exists
    for (int i = 0; i < current_circuit.wire_count; i++) {
        if (current_circuit.wires[i].to_gate == to_gate && 
            current_circuit.wires[i].to_pin == to_pin) {
            display_error("A wire already connected to this pin.");
            return;
        }
    }
    
    Wire new_wire;
    new_wire.id = next_wire_id++;
    new_wire.from_gate = from_gate;
    new_wire.to_gate = to_gate;
    new_wire.to_pin = to_pin;
    
    current_circuit.wires[current_circuit.wire_count++] = new_wire;
    
    // Update gate connections
    for (int i = 0; i < current_circuit.gate_count; i++) {
        if (current_circuit.gates[i].id == to_gate) {
            if (to_pin == 1) {
                current_circuit.gates[i].input1 = from_gate;
            } else {
                current_circuit.gates[i].input2 = from_gate;
            }
            break;
        }
    }
    
    save_action("Add wire");
    printf("Added wire from gate %d to gate %d (pin %d)\n", from_gate, to_gate, to_pin);
}

// Delete a wire from the circuit
void delete_wire(int wire_id) {
    int found = 0;
    for (int i = 0; i < current_circuit.wire_count; i++) {
        if (current_circuit.wires[i].id == wire_id) {
            int to_gate = current_circuit.wires[i].to_gate;
            int to_pin = current_circuit.wires[i].to_pin;
            
            // Remove the wire
            for (int j = i; j < current_circuit.wire_count - 1; j++) {
                current_circuit.wires[j] = current_circuit.wires[j+1];
            }
            current_circuit.wire_count--;
            found = 1;
            
            // Update gate connections
            for (int j = 0; j < current_circuit.gate_count; j++) {
                if (current_circuit.gates[j].id == to_gate) {
                    if (to_pin == 1) {
                        current_circuit.gates[j].input1 = -1;
                    } else {
                        current_circuit.gates[j].input2 = -1;
                    }
                    break;
                }
            }
            break;
        }
    }
    
    if (!found) {
        display_error("Wire not found.");
        return;
    }
    
    save_action("Delete wire");
    printf("Deleted wire ID %d\n", wire_id);
}

// Evaluate the entire circuit
void evaluate_circuit() {
    // Reset all outputs except inputs
    for (int i = 0; i < current_circuit.gate_count; i++) {
        if (current_circuit.gates[i].type != GATE_INPUT) {
            current_circuit.gates[i].output = 0;
        }
    }
    
    // Evaluate all gates
    for (int i = 0; i < current_circuit.gate_count; i++) {
        evaluate_gate(current_circuit.gates[i].id);
    }
    
    printf("Circuit evaluation completed.\n");
}

// Evaluate a specific gate
int evaluate_gate(int gate_id) {
    Gate* gate = NULL;
    int index = -1;
    
    // Find the gate
    for (int i = 0; i < current_circuit.gate_count; i++) {
        if (current_circuit.gates[i].id == gate_id) {
            gate = &current_circuit.gates[i];
            index = i;
            break;
        }
    }
    
    if (!gate) return 0;
    
    // If already evaluated, return the value
    if (gate->output != 0 || gate->type == GATE_INPUT) {
        return gate->output;
    }
    
    // Get input values
    int input1_val = 0, input2_val = 0;
    
    if (gate->input1 != -1) {
        input1_val = evaluate_gate(gate->input1);
    }
    
    if (gate->input2 != -1) {
        input2_val = evaluate_gate(gate->input2);
    }
    
    // Calculate output based on gate type
    switch (gate->type) {
        case GATE_NOT:
            gate->output = gate_not(input1_val);
            break;
        case GATE_AND:
            gate->output = gate_and(input1_val, input2_val);
            break;
        case GATE_OR:
            gate->output = gate_or(input1_val, input2_val);
            break;
        case GATE_XOR:
            gate->output = gate_xor(input1_val, input2_val);
            break;
        case GATE_NAND:
            gate->output = gate_nand(input1_val, input2_val);
            break;
        case GATE_NOR:
            gate->output = gate_nor(input1_val, input2_val);
            break;
        case GATE_OUTPUT:
            gate->output = input1_val;  // Output just passes through the input
            break;
        default:
            gate->output = 0;
    }
    
    return gate->output;
}

// Generate truth table for the circuit
void generate_truth_table() {
    if (current_circuit.input_count == 0) {
        display_error("No input gates in the circuit.");
        return;
    }
    
    if (current_circuit.output_count == 0) {
        display_error("No output gates in the circuit.");
        return;
    }
    
    int num_inputs = current_circuit.input_count;
    int num_outputs = current_circuit.output_count;
    int num_combinations = 1 << num_inputs;  // 2^num_inputs
    
    printf("\nTruth Table:\n");
    
    // Print header
    printf("|");
    for (int i = 0; i < num_inputs; i++) {
        printf(" I%d |", i+1);
    }
    for (int i = 0; i < num_outputs; i++) {
        printf(" O%d |", i+1);
    }
    printf("\n");
    
    // Print separator
    printf("|");
    for (int i = 0; i < num_inputs + num_outputs; i++) {
        printf("----|");
    }
    printf("\n");
    
    // Generate all input combinations and evaluate
    for (int comb = 0; comb < num_combinations; comb++) {
        // Set input values
        for (int i = 0; i < num_inputs; i++) {
            int input_val = (comb >> (num_inputs - 1 - i)) & 1;
            for (int j = 0; j < current_circuit.gate_count; j++) {
                if (current_circuit.gates[j].id == current_circuit.input_gates[i]) {
                    current_circuit.gates[j].output = input_val;
                    break;
                }
            }
        }
        
        // Evaluate circuit
        evaluate_circuit();
        
        // Print row
        printf("|");
        for (int i = 0; i < num_inputs; i++) {
            printf("  %d  |", (comb >> (num_inputs - 1 - i)) & 1);
        }
        for (int i = 0; i < num_outputs; i++) {
            for (int j = 0; j < current_circuit.gate_count; j++) {
                if (current_circuit.gates[j].id == current_circuit.output_gates[i]) {
                    printf("  %d  |", current_circuit.gates[j].output);
                    break;
                }
            }
        }
        printf("\n");
    }
}

// Toggle an input value
void toggle_input(int input_index) {
    if (input_index < 0 || input_index >= current_circuit.input_count) {
        display_error("Invalid input index.");
        return;
    }
    
    int gate_id = current_circuit.input_gates[input_index];
    for (int i = 0; i < current_circuit.gate_count; i++) {
        if (current_circuit.gates[i].id == gate_id) {
            current_circuit.gates[i].output = !current_circuit.gates[i].output;
            printf("Toggled input %d to %d\n", input_index + 1, current_circuit.gates[i].output);
            save_action("Toggle input");
            return;
        }
    }
    
    display_error("Input gate not found.");
}

// Display error message
void display_error(const char* message) {
    printf("ERROR: %s\n", message);
}

// Detect logic loops in the circuit (basic implementation)
int detect_loops() {
    // This is a simplified implementation
    // A full implementation would use graph cycle detection algorithms
    
    for (int i = 0; i < current_circuit.wire_count; i++) {
        Wire wire = current_circuit.wires[i];
        
        // Check if a gate connects to itself
        if (wire.from_gate == wire.to_gate) {
            display_error("Logic loop detected: gate connected to itself.");
            return 1;
        }
        
        // Check for circular dependencies (simplified)
        for (int j = 0; j < current_circuit.wire_count; j++) {
            if (i != j && current_circuit.wires[j].from_gate == wire.to_gate && 
                current_circuit.wires[j].to_gate == wire.from_gate) {
                display_error("Logic loop detected: circular connection between gates.");
                return 1;
            }
        }
    }
    
    return 0;
}

// Save current state to undo stack
void save_action(const char* description) {
    Action* new_action = (Action*)malloc(sizeof(Action));
    new_action->circuit = current_circuit;
    strcpy(new_action->description, description);
    new_action->next = NULL;
    new_action->prev = NULL;
    
    if (undo_stack) {
        undo_stack->next = new_action;
        new_action->prev = undo_stack;
    }
    undo_stack = new_action;
    
    // Clear redo stack when new action is performed
    while (redo_stack) {
        Action* temp = redo_stack;
        redo_stack = redo_stack->next;
        free(temp);
    }
}

// Undo last action
void undo() {
    if (!undo_stack) {
        display_error("Nothing to undo.");
        return;
    }
    
    // Save current state to redo stack
    Action* redo_action = (Action*)malloc(sizeof(Action));
    redo_action->circuit = current_circuit;
    strcpy(redo_action->description, "Redo point");
    redo_action->next = redo_stack;
    redo_action->prev = NULL;
    if (redo_stack) redo_stack->prev = redo_action;
    redo_stack = redo_action;
    
    // Restore previous state
    current_circuit = undo_stack->circuit;
    Action* temp = undo_stack;
    undo_stack = undo_stack->prev;
    if (undo_stack) undo_stack->next = NULL;
    free(temp);
    
    printf("Undo completed: %s\n", redo_action->description);
}

// Redo last undone action
void redo() {
    if (!redo_stack) {
        display_error("Nothing to redo.");
        return;
    }
    
    // Save current state to undo stack
    Action* undo_action = (Action*)malloc(sizeof(Action));
    undo_action->circuit = current_circuit;
    strcpy(undo_action->description, "Undo point");
    undo_action->next = NULL;
    undo_action->prev = undo_stack;
    if (undo_stack) undo_stack->next = undo_action;
    undo_stack = undo_action;
    
    // Restore next state
    current_circuit = redo_stack->circuit;
    Action* temp = redo_stack;
    redo_stack = redo_stack->next;
    if (redo_stack) redo_stack->prev = NULL;
    free(temp);
    
    printf("Redo completed.\n");
}

// Save circuit to file
void save_circuit(const char* filename) {
    FILE* file = fopen(filename, "wb");
    if (!file) {
        display_error("Cannot open file for writing.");
        return;
    }
    
    fwrite(&current_circuit, sizeof(Circuit), 1, file);
    fclose(file);
    printf("Circuit saved to %s\n", filename);
}

// Load circuit from file
void load_circuit(const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        display_error("Cannot open file for reading.");
        return;
    }
    
    fread(&current_circuit, sizeof(Circuit), 1, file);
    fclose(file);
    
    // Update next IDs
    next_gate_id = 0;
    next_wire_id = 0;
    for (int i = 0; i < current_circuit.gate_count; i++) {
        if (current_circuit.gates[i].id >= next_gate_id) {
            next_gate_id = current_circuit.gates[i].id + 1;
        }
    }
    for (int i = 0; i < current_circuit.wire_count; i++) {
        if (current_circuit.wires[i].id >= next_wire_id) {
            next_wire_id = current_circuit.wires[i].id + 1;
        }
    }
    
    printf("Circuit loaded from %s\n", filename);
}

// Main function
int main() {
    int choice;
    initialize_circuit();
    
    printf("Digital Logic Circuit Simulator\n");
    printf("===============================\n");
    
    // Add some default gates for demonstration
    add_gate(GATE_INPUT, 10, 5);
    add_gate(GATE_INPUT, 10, 10);
    add_gate(GATE_AND, 30, 7);
    add_gate(GATE_OUTPUT, 50, 7);
    
    add_wire(1, 3, 1);
    add_wire(2, 3, 2);
    add_wire(3, 4, 1);
    
    do {
        display_menu();
        scanf("%d", &choice);
        
        switch (choice) {
            case 1: {
                // Add gate
                int type, x, y;
                printf("Gate types: 0=NOT, 1=AND, 2=OR, 3=XOR, 4=NAND, 5=NOR, 6=INPUT, 7=OUTPUT\n");
                printf("Enter gate type: ");
                scanf("%d", &type);
                printf("Enter X position: ");
                scanf("%d", &x);
                printf("Enter Y position: ");
                scanf("%d", &y);
                
                if (type >= 0 && type <= 7) {
                    add_gate((GateType)type, x, y);
                } else {
                    display_error("Invalid gate type.");
                }
                break;
            }
            
            case 2: {
                // Add wire
                int from_gate, to_gate, to_pin;
                printf("Enter source gate ID: ");
                scanf("%d", &from_gate);
                printf("Enter destination gate ID: ");
                scanf("%d", &to_gate);
                printf("Enter destination pin (1 or 2): ");
                scanf("%d", &to_pin);
                
                if (to_pin == 1 || to_pin == 2) {
                    add_wire(from_gate, to_gate, to_pin);
                } else {
                    display_error("Invalid pin number. Use 1 or 2.");
                }
                break;
            }
            
            case 3: {
                // Delete gate
                int gate_id;
                printf("Enter gate ID to delete: ");
                scanf("%d", &gate_id);
                delete_gate(gate_id);
                break;
            }
            
            case 4: {
                // Delete wire
                int wire_id;
                printf("Enter wire ID to delete: ");
                scanf("%d", &wire_id);
                delete_wire(wire_id);
                break;
            }
            
            case 5: {
                // Toggle input
                int input_index;
                printf("Enter input index to toggle (1-%d): ", current_circuit.input_count);
                scanf("%d", &input_index);
                toggle_input(input_index - 1);
                break;
            }
            
            case 6:
                // Evaluate circuit
                evaluate_circuit();
                break;
                
            case 7:
                // Generate truth table
                generate_truth_table();
                break;
                
            case 8:
                // Display workspace
                display_workspace();
                break;
                
            case 9:
                // Undo
                undo();
                break;
                
            case 10:
                // Redo
                redo();
                break;
                
            case 11: {
                // Save circuit
                char filename[100];
                printf("Enter filename to save: ");
                scanf("%s", filename);
                save_circuit(filename);
                break;
            }
                
            case 12: {
                // Load circuit
                char filename[100];
                printf("Enter filename to load: ");
                scanf("%s", filename);
                load_circuit(filename);
                break;
            }
                
            case 13:
                // Clear workspace
                initialize_circuit();
                printf("Workspace cleared.\n");
                break;
                
            case 0:
                printf("Exiting...\n");
                break;
                
            default:
                display_error("Invalid option.");
        }
        
        if (choice != 0) {
            printf("\nPress Enter to continue...");
            getchar();  // Clear buffer
            getchar();  // Wait for Enter
        }
        
    } while (choice != 0);
    
    return 0;
}