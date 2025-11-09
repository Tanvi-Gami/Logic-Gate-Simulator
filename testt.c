// Digital Logic Circuit Simulator (SDL3, fullscreen, ASCII-only)
// Build: gcc testt.c -o sim $(pkg-config --cflags --libs sdl3) -lm

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

/* Limits and layout */
#define MAX_COMPONENTS 100
#define MAX_WIRES 200
#define MAX_UNDO_STACK 50
#define COMPONENT_SIZE 60
#define GRID_SIZE 20
#define WIRE_CLICK_TOLERANCE 5

/* Component types */
typedef enum {
    COMP_NONE = 0,
    COMP_AND,
    COMP_OR,
    COMP_NOT,
    COMP_NAND,
    COMP_NOR,
    COMP_XOR,
    COMP_INPUT_TOGGLE,
    COMP_OUTPUT_LED
} ComponentType;

/* Tool modes */
typedef enum {
    TOOL_SELECT = 0,
    TOOL_ADD_GATE,
    TOOL_WIRE,
    TOOL_DELETE
} ToolMode;

/* Connection point */
typedef struct {
    int component_id;
    int pin_index; /* 0,1 for inputs; -1 for output */
} ConnectionPoint;

/* Wire */
typedef struct {
    int id;
    ConnectionPoint start;
    ConnectionPoint end;
    bool is_valid;
    int value; /* 0, 1, or -1 (undef) */
} Wire;

/* Component */
typedef struct {
    int id;
    ComponentType type;
    float x, y;
    bool selected;

    int output_value;    /* evaluated signal at output */
    bool input_state;    /* for input toggle */

    /* bookkeeping; simple fan-out list for outputs if needed later */
    int output_wire_ids[16];
    int output_count;
    char label[32];
    int inputs[2]; /* Wire IDs connected to inputs */
} Component;

/* Undo/Redo */
typedef enum {
    ACTION_ADD_COMPONENT = 0,
    ACTION_DELETE_COMPONENT,
    ACTION_ADD_WIRE,
    ACTION_DELETE_WIRE,
    ACTION_MOVE_COMPONENT
} ActionType;

typedef struct {
    ActionType type;
    Component component;
    Wire wire;
    float old_x, old_y;
    float new_x, new_y;
} UndoAction;

/* App state */
typedef struct {
    SDL_Window* window;
    SDL_Renderer* renderer;
    int screen_w;
    int screen_h;

    Component components[MAX_COMPONENTS];
    int component_count;
    int next_component_id;

    Wire wires[MAX_WIRES];
    int wire_count;
    int next_wire_id;

    ToolMode current_tool;
    ComponentType selected_gate_type;

    int dragging_component_id;
    float drag_dx, drag_dy;

    bool wiring_in_progress;
    ConnectionPoint wire_start;
    float wire_temp_x, wire_temp_y;

    UndoAction undo_stack[MAX_UNDO_STACK];
    int undo_count;
    UndoAction redo_stack[MAX_UNDO_STACK];
    int redo_count;

    bool running;
    bool simulation_running;
    char error_message[256];
    int error_timer;
} AppState;

/* Forward declarations */
static void app_init(AppState* app);
static void app_cleanup(AppState* app);
static void cleanup_app(AppState* app);
static void app_events(AppState* app);
static void app_update(AppState* app);
static void app_render(AppState* app);

/* Drawing helpers */
static void draw_filled_rect(SDL_Renderer* r, float x, float y, float w, float h);
static void draw_rect(SDL_Renderer* r, float x, float y, float w, float h);
static void draw_line(SDL_Renderer* r, float x1, float y1, float x2, float y2);
static void draw_circle(SDL_Renderer* r, float cx, float cy, float radius);
static void draw_filled_circle(SDL_Renderer* r, float cx, float cy, float radius);

/* Circuit helpers */
static int add_component(AppState* app, ComponentType type, float x, float y);
static void delete_component(AppState* app, int id);
static int add_wire(AppState* app, ConnectionPoint s, ConnectionPoint e);
static void delete_wire(AppState* app, int id);

static Component* get_component_by_id(AppState* app, int id);
static Wire* get_wire_by_id(AppState* app, int id);
static Component* hit_component(AppState* app, float x, float y);
static Wire* hit_wire(AppState* app, float x, float y);

/* Undo/Redo */
static void push_undo(AppState* app, const UndoAction* a);
static void undo(AppState* app);
static void redo(AppState* app);

/* Simulation */
static int eval_gate(ComponentType t, int in1, int in2);
static void simulate(AppState* app);

/* Implementation */
static int eval_gate(ComponentType t, int in1, int in2) {
    if (t == COMP_NOT) {
        if (in1 == -1) return -1;
        return (!in1) ? 1 : 0;
    }
    if (in1 == -1 || in2 == -1) return -1;
    switch (t) {
        case COMP_AND:  return (in1 && in2) ? 1 : 0;
        case COMP_OR:   return (in1 || in2) ? 1 : 0;
        case COMP_NAND: return (!(in1 && in2)) ? 1 : 0;
        case COMP_NOR:  return (!(in1 || in2)) ? 1 : 0;
        case COMP_XOR:  return (in1 ^ in2) ? 1 : 0;
        default:        return -1;
    }
}

static void label_for(ComponentType t, char* dst, size_t n) {
    const char* s = "?";
    switch (t) {
        case COMP_AND: s = "AND"; break;
        case COMP_OR: s = "OR"; break;
        case COMP_NOT: s = "NOT"; break;
        case COMP_NAND: s = "NAND"; break;
        case COMP_NOR: s = "NOR"; break;
        case COMP_XOR: s = "XOR"; break;
        case COMP_INPUT_TOGGLE: s = "IN"; break;
        case COMP_OUTPUT_LED: s = "LED"; break;
        default: s = "?"; break;
    }
    SDL_strlcpy(dst, s, n);
}

static void set_error(AppState* app, const char* msg) {
    SDL_strlcpy(app->error_message, msg, sizeof(app->error_message));
    app->error_timer = 180;
}

static void app_init(AppState* app) {
    SDL_zero(*app);

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        SDL_Log("SDL_Init failed: %s\n", SDL_GetError());
        exit(1);
    }

    SDL_DisplayID display = SDL_GetPrimaryDisplay();
    if (display == 0) {
        SDL_Log("No display: %s\n", SDL_GetError());
        exit(1);
    }

    const SDL_DisplayMode* desk = SDL_GetDesktopDisplayMode(display);
    int w = 1280, h = 720;
    if (desk) {
        w = desk->w;
        h = desk->h;
    }

    app->window = SDL_CreateWindow("Digital Logic Circuit Simulator", w, h, 0);
    if (!app->window) {
        SDL_Log("CreateWindow failed: %s\n", SDL_GetError());
        exit(1);
    }

    /* Request desktop fullscreen after creation */
    if (SDL_SetWindowFullscreen(app->window, true) != 0) {
        SDL_Log("Fullscreen request failed (continuing windowed): %s\n", SDL_GetError());
    }

    /* Query final size */
    int cw = 0, ch = 0;
    if (SDL_GetWindowSize(app->window, &cw, &ch) != 0) {
        cw = w;
        ch = h;
    }
    app->screen_w = cw;
    app->screen_h = ch;

    app->renderer = SDL_CreateRenderer(app->window, NULL);
    if (!app->renderer) {
        SDL_Log("CreateRenderer failed: %s\n", SDL_GetError());
        exit(1);
    }

    app->current_tool = TOOL_SELECT;
    app->selected_gate_type = COMP_AND;
    app->simulation_running = true;
    app->running = true;
}

static void app_cleanup(AppState* app) {
    if (app->renderer) SDL_DestroyRenderer(app->renderer);
    if (app->window) SDL_DestroyWindow(app->window);
    SDL_Quit();
}

static void cleanup_app(AppState* app) {
    app_cleanup(app);
}

static Component* get_component_by_id(AppState* app, int id) {
    for (int i = 0; i < app->component_count; i++) {
        if (app->components[i].id == id) return &app->components[i];
    }
    return NULL;
}

static Wire* get_wire_by_id(AppState* app, int id) {
    for (int i = 0; i < app->wire_count; i++) {
        if (app->wires[i].id == id) return &app->wires[i];
    }
    return NULL;
}

static Component* hit_component(AppState* app, float x, float y) {
    for (int i = app->component_count - 1; i >= 0; i--) {
        Component* c = &app->components[i];
        if (x >= c->x && x <= c->x + COMPONENT_SIZE &&
            y >= c->y && y <= c->y + COMPONENT_SIZE) {
            return c;
        }
    }
    return NULL;
}

static Wire* hit_wire(AppState* app, float x, float y) {
    for (int i = 0; i < app->wire_count; i++) {
        Wire* w = &app->wires[i];
        Component* sc = get_component_by_id(app, w->start.component_id);
        Component* ec = get_component_by_id(app, w->end.component_id);
        if (!sc || !ec) continue;

        float x1 = sc->x + COMPONENT_SIZE;
        float y1 = sc->y + COMPONENT_SIZE * 0.5f;
        float x2 = ec->x;
        float y2 = ec->y + COMPONENT_SIZE * 0.5f;

        float A = x - x1, B = y - y1;
        float C = x2 - x1, D = y2 - y1;
        float dot = A * C + B * D;
        float len2 = C * C + D * D;
        float t = (len2 > 0.0f) ? (dot / len2) : -1.0f;

        float px, py;
        if (t < 0.0f) { px = x1; py = y1; }
        else if (t > 1.0f) { px = x2; py = y2; }
        else { px = x1 + t * C; py = y1 + t * D; }

        float dx = x - px, dy = y - py;
        float dist = sqrtf(dx * dx + dy * dy);
        if (dist <= WIRE_CLICK_TOLERANCE) return w;
    }
    return NULL;
}

static int add_component(AppState* app, ComponentType type, float x, float y) {
    if (app->component_count >= MAX_COMPONENTS) {
        set_error(app, "Component limit reached");
        return -1;
    }
    Component* c = &app->components[app->component_count++];
    SDL_zero(*c);
    c->id = app->next_component_id++;
    c->type = type;
    c->x = x;
    c->y = y;
    c->output_value = -1;
    c->input_state = false;
    c->inputs[0] = -1;
    c->inputs[1] = -1;
    label_for(type, c->label, sizeof(c->label));

    UndoAction a;
    SDL_zero(a);
    a.type = ACTION_ADD_COMPONENT;
    a.component = *c;
    push_undo(app, &a);
    return c->id;
}

static void delete_component(AppState* app, int id) {
    for (int i = 0; i < app->component_count; i++) {
        if (app->components[i].id == id) {
            UndoAction a;
            SDL_zero(a);
            a.type = ACTION_DELETE_COMPONENT;
            a.component = app->components[i];
            push_undo(app, &a);

            /* remove wires attached */
            for (int w = app->wire_count - 1; w >= 0; w--) {
                if (app->wires[w].start.component_id == id ||
                    app->wires[w].end.component_id == id) {
                    for (int k = w; k < app->wire_count - 1; k++) {
                        app->wires[k] = app->wires[k + 1];
                    }
                    app->wire_count--;
                }
            }
            for (int j = i; j < app->component_count - 1; j++) {
                app->components[j] = app->components[j + 1];
            }
            app->component_count--;
            return;
        }
    }
}

static int add_wire(AppState* app, ConnectionPoint s, ConnectionPoint e) {
    if (app->wire_count >= MAX_WIRES) {
        set_error(app, "Wire limit reached");
        return -1;
    }
    Component* sc = get_component_by_id(app, s.component_id);
    Component* ec = get_component_by_id(app, e.component_id);
    if (!sc || !ec) return -1;
    if (s.pin_index < 0 && e.pin_index < 0) {
        set_error(app, "Cannot connect output to output");
        return -1;
    }

    Wire* w = &app->wires[app->wire_count++];
    SDL_zero(*w);
    w->id = app->next_wire_id++;
    w->start = s;
    w->end = e;
    w->is_valid = true;
    w->value = -1;

    UndoAction a;
    SDL_zero(a);
    a.type = ACTION_ADD_WIRE;
    a.wire = *w;
    push_undo(app, &a);
    return w->id;
}

static void delete_wire(AppState* app, int id) {
    for (int i = 0; i < app->wire_count; i++) {
        if (app->wires[i].id == id) {
            UndoAction a;
            SDL_zero(a);
            a.type = ACTION_DELETE_WIRE;
            a.wire = app->wires[i];
            push_undo(app, &a);
            for (int j = i; j < app->wire_count - 1; j++) app->wires[j] = app->wires[j + 1];
            app->wire_count--;
            return;
        }
    }
}

static void push_undo(AppState* app, const UndoAction* a) {
    if (app->undo_count >= MAX_UNDO_STACK) {
        /* simple drop oldest by shifting */
        for (int i = 1; i < app->undo_count; i++) app->undo_stack[i - 1] = app->undo_stack[i];
        app->undo_count--;
    }
    app->undo_stack[app->undo_count++] = *a;
    app->redo_count = 0;
}

static void undo(AppState* app) {
    if (app->undo_count <= 0) return;
    UndoAction a = app->undo_stack[--app->undo_count];
    app->redo_stack[app->redo_count++] = a;

    switch (a.type) {
        case ACTION_ADD_COMPONENT: {
            int id = a.component.id;
            for (int i = 0; i < app->component_count; i++) {
                if (app->components[i].id == id) {
                    for (int j = i; j < app->component_count - 1; j++) app->components[j] = app->components[j + 1];
                    app->component_count--;
                    break;
                }
            }
        } break;
        case ACTION_DELETE_COMPONENT: {
            if (app->component_count < MAX_COMPONENTS) app->components[app->component_count++] = a.component;
        } break;
        case ACTION_ADD_WIRE: {
            int id = a.wire.id;
            for (int i = 0; i < app->wire_count; i++) {
                if (app->wires[i].id == id) {
                    for (int j = i; j < app->wire_count - 1; j++) app->wires[j] = app->wires[j + 1];
                    app->wire_count--;
                    break;
                }
            }
        } break;
        case ACTION_DELETE_WIRE: {
            if (app->wire_count < MAX_WIRES) app->wires[app->wire_count++] = a.wire;
        } break;
        case ACTION_MOVE_COMPONENT: {
            Component* c = get_component_by_id(app, a.component.id);
            if (c) { c->x = a.old_x; c->y = a.old_y; }
        } break;
        default: break;
    }
}

static void redo(AppState* app) {
    if (app->redo_count <= 0) return;
    UndoAction a = app->redo_stack[--app->redo_count];

    switch (a.type) {
        case ACTION_ADD_COMPONENT: {
            if (app->component_count < MAX_COMPONENTS) app->components[app->component_count++] = a.component;
        } break;
        case ACTION_DELETE_COMPONENT: {
            int id = a.component.id;
            for (int i = 0; i < app->component_count; i++) {
                if (app->components[i].id == id) {
                    for (int j = i; j < app->component_count - 1; j++) app->components[j] = app->components[j + 1];
                    app->component_count--;
                    break;
                }
            }
        } break;
        case ACTION_ADD_WIRE: {
            if (app->wire_count < MAX_WIRES) app->wires[app->wire_count++] = a.wire;
        } break;
        case ACTION_DELETE_WIRE: {
            int id = a.wire.id;
            for (int i = 0; i < app->wire_count; i++) {
                if (app->wires[i].id == id) {
                    for (int j = i; j < app->wire_count - 1; j++) app->wires[j] = app->wires[j + 1];
                    app->wire_count--;
                    break;
                }
            }
        } break;
        case ACTION_MOVE_COMPONENT: {
            Component* c = get_component_by_id(app, a.component.id);
            if (c) { c->x = a.new_x; c->y = a.new_y; }
        } break;
        default: break;
    }
}

static void simulate(AppState* app) {
    for (int i = 0; i < app->component_count; i++) app->components[i].output_value = -1;

    for (int i = 0; i < app->component_count; i++) {
        Component* c = &app->components[i];
        if (c->type == COMP_INPUT_TOGGLE) c->output_value = c->input_state ? 1 : 0;
    }

    for (int iter = 0; iter < 16; iter++) {
        bool changed = false;
        for (int i = 0; i < app->component_count; i++) {
            Component* c = &app->components[i];
            if (c->type == COMP_INPUT_TOGGLE) continue;

            int in0 = -1, in1 = -1;
            for (int w = 0; w < app->wire_count; w++) {
                Wire* wire = &app->wires[w];
                if (wire->end.component_id == c->id) {
                    Component* src = get_component_by_id(app, wire->start.component_id);
                    if (!src) continue;
                    if (in0 == -1) in0 = src->output_value;
                    else in1 = src->output_value;
                }
            }
            int old = c->output_value;
            if (c->type == COMP_OUTPUT_LED) c->output_value = in0;
            else c->output_value = eval_gate(c->type, in0, in1);
            if (old != c->output_value) changed = true;
        }
        if (!changed) break;
    }

    for (int w = 0; w < app->wire_count; w++) {
        Wire* wire = &app->wires[w];
        Component* src = get_component_by_id(app, wire->start.component_id);
        wire->value = src ? src->output_value : -1;
    }
}

/* Input handling */
static void app_events(AppState* app) {
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        if (ev.type == SDL_EVENT_QUIT) app->running = false;

        else if (ev.type == SDL_EVENT_KEY_DOWN) {
            SDL_Keycode k = ev.key.key;
            bool ctrl = (SDL_GetModState() & SDL_KMOD_CTRL) != 0;

            if (k == SDLK_ESCAPE) app->running = false;
            else if (ctrl && k == SDLK_Z) undo(app);
            else if (ctrl && k == SDLK_Y) redo(app);
            else if (k == SDLK_S) app->current_tool = TOOL_SELECT;
            else if (k == SDLK_W) app->current_tool = TOOL_WIRE;
            else if (k == SDLK_D) app->current_tool = TOOL_DELETE;
            else if (k == SDLK_1) { app->current_tool = TOOL_ADD_GATE; app->selected_gate_type = COMP_AND; }
            else if (k == SDLK_2) { app->current_tool = TOOL_ADD_GATE; app->selected_gate_type = COMP_OR; }
            else if (k == SDLK_3) { app->current_tool = TOOL_ADD_GATE; app->selected_gate_type = COMP_NOT; }
            else if (k == SDLK_4) { app->current_tool = TOOL_ADD_GATE; app->selected_gate_type = COMP_NAND; }
            else if (k == SDLK_5) { app->current_tool = TOOL_ADD_GATE; app->selected_gate_type = COMP_NOR; }
            else if (k == SDLK_6) { app->current_tool = TOOL_ADD_GATE; app->selected_gate_type = COMP_XOR; }
            else if (k == SDLK_7) { app->current_tool = TOOL_ADD_GATE; app->selected_gate_type = COMP_INPUT_TOGGLE; }
            else if (k == SDLK_8) { app->current_tool = TOOL_ADD_GATE; app->selected_gate_type = COMP_OUTPUT_LED; }
        }

        else if (ev.type == SDL_EVENT_MOUSE_BUTTON_DOWN && ev.button.button == SDL_BUTTON_LEFT) {
            float mx = (float)ev.button.x;
            float my = (float)ev.button.y;

            if (app->current_tool == TOOL_ADD_GATE) {
                add_component(app, app->selected_gate_type, mx - COMPONENT_SIZE * 0.5f, my - COMPONENT_SIZE * 0.5f);
            } else if (app->current_tool == TOOL_SELECT) {
                Component* c = hit_component(app, mx, my);
                if (c) {
                    if (c->type == COMP_INPUT_TOGGLE) c->input_state = !c->input_state;
                    else {
                        app->dragging_component_id = c->id;
                        app->drag_dx = mx - c->x;
                        app->drag_dy = my - c->y;
                    }
                }
            } else if (app->current_tool == TOOL_WIRE) {
                Component* c = hit_component(app, mx, my);
                if (c) {
                    if (!app->wiring_in_progress) {
                        app->wiring_in_progress = true;
                        app->wire_start.component_id = c->id;
                        app->wire_start.pin_index = -1; /* from output */
                    } else {
                        ConnectionPoint endp;
                        endp.component_id = c->id;
                        endp.pin_index = 0; /* to input 0 (simplified) */
                        add_wire(app, app->wire_start, endp);
                        app->wiring_in_progress = false;
                    }
                } else if (app->wiring_in_progress) {
                    app->wiring_in_progress = false;
                }
            } else if (app->current_tool == TOOL_DELETE) {
                Component* c = hit_component(app, mx, my);
                if (c) delete_component(app, c->id);
                else {
                    Wire* w = hit_wire(app, mx, my);
                    if (w) delete_wire(app, w->id);
                }
            }
        }

        else if (ev.type == SDL_EVENT_MOUSE_BUTTON_UP && ev.button.button == SDL_BUTTON_LEFT) {
            if (app->dragging_component_id != -1) {
                Component* c = get_component_by_id(app, app->dragging_component_id);
                if (c) {
                    UndoAction a;
                    SDL_zero(a);
                    a.type = ACTION_MOVE_COMPONENT;
                    a.component = *c;
                    a.old_x = c->x; a.old_y = c->y; /* best-effort: ideally store old at down */
                    a.new_x = c->x; a.new_y = c->y;
                    push_undo(app, &a);
                }
            }
            app->dragging_component_id = -1;
        }

        else if (ev.type == SDL_EVENT_MOUSE_MOTION) {
            float mx = (float)ev.motion.x;
            float my = (float)ev.motion.y;

            if (app->dragging_component_id != -1) {
                Component* c = get_component_by_id(app, app->dragging_component_id);
                if (c) {
                    c->x = mx - app->drag_dx;
                    c->y = my - app->drag_dy;
                }
            }
            if (app->wiring_in_progress) {
                app->wire_temp_x = mx;
                app->wire_temp_y = my;
            }
        }
    }
}

static void app_update(AppState* app) {
    if (app->simulation_running) simulate(app);
    if (app->error_timer > 0) app->error_timer--;
}

/* Drawing primitives */
static void draw_filled_rect(SDL_Renderer* r, float x, float y, float w, float h) {
    SDL_FRect fr = { x, y, w, h };
    SDL_RenderFillRect(r, &fr);
}
static void draw_rect(SDL_Renderer* r, float x, float y, float w, float h) {
    SDL_FRect fr = { x, y, w, h };
    SDL_RenderRect(r, &fr);
}
static void draw_line(SDL_Renderer* r, float x1, float y1, float x2, float y2) {
    SDL_RenderLine(r, x1, y1, x2, y2);
}
static void draw_circle(SDL_Renderer* r, float cx, float cy, float radius) {
    const int step = 8;
    for (int a = 0; a < 360; a += step) {
        float r1 = (float)a * (float)M_PI / 180.0f;
        float r2 = (float)(a + step) * (float)M_PI / 180.0f;
        float x1 = cx + radius * cosf(r1);
        float y1 = cy + radius * sinf(r1);
        float x2 = cx + radius * cosf(r2);
        float y2 = cy + radius * sinf(r2);
        SDL_RenderLine(r, x1, y1, x2, y2);
    }
}
static void draw_filled_circle(SDL_Renderer* r, float cx, float cy, float radius) {
    for (float rr = radius; rr > 0.0f; rr -= 1.0f) draw_circle(r, cx, cy, rr);
}

/* Rendering */
static void render_component(SDL_Renderer* rr, const Component* c) {
    SDL_SetRenderDrawColor(rr, 200, 200, 200, 255);
    draw_filled_rect(rr, c->x, c->y, COMPONENT_SIZE, COMPONENT_SIZE);
    SDL_SetRenderDrawColor(rr, 0, 0, 0, 255);
    draw_rect(rr, c->x, c->y, COMPONENT_SIZE, COMPONENT_SIZE);

    if (c->type == COMP_INPUT_TOGGLE) {
        if (c->input_state) SDL_SetRenderDrawColor(rr, 0, 200, 0, 255);
        else SDL_SetRenderDrawColor(rr, 200, 0, 0, 255);
        draw_filled_circle(rr, c->x + COMPONENT_SIZE * 0.5f, c->y + COMPONENT_SIZE * 0.5f, 14);
    } else if (c->type == COMP_OUTPUT_LED) {
        if (c->output_value == 1) SDL_SetRenderDrawColor(rr, 0, 220, 0, 255);
        else if (c->output_value == 0) SDL_SetRenderDrawColor(rr, 60, 60, 60, 255);
        else SDL_SetRenderDrawColor(rr, 220, 220, 0, 255);
        draw_filled_circle(rr, c->x + COMPONENT_SIZE * 0.5f, c->y + COMPONENT_SIZE * 0.5f, 14);
    }
}

static void render_wire(SDL_Renderer* rr, AppState* app, const Wire* w) {
    const Component* sc = get_component_by_id(app, w->start.component_id);
    const Component* ec = get_component_by_id(app, w->end.component_id);
    if (!sc || !ec) return;

    float x1 = sc->x + COMPONENT_SIZE;
    float y1 = sc->y + COMPONENT_SIZE * 0.5f;
    float x2 = ec->x;
    float y2 = ec->y + COMPONENT_SIZE * 0.5f;

    if (w->value == 1) SDL_SetRenderDrawColor(rr, 0, 220, 0, 255);
    else if (w->value == 0) SDL_SetRenderDrawColor(rr, 220, 0, 0, 255);
    else SDL_SetRenderDrawColor(rr, 130, 130, 130, 255);

    float mx = (x1 + x2) * 0.5f;
    draw_line(rr, x1, y1, mx, y1);
    draw_line(rr, mx, y1, mx, y2);
    draw_line(rr, mx, y2, x2, y2);
}

static void render_toolbar(SDL_Renderer* rr, AppState* app) {
    SDL_SetRenderDrawColor(rr, 60, 60, 60, 255);
    draw_filled_rect(rr, 0, 0, (float)app->screen_w, 60.0f);

    const int count = 12;
    for (int i = 0; i < count; i++) {
        float x = 10.0f + i * 100.0f;
        float y = 10.0f;
        SDL_SetRenderDrawColor(rr, 100, 100, 100, 255);
        draw_filled_rect(rr, x, y, 90.0f, 40.0f);
        SDL_SetRenderDrawColor(rr, 200, 200, 200, 255);
        draw_rect(rr, x, y, 90.0f, 40.0f);
    }

    if (app->error_timer > 0) {
        SDL_SetRenderDrawColor(rr, 200, 0, 0, 255);
        draw_filled_rect(rr, (float)app->screen_w * 0.5f - 200.0f, 70.0f, 400.0f, 28.0f);
    }
}

static void app_render(AppState* app) {
    SDL_Renderer* rr = app->renderer;

    SDL_SetRenderDrawColor(rr, 40, 40, 40, 255);
    SDL_RenderClear(rr);

    SDL_SetRenderDrawColor(rr, 60, 60, 60, 255);
    for (int x = 0; x < app->screen_w; x += GRID_SIZE) draw_line(rr, (float)x, 60.0f, (float)x, (float)app->screen_h);
    for (int y = 60; y < app->screen_h; y += GRID_SIZE) draw_line(rr, 0.0f, (float)y, (float)app->screen_w, (float)y);

    for (int i = 0; i < app->wire_count; i++) render_wire(rr, app, &app->wires[i]);

    if (app->wiring_in_progress) {
        Component* sc = get_component_by_id(app, app->wire_start.component_id);
        if (sc) {
            SDL_SetRenderDrawColor(rr, 255, 255, 0, 255);
            draw_line(rr, sc->x + COMPONENT_SIZE, sc->y + COMPONENT_SIZE * 0.5f, app->wire_temp_x, app->wire_temp_y);
        }
    }

    for (int i = 0; i < app->component_count; i++) render_component(rr, &app->components[i]);

    render_toolbar(rr, app);

    SDL_RenderPresent(rr);
}

int main(int argc, char** argv) {
    (void)argc; (void)argv;
    AppState app;
    app_init(&app);

    while (app.running) {
        app_events(&app);
        app_update(&app);
        app_render(&app);
        SDL_Delay(16);
    }

    cleanup_app(&app);
    return 0;
}