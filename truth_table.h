#ifndef TRUTH_TABLE_H
#define TRUTH_TABLE_H

#include "logicgates.h"

// Use void pointers for all functions
void generate_truth_table(void* gates, int gate_count, void* wires, int wire_count);
int find_input_gates(void* gates, int gate_count, int* input_gate_indices);
int find_output_gates(void* gates, int gate_count, int* output_gate_indices);
void simulate_circuit_with_inputs(void* gates, int gate_count, void* wires, int wire_count, int* input_values, int* output_values);
void draw_truth_table_window(void* gates, int gate_count, int num_inputs, int num_outputs, int* input_gate_indices, int* output_gate_indices);

#endif