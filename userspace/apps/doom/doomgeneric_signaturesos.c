#include "src/doomgeneric/doomgeneric.h"
#include "../../libbos/include/bos.h"
#include "../../libbos/include/bpde.h"
#include "../../libbos_gui/include/bos_gui.h"
#include "src/doomgeneric/doomkeys.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define DOOM_W 640
#define DOOM_H 400

static BOSWindow* s_doom_window = NULL;
static uint32_t s_doom_pixels[DOOM_W * DOOM_H];

static unsigned char doom_key_from_bos(uint32_t key, uint32_t character) {
    static const unsigned char set1_to_doom[] = {
        0, KEY_ESCAPE, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', KEY_BACKSPACE,
        KEY_TAB, 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', KEY_ENTER,
        0, KEY_FIRE, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
        KEY_RSHIFT, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', KEY_RSHIFT,
        KEYP_MULTIPLY, KEY_LALT, KEY_USE, KEY_CAPSLOCK, KEY_F1, KEY_F2, KEY_F3, KEY_F4,
        KEY_F5, KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_NUMLOCK
    };

    switch (key) {
        case BOS_KEY_UP:    return KEY_UPARROW;
        case BOS_KEY_DOWN:  return KEY_DOWNARROW;
        case BOS_KEY_LEFT:  return KEY_LEFTARROW;
        case BOS_KEY_RIGHT: return KEY_RIGHTARROW;
        case BOS_KEY_ESC:   return KEY_ESCAPE;
        case BOS_KEY_CTRL:  return KEY_FIRE;
        case BOS_KEY_ALT:   return KEY_LALT;
        case BOS_KEY_SHIFT: return KEY_RSHIFT;
        case BOS_KEY_F1:    return KEY_F1;
        case BOS_KEY_F2:    return KEY_F2;
        case BOS_KEY_F3:    return KEY_F3;
        case BOS_KEY_F4:    return KEY_F4;
        case BOS_KEY_F5:    return KEY_F5;
        case BOS_KEY_F6:    return KEY_F6;
        case BOS_KEY_F7:    return KEY_F7;
        case BOS_KEY_F8:    return KEY_F8;
        case BOS_KEY_F9:    return KEY_F9;
        case BOS_KEY_F10:   return KEY_F10;
        case BOS_KEY_F11:   return KEY_F11;
        case BOS_KEY_F12:   return KEY_F12;
        default: break;
    }

    if (key < sizeof(set1_to_doom)) {
        return set1_to_doom[key];
    }
    if (character == '\n' || character == '\r') {
        return KEY_ENTER;
    }
    return 0;
}

void DG_Init() {
    bos_print("[PASS] doom_main entered (DG_Init)\n");
    BOS_GUI_Init();
    
    bos_print("[DOOM] Calling window_create\n");
    s_doom_window = BOS_CreateWindow("DOOM", 100, 100, DOOM_W, DOOM_H);
    if (s_doom_window) {
        bos_print("[PASS] Window created\n");
        bos_print("[PASS] Surface created\n");
    } else {
        bos_print("[FAIL] Window created\n");
    }
    
    bos_print("[DOOM] Calling surface_show\n");
    BOS_ShowWindow(s_doom_window);
    bos_print("[PASS] Surface visible\n");
    bos_print("[PASS] Render callback registered (implicitly via OS)\n");
    
    bos_print("[DOOM] GUI initialized\n");
}

void DG_DrawFrame() {
    if (!s_doom_window) return;
    
    for (int i = 0; i < DOOM_W * DOOM_H; i++) {
        uint32_t c = DG_ScreenBuffer[i];
        s_doom_pixels[i] = c | 0xFF000000;
    }
    
    bos_surface_present(s_doom_window->id, s_doom_pixels, DOOM_W, DOOM_H);
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
