#ifndef LOGICGATES_H
#define LOGICGATES_H

// Gate function declarations
int AND(int a, int b);
int OR(int a, int b);
int NOT(int a);
int NAND(int a, int b);
int NOR(int a, int b);
int XOR(int a, int b);

// Input validation function
int is_binary(int x);

// Gate name utility function
const char* get_gate_name(int gate_type);

// User interface functions
void display_menu();
int get_binary_input(const char* prompt);

// Multi-gate simulation function
void multi_choice_logic_gate_simulation(int number_of_gates, int choice[number_of_gates]);

#endif