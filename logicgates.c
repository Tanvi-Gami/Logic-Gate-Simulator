#include <stdio.h>
#include "logicgates.h"

// Gate functions 
int AND(int a, int b) {
    return (a && b);
}

int OR(int a, int b) {
    return (a || b);
}

int NOT(int a) {
    return !a;
} 

int NAND(int a, int b) {
    return !(a && b);
}

int NOR(int a, int b) {
    return !(a || b);
}

int XOR(int a, int b) {
    return (a && !b) || (!a && b);  // Logical XOR
}

// Function to validate binary input
int is_binary(int x) {
    return (x == 0 || x == 1);
}

// Function to get gate name
const char* get_gate_name(int gate_type) {
    switch(gate_type) {
        case 1: return "AND";
        case 2: return "OR";
        case 3: return "NOT";
        case 4: return "NAND";
        case 5: return "NOR";
        case 6: return "XOR";
        case 7: return "EXIT";
        default: return "UNKNOWN";
    }
}

// Display menu
void display_menu() {
    printf("\n--- Digital Logic Gate Simulator ---\n");
    printf("1. AND Gate\n");
    printf("2. OR Gate\n");
    printf("3. NOT Gate\n");
    printf("4. NAND Gate\n");
    printf("5. NOR Gate\n");
    printf("6. XOR Gate\n");
    printf("7. Exit\n");
    printf("Choose an option (1-7): ");
}

// Function to get binary input with validation
int get_binary_input(const char* prompt) {
    int input;
    do {
        printf("%s", prompt);
        scanf("%d", &input);
        if (!is_binary(input)) {
            printf("Error: Please enter only 0 or 1!\n");
        }
    } while (!is_binary(input));
    return input;
}

// Function for multi-choice logic gate simulation
void multi_choice_logic_gate_simulation(int number_of_gates, int choice[number_of_gates]) {
    int a, b, result;
    
    // Get initial inputs with validation
    printf("\n=== Initial Inputs ===\n");
    a = get_binary_input("Enter first input (0 or 1): ");
    b = get_binary_input("Enter second input (0 or 1): ");
    
    for(int i = 0; i < number_of_gates; i++) {
        printf("\n--- Gate %d (%s) ---\n", i + 1, get_gate_name(choice[i]));
        
        switch(choice[i]) {
            case 1:
                result = AND(a, b);
                printf("AND(%d, %d) = %d\n", a, b, result);
                break;
            case 2:
                result = OR(a, b);
                printf("OR(%d, %d) = %d\n", a, b, result);
                break;
            case 3:
                result = NOT(a);
                printf("NOT(%d) = %d\n", a, result);
                break;
            case 4:
                result = NAND(a, b);
                printf("NAND(%d, %d) = %d\n", a, b, result);
                break;
            case 5:
                result = NOR(a, b);
                printf("NOR(%d, %d) = %d\n", a, b, result);
                break;
            case 6:
                result = XOR(a, b);
                printf("XOR(%d, %d) = %d\n", a, b, result);
                break;
            case 7:
                printf("Goodbye!\n");
                return;
            default:
                printf("Invalid choice! Please try again.\n");
                i--; // Retry this gate
                continue;
        }
        
        // Prepare for next gate
        if (i < number_of_gates - 1) {
            int next_gate = choice[i + 1];
            
            if (next_gate == 3) { // Next gate is NOT - only needs one input
                printf("Output %d automatically passed as input to next gate (NOT)\n", result);
                a = result;
                // 'b' value doesn't matter for NOT gate, but we'll keep it for consistency
            } else {
                // For 2-input gates
                printf("\n--- Preparing for next gate (%s) ---\n", get_gate_name(next_gate));
                a = get_binary_input("Enter new input (0 or 1): ");
                b = result;
                printf("Using previous result %d as second input\n", result);
            }
        }
    }
    
    printf("\n=== Simulation Complete ===\n");
    printf("Final output: %d\n", result);
}

// // Main function
// int main() {
//     int choice[100];
//     int num_gates;

//     printf("=== Digital Logic Gate Simulator ===\n");
    
//     // Get number of gates
//     do {
//         printf("Enter the number of gates you want to simulate (1-100): ");
//         scanf("%d", &num_gates);
//         if (num_gates <= 0 || num_gates > 100) {
//             printf("Invalid number! Please enter between 1 and 100.\n");
//         }
//     } while (num_gates <= 0 || num_gates > 100);
    
//     // Get choices for all gates
//     printf("\n=== Gate Selection ===\n");
//     for(int i = 0; i < num_gates; i++) {
//         printf("\nGate %d:\n", i + 1);
//         display_menu();
//         scanf("%d", &choice[i]);
        
//         // Validate gate choice
//         while (choice[i] < 1 || choice[i] > 7) {
//             printf("Invalid choice! Please enter 1-7: ");
//             scanf("%d", &choice[i]);
//         }
        
//         // Exit option
//         if (choice[i] == 7) {           
//             printf("Goodbye!\n");
//             return 0;
//         }
//     }
    
//     // Display gate sequence
//     printf("\n=== Gate Sequence ===\n");
//     for(int i = 0; i < num_gates; i++) {
//         printf("Gate %d: %s\n", i + 1, get_gate_name(choice[i]));
//     }
    
//     // Run the simulation
//     multi_choice_logic_gate_simulation(num_gates, choice);
    
//     return 0;
// }