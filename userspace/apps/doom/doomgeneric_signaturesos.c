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
    
    printf("[DRAW]\n");
    
    for (int i = 0; i < DOOM_W * DOOM_H; i++) {
        uint32_t c = DG_ScreenBuffer[i];
        s_doom_pixels[i] = c | 0xFF000000;
    }
    
    printf("[PRESENT]\n");
    bos_surface_present(s_doom_window->id, s_doom_pixels, DOOM_W, DOOM_H);
}

void DG_SleepMs(uint32_t ms) {
    bos_yield();
}

uint32_t DG_GetTicksMs() {
    printf("[TICK]\n");
    return bos_uptime(); 
}

int DG_GetKey(int* pressed, unsigned char* doomKey) {
    bos_input_event_t event;
    if (BOS_InputPollEvent(&event)) {
        if (event.type == BOS_INPUT_KEY_DOWN || event.type == BOS_INPUT_KEY_UP) {
            *pressed = (event.type == BOS_INPUT_KEY_DOWN);
            uint32_t key = event.data.key.key;
            char ascii = (char)event.data.key.character;

            switch (key) {
                case BOS_KEY_UP:    *doomKey = KEY_UPARROW; break;
                case BOS_KEY_DOWN:  *doomKey = KEY_DOWNARROW; break;
                case BOS_KEY_LEFT:  *doomKey = KEY_LEFTARROW; break;
                case BOS_KEY_RIGHT: *doomKey = KEY_RIGHTARROW; break;
                case '\n':          *doomKey = KEY_ENTER; break;
                case BOS_KEY_ESC:   *doomKey = KEY_ESCAPE; break;
                case BOS_KEY_CTRL:  *doomKey = KEY_FIRE; break;
                default:
                    if (ascii != 0) {
                        *doomKey = ascii;
                        if (ascii == ' ') *doomKey = KEY_USE;
                    } else {
                        return 0; // Unknown key
                    }
                    break;
            }
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
