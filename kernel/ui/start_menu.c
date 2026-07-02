#include "start_menu.h"
#include "../engine/horse_engine.h"

/* External debug print (stub) */
extern void debug_print(const char* msg);

/* External Drawing Primitives (Compositor stubs) */
extern void draw_rect(int x, int y, int w, int h, uint32_t color);
extern void draw_text(int x, int y, const char* text, uint32_t color);

/* State */
static bool is_open = false;

/* Dimensions for the Square Start Menu */
#define MENU_WIDTH  400
#define MENU_HEIGHT 300

/* Simple solid colors (No gradients, no transparency) */
#define COLOR_BG_LEFT   0x222222 // Dark Gray
#define COLOR_BG_RIGHT  0x111111 // Darker Gray
#define COLOR_BORDER    0x444444 // Border Gray
#define COLOR_TEXT      0xFFFFFF // White
#define COLOR_TEXT_ACC  0x00AFFF // ATOMS Blue

void start_menu_init(void) {
    is_open = false;
    debug_print("[UI] Start Menu Initialized\n");
}

void start_menu_open(void) {
    if (!is_open) {
        is_open = true;
        debug_print("[UI] Start Menu Opened\n");
        start_menu_draw();
    }
}

void start_menu_close(void) {
    if (is_open) {
        is_open = false;
        debug_print("[UI] Start Menu Closed\n");
        /* TODO: Trigger compositor desktop refresh to clear the menu pixels */
    }
}

static void render_pinned_apps(void) {
    /* Left Section - 250px wide */
    draw_rect(0, 0, 250, MENU_HEIGHT, COLOR_BG_LEFT);
    
    draw_text(20, 20, "Pinned Apps", COLOR_TEXT);
    
    /* Exactly 5 applications */
    draw_text(20, 60,  "1. ATOMS",    COLOR_TEXT);
    draw_text(20, 100, "2. Search",   COLOR_TEXT);
    draw_text(20, 140, "3. Files",    COLOR_TEXT);
    draw_text(20, 180, "4. Notes",    COLOR_TEXT);
    draw_text(20, 220, "5. Terminal", COLOR_TEXT);
}

static void render_system_panel(void) {
    /* Right Section - 150px wide */
    draw_rect(250, 0, 150, MENU_HEIGHT, COLOR_BG_RIGHT);
    
    /* Top: Temporary Profile */
    draw_text(270, 20, "ATOMS", COLOR_TEXT_ACC);
    
    /* Separator */
    draw_rect(260, 50, 130, 1, COLOR_BORDER);
    
    /* Bottom: System Options */
    draw_text(270, 70,  "Settings",  COLOR_TEXT);
    draw_text(270, 110, "Restart",   COLOR_TEXT);
    draw_text(270, 150, "Power Off", COLOR_TEXT);
}

void start_menu_draw(void) {
    if (!is_open) return;
    
    /* Render Sections */
    render_pinned_apps();
    render_system_panel();
    
    /* Outer Border */
    draw_rect(0, 0, MENU_WIDTH, 1, COLOR_BORDER);
    draw_rect(0, 0, 1, MENU_HEIGHT, COLOR_BORDER);
    draw_rect(MENU_WIDTH - 1, 0, 1, MENU_HEIGHT, COLOR_BORDER);
    draw_rect(0, MENU_HEIGHT - 1, MENU_WIDTH, 1, COLOR_BORDER);
}

void start_menu_handle_mouse(int x, int y, bool clicked) {
    if (!is_open) return;

    /* Clicked outside the start menu -> Close */
    if (x < 0 || x >= MENU_WIDTH || y < 0 || y >= MENU_HEIGHT) {
        if (clicked) {
            start_menu_close();
        }
        return;
    }

    if (clicked) {
        /* LEFT SECTION: Pinned Apps */
        if (x < 250) {
            if (y >= 50 && y < 90) {
                debug_print("[UI] Pinned App Clicked: ATOMS\n");
                horse_launch(APP_ID_ATOMS);
            } else if (y >= 90 && y < 130) {
                debug_print("[UI] Pinned App Clicked: Search\n");
                horse_launch(APP_ID_SEARCH);
            } else if (y >= 130 && y < 170) {
                debug_print("[UI] Pinned App Clicked: Files\n");
                horse_launch(APP_ID_FILES);
            } else if (y >= 170 && y < 210) {
                debug_print("[UI] Pinned App Clicked: Notes\n");
                horse_launch(APP_ID_NOTES);
            } else if (y >= 210 && y < 250) {
                debug_print("[UI] Pinned App Clicked: Terminal\n");
                horse_launch(APP_ID_TERMINAL);
            }
        } 
        /* RIGHT SECTION: System */
        else {
            if (y >= 60 && y < 100) {
                debug_print("[UI] System Clicked: Settings\n");
                horse_launch(APP_ID_SETTINGS);
            } else if (y >= 100 && y < 140) {
                debug_print("[UI] System Clicked: Restart\n");
                // Trigger Restart
            } else if (y >= 140 && y < 180) {
                debug_print("[UI] System Clicked: Power Off\n");
                // Trigger Power Off
            }
        }
        
        /* Any valid interaction closes the menu */
        start_menu_close();
    }
}
