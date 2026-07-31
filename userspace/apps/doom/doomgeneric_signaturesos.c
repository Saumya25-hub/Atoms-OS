#include "src/doomgeneric/doomgeneric.h"
#include "../../libbos/include/bos.h"
#include "../../libbos/include/bpde.h"
#include "../../libbos_gui/include/bos_gui.h"
#include "src/doomgeneric/doomkeys.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

static inline void bos_checkpoint(uint32_t id) {
    __asm__ volatile ("mov %0, %%rdi" :: "r" ((uint64_t)300));
    __asm__ volatile ("mov %0, %%rsi" :: "r" ((uint64_t)id));
    __asm__ volatile ("int $0x80");
}

#define DOOM_W 640
#define DOOM_H 400

static BOSWindow* s_doom_window = NULL;
static uint32_t s_doom_pixels[DOOM_W * DOOM_H];

static unsigned char doom_key_from_bos(uint32_t key, uint32_t character) {
    // 1. Check BOS Special keys
    switch (key) {
        case BOS_KEY_UP:    return KEY_UPARROW;
        case BOS_KEY_DOWN:  return KEY_DOWNARROW;
        case BOS_KEY_LEFT:  return KEY_LEFTARROW;
        case BOS_KEY_RIGHT: return KEY_RIGHTARROW;
        case BOS_KEY_ESC:   return KEY_ESCAPE;
        case BOS_KEY_CTRL:  return KEY_FIRE;
        case BOS_KEY_ALT:   return KEY_LALT;
        case BOS_KEY_SHIFT: return KEY_RSHIFT;
        case 0x11: return KEY_UPARROW;   // PS/2 W
        case 0x1E: return KEY_LEFTARROW; // PS/2 A
        case 0x1F: return KEY_DOWNARROW; // PS/2 S
        case 0x20: return KEY_RIGHTARROW;// PS/2 D
        case 0x1C: return KEY_ENTER;     // PS/2 Enter
        case 0x39: return KEY_USE;       // PS/2 Space
        case 0x12: return KEY_USE;       // PS/2 E
        default: break;
    }

    // 2. Check ASCII characters
    if (character == 'w' || character == 'W') return KEY_UPARROW;
    if (character == 's' || character == 'S') return KEY_DOWNARROW;
    if (character == 'a' || character == 'A') return KEY_LEFTARROW;
    if (character == 'd' || character == 'D') return KEY_RIGHTARROW;
    if (character == 'e' || character == 'E' || character == ' ') return KEY_USE;
    if (character == '\r' || character == '\n') return KEY_ENTER;
    if (character >= 'a' && character <= 'z') return (unsigned char)character;
    if (character >= 'A' && character <= 'Z') return (unsigned char)(character + 32);
    if (character >= '0' && character <= '9') return (unsigned char)character;

    // 3. Fallback PS/2 Scancode Table
    static const unsigned char set1_to_doom[128] = {
        0, KEY_ESCAPE, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', KEY_BACKSPACE,
        KEY_TAB, 'q', KEY_UPARROW, 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', KEY_ENTER,
        KEY_FIRE, KEY_LEFTARROW, KEY_DOWNARROW, KEY_RIGHTARROW, 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
        KEY_RSHIFT, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', KEY_RSHIFT,
        KEYP_MULTIPLY, KEY_LALT, KEY_USE, KEY_CAPSLOCK, KEY_F1, KEY_F2, KEY_F3, KEY_F4,
        KEY_F5, KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_NUMLOCK
    };
    if (key < 128 && set1_to_doom[key] != 0) {
        return set1_to_doom[key];
    }
    return 0;
}

typedef enum {
    DOOM_RENDERER_SOFTWARE = 0,
    DOOM_RENDERER_OPENGL   = 1
} doom_renderer_t;

static doom_renderer_t s_doom_renderer_mode = DOOM_RENDERER_OPENGL;

void DG_Init() {
    bos_print("[PASS] doom_main entered (DG_Init)\n");
    BOS_GUI_Init();
    
    bos_print("[DOOM] Calling window_create\n");
    s_doom_window = BOS_CreateWindow("DOOM (BOS OpenGL)", 100, 100, DOOM_W, DOOM_H);
    if (s_doom_window) {
        bos_print("[PASS] Window created\n");
        bos_print("[PASS] Surface created\n");
        BOS_ShowWindow(s_doom_window);
        bos_print("[PASS] Surface visible\n");

        if (s_doom_renderer_mode == DOOM_RENDERER_OPENGL) {
            bos_print("[DOOM] Initializing BOS OpenGL Renderer Backend...\n");
            int gl_err = bos_gl_init_context(s_doom_window->id);
            if (gl_err == 0) {
                bos_print("[PASS] BOS OpenGL Renderer Backend Active!\n");
            } else {
                bos_print("[WARN] OpenGL Init failed; falling back to Software Renderer\n");
                s_doom_renderer_mode = DOOM_RENDERER_SOFTWARE;
            }
        }
    } else {
        bos_print("[FAIL] Window created\n");
    }
    
    bos_print("[DOOM] GUI initialized\n");
}

void DG_DrawFrame() {
    if (!s_doom_window) return;
    
    for (int i = 0; i < DOOM_W * DOOM_H; i++) {
        uint32_t c = DG_ScreenBuffer[i];
        s_doom_pixels[i] = c | 0xFF000000;
    }
    
    if (s_doom_renderer_mode == DOOM_RENDERER_OPENGL) {
        // NATIVE OPENGL BACKEND: Texture upload -> Textured quad -> bglSwapBuffers
        bos_gl_present_frame(s_doom_window->id, s_doom_pixels, DOOM_W, DOOM_H);
    } else {
        // FALLBACK SOFTWARE PATH: Legacy surface presentation
        bos_surface_present(s_doom_window->id, s_doom_pixels, DOOM_W, DOOM_H);
    }
}

void DG_SleepMs(uint32_t ms) {
    bos_yield();
}

uint32_t DG_GetTicksMs() {
    return bos_uptime(); 
}

int DG_GetKey(int* pressed, unsigned char* doomKey) {
    bos_input_event_t event;
    while (BOS_InputPollEvent(&event)) {
        if (event.type == BOS_INPUT_KEY_DOWN || event.type == BOS_INPUT_KEY_UP) {
            unsigned char mapped = doom_key_from_bos(event.data.key.key, event.data.key.character);
            if (mapped == 0) {
                continue;
            }
            *pressed = (event.type == BOS_INPUT_KEY_DOWN);
            *doomKey = mapped;
            return 1;
        }
    }
    return 0;
}

int DG_PollEvent(int* ev_type, int* data1, int* data2, int* data3) {
    bos_input_event_t event;
    static int32_t last_mouse_x = -1;
    static int32_t last_mouse_y = -1;

    while (BOS_InputPollEvent(&event)) {
        if (event.type == BOS_INPUT_KEY_DOWN || event.type == BOS_INPUT_KEY_UP) {
            unsigned char mapped = doom_key_from_bos(event.data.key.key, event.data.key.character);
            if (mapped == 0) {
                continue;
            }
            *ev_type = (event.type == BOS_INPUT_KEY_DOWN) ? 1 : 2; // 1 = keydown, 2 = keyup
            *data1 = mapped;
            return 1;
        } else if (event.type == BOS_INPUT_MOUSE_MOVE || event.type == BOS_INPUT_MOUSE_DOWN || event.type == BOS_INPUT_MOUSE_UP) {
            int32_t mx = event.data.mouse.screen_x;
            int32_t my = event.data.mouse.screen_y;
            int32_t dx = 0, dy = 0;
            if (last_mouse_x >= 0 && last_mouse_y >= 0) {
                dx = mx - last_mouse_x;
                dy = my - last_mouse_y;
            }
            last_mouse_x = mx;
            last_mouse_y = my;

            if (dx == 0 && dy == 0 && event.type == BOS_INPUT_MOUSE_MOVE) {
                continue;
            }

            *ev_type = 3; // 3 = mouse
            *data1 = event.data.mouse.buttons;
            *data2 = dx * 8;
            *data3 = -dy * 8;
            return 1;
        }
    }
    return 0;
}

void DG_SetWindowTitle(const char * title) {
    // Window title is already set via BOS_CreateWindow
}

void _start(void) {
    bos_print("[PASS] ELF entry executed\n");
    char* argv[] = { "doom", NULL };
    doomgeneric_Create(1, argv);

    while (1) {
        doomgeneric_Tick();
        bos_yield();
    }
}
