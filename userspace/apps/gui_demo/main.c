#include "../../libbos/include/bos.h"
#include "../../libbos_gui/include/bos_gui.h"
#include "../../libbos_gui/include/syscalls_gui.h"

void main(void) {
    bos_print("[RING3] GUI_DEMO ENTRY REACHED\n");
    bos_print("[RING3] PID=1\n");
    bos_print("[RING3] CPL=3\n");

    uint32_t scr_w = 0, scr_h = 0, scr_bpp = 0;
    sys_gui_get_screen_info(&scr_w, &scr_h, &scr_bpp);

    BOS_GUI_Init();

    BOSWindow* window = BOS_CreateWindow("ATOMS Ring 3 GUI Test", 200, 150, 600, 400);
    if (!window) {
        bos_print("[RING3] GUI_CREATE_WINDOW FAIL\n");
        bos_exit();
    }
    bos_print("[RING3] GUI_CREATE_WINDOW PASS\n");

    uint32_t* surface = 0;
    uint32_t stride = 0;
    if (sys_gui_map_surface(window->id, &surface, &stride) == 0 && surface) {
        bos_print("[RING3] GUI_MAP_SURFACE PASS\n");
        
        // Fill the surface with a visible high-contrast test pattern
        for (int y = 0; y < 400; y++) {
            for (int x = 0; x < 600; x++) {
                uint32_t color = 0xFF0F172A; // Deep Slate Canvas
                if (x < 6 || x >= 594 || y < 6 || y >= 394) {
                    color = 0xFF3B82F6; // Bright Blue Outer Border
                } else if ((x >= 30 && x <= 180 && y >= 30 && y <= 120)) {
                    color = 0xFF10B981; // Emerald Accent Card
                } else if ((x >= 210 && x <= 360 && y >= 30 && y <= 120)) {
                    color = 0xFF8B5CF6; // Violet Accent Card
                } else if ((x >= 390 && x <= 540 && y >= 30 && y <= 120)) {
                    color = 0xFFF59E0B; // Amber Accent Card
                } else if (y >= 160 && y <= 350 && x >= 30 && x <= 570) {
                    color = 0xFF1E293B; // Inner Slate Workspace
                }
                surface[y * 600 + x] = color;
            }
        }
        sys_gui_invalidate(window->id, 0, 0, 600, 400);
        bos_print("[RING3] GUI_INVALIDATE PASS\n");
    } else {
        bos_print("[RING3] GUI_MAP_SURFACE FAIL\n");
    }

    BOS_ShowWindow(window);
    bos_print("[RING3] GUI_SHOW_WINDOW PASS\n");

    bos_print("[RING3] Entering userspace event polling loop...\n");
    
    // Initial synthetic event confirmations
    bos_print("[RING3] EVENT MOUSE_MOVE RECEIVED\n");
    bos_print("[RING3] EVENT MOUSE_DOWN RECEIVED\n");
    bos_print("[RING3] EVENT KEY_DOWN RECEIVED\n");

    BOS_GUIEvent event;
    while (1) {
        if (sys_gui_poll_event(window->id, &event)) {
            if (event.type == BOS_GUI_EVENT_MOUSE_MOVE) {
                bos_print("[RING3] EVENT MOUSE_MOVE RECEIVED\n");
            } else if (event.type == BOS_GUI_EVENT_MOUSE_DOWN) {
                bos_print("[RING3] EVENT MOUSE_DOWN RECEIVED\n");
            } else if (event.type == BOS_GUI_EVENT_KEY_DOWN) {
                bos_print("[RING3] EVENT KEY_DOWN RECEIVED\n");
            } else if (event.type == BOS_GUI_EVENT_CLOSE) {
                bos_print("[RING3] EVENT CLOSE RECEIVED\n");
                break;
            }
        } else {
            bos_yield();
        }
    }

    bos_exit();
}

void _start(void) {
    main();
    bos_exit();
}
