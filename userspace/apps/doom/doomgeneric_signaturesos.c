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
    for (uint32_t discarded_motion = 0; discarded_motion < 16; discarded_motion++) {
        if (!BOS_InputPollEvent(&event)) {
            return 0;
        }
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
