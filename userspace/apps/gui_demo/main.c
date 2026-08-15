#include "../../libbos/include/bos.h"
#include "../../libbos_gui/include/bos_gui.h"
#include "../../libbos_gui/include/syscalls_gui.h"

void main(void) {
    bos_print("Starting ATOMS OS Ring 3 GUI Window Application...\n");
    BOS_GUI_Init();

    BOSWindow* window = BOS_CreateWindow("ATOMS Ring 3 Window", 200, 150, 600, 400);
    if (!window) {
        bos_print("Failed to create window!\n");
        bos_exit();
    }

    uint32_t* surface = 0;
    uint32_t stride = 0;
    if (sys_gui_map_surface(window->id, &surface, &stride) == 0 && surface) {
        // Draw test pattern inside window's private surface
        for (int y = 0; y < 400; y++) {
            for (int x = 0; x < 600; x++) {
                uint32_t color = 0xFF1E293B; // Dark Slate Canvas
                if (x < 10 || x >= 590 || y < 10 || y >= 390) {
                    color = 0xFF3B82F6; // Blue Border
                }
                surface[y * 600 + x] = color;
            }
        }
        sys_gui_invalidate(window->id, 0, 0, 600, 400);
    }

    BOS_ShowWindow(window);
    bos_print("Window shown. Entering event loop...\n");
    BOS_Run();

    bos_exit();
}

void _start(void) {
    main();
    bos_exit();
}
