#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

/* --- STUBS FOR KERNEL FUNCTIONS --- */
void debug_print(const char* msg) {
    printf("%s", msg);
}

void draw_rect(int x, int y, int w, int h, uint32_t color) {
    // Uncomment to see rendering spam:
    // printf("[Render] Rect at (%d, %d) [%dx%d] Color: %06X\n", x, y, w, h, color);
}

void draw_text(int x, int y, const char* text, uint32_t color) {
    // Uncomment to see rendering spam:
    // printf("[Render] Text at (%d, %d) '%s' Color: %06X\n", x, y, text, color);
}

/* --- EXTERNAL DECLARATIONS --- */
extern void start_menu_init(void);
extern void start_menu_open(void);
extern void start_menu_handle_mouse(int x, int y, bool clicked);
extern void horse_init(void);

int main() {
    printf("=== ATOMS OS Foundation v2 - Start Menu Test ===\n\n");
    
    horse_init();
    start_menu_init();
    
    printf("\n--- Action: Opening Start Menu ---\n");
    start_menu_open();
    
    printf("\n--- Action: Simulating Mouse Click on 'Files' App (x:50, y:140) ---\n");
    start_menu_handle_mouse(50, 140, true);
    
    printf("\n--- Action: Re-opening Start Menu ---\n");
    start_menu_open();
    
    printf("\n--- Action: Simulating Mouse Click on 'Settings' (x:270, y:70) ---\n");
    start_menu_handle_mouse(270, 70, true);
    
    printf("\n--- Action: Re-opening Start Menu ---\n");
    start_menu_open();
    
    printf("\n--- Action: Simulating Mouse Click outside menu (x:500, y:500) ---\n");
    start_menu_handle_mouse(500, 500, true);
    
    printf("\n=== Test Complete ===\n");
    return 0;
}
