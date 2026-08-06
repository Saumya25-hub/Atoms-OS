#include "kernel/graphics/display/dpdp/include/dpdp_preview.h"

extern void display_print(const char* str);

void dpdp_render_mini_preview(const char* stage_name, dpdp_status_t status, bool is_green_line, bool is_black) {
    if (!stage_name) return;

    display_print("     ");
    display_print(stage_name);
    display_print("\n     │\n     ▼\n     ");

    if (status == DPDP_STATUS_FAIL || is_black) {
        display_print("[░░░░░░░░░░]  Black / Failed Frame\n");
    } else if (is_green_line) {
        display_print("[██░░░░░░░░]  Green Scanline Only\n");
    } else {
        display_print("[██████████]  Rendered Image Content Valid\n");
    }
}
