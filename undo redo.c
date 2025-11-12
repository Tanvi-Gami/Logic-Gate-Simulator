// Button definitions
SDL_FRect run_button = {WINDOW_WIDTH - 450, WINDOW_HEIGHT - 60, 130, 45};
SDL_FRect undo_button = {WINDOW_WIDTH - 310, WINDOW_HEIGHT - 60, 130, 45};
SDL_FRect redo_button = {WINDOW_WIDTH - 170, WINDOW_HEIGHT - 60, 130, 45};
bool circuit_running = false;

// In event handling (SDL_EVENT_MOUSE_BUTTON_DOWN):
if (mouse_x >= run_button.x && mouse_x <= run_button.x + run_button.w &&
    mouse_y >= run_button.y && mouse_y <= run_button.y + run_button.h) {
    circuit_running = !circuit_running;
    printf(circuit_running ? "Circuit started\n" : "Circuit stopped\n");
    continue;
}

if (mouse_x >= undo_button.x && mouse_x <= undo_button.x + undo_button.w &&
    mouse_y >= undo_button.y && mouse_y <= undo_button.y + undo_button.h) {
    if (history.current_index > 0) {
        history.current_index--;
        restore_state(&history, gates, &gate_count, wires, &wire_count, &next_gate_id);
        printf("Undo: state %d\n", history.current_index);
    }
    continue;
}

if (mouse_x >= redo_button.x && mouse_x <= redo_button.x + redo_button.w &&
    mouse_y >= redo_button.y && mouse_y <= redo_button.y + redo_button.h) {
    if (history.current_index < history.num_states - 1) {
        history.current_index++;
        restore_state(&history, gates, &gate_count, wires, &wire_count, &next_gate_id);
        printf("Redo: state %d\n", history.current_index);
    }
    continue;
}

// In event handling (SDL_EVENT_KEY_DOWN):
if (event.key.key == SDLK_Z && (event.key.mod & SDL_KMOD_CTRL)) {
    if (history.current_index > 0) {
        history.current_index--;
        restore_state(&history, gates, &gate_count, wires, &wire_count, &next_gate_id);
        printf("Undo (Ctrl+Z): state %d\n", history.current_index);
    }
} else if (event.key.key == SDLK_Y && (event.key.mod & SDL_KMOD_CTRL)) {
    if (history.current_index < history.num_states - 1) {
        history.current_index++;
        restore_state(&history, gates, &gate_count, wires, &wire_count, &next_gate_id);
        printf("Redo (Ctrl+Y): state %d\n", history.current_index);
    }
}

// In rendering section:
SDL_Color run_button_color = circuit_running ? (SDL_Color){220, 100, 80, 255} : (SDL_Color){100, 200, 100, 255};
SDL_SetRenderDrawColor(renderer, run_button_color.r, run_button_color.g, run_button_color.b, 255);
SDL_RenderFillRect(renderer, &run_button);
SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
SDL_RenderRect(renderer, &run_button);
const char* run_text = circuit_running ? "STOP" : "RUN";
draw_text(renderer, run_text, run_button.x + 35, run_button.y + 17, button_text_color);

bool can_undo = history.current_index > 0;
SDL_Color undo_color = can_undo ? (SDL_Color){180, 140, 70, 255} : (SDL_Color){120, 120, 120, 255};
SDL_SetRenderDrawColor(renderer, undo_color.r, undo_color.g, undo_color.b, 255);
SDL_RenderFillRect(renderer, &undo_button);
SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
SDL_RenderRect(renderer, &undo_button);
draw_text(renderer, "UNDO", undo_button.x + 35, undo_button.y + 17, button_text_color);

bool can_redo = history.current_index < history.num_states - 1;
SDL_Color redo_color = can_redo ? (SDL_Color){180, 140, 70, 255} : (SDL_Color){120, 120, 120, 255};
SDL_SetRenderDrawColor(renderer, redo_color.r, redo_color.g, redo_color.b, 255);
SDL_RenderFillRect(renderer, &redo_button);
SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
SDL_RenderRect(renderer, &redo_button);
draw_text(renderer, "REDO", redo_button.x + 35, redo_button.y + 17, button_text_color);

// In main loop (before rendering):
if (circuit_running) {
    propagate_signals(gates, gate_count, wires, wire_count);
}
