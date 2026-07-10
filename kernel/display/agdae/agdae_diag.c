/**
 * @file agdae_diag.c
 * @brief ATOMS OS Display Adaptation Engine - Diagnostics & Reporting
 */

#include "agdae.h"

extern void display_print(const char* str);
extern void display_print_dec(uint32_t val);

extern AGDAE_Metrics g_agdae_metrics;

void AGDAE_DumpDiagnostics(void) {
    display_print("\n=================================================\n");
    display_print(" AGDAE Phase 6: Adaptation Engine Report\n");
    display_print("=================================================\n");

    display_print("1. Resolutions:\n");
    display_print("   Physical HW Framebuffer : ");
    display_print_dec(g_agdae_metrics.physical_width);
    display_print(" x ");
    display_print_dec(g_agdae_metrics.physical_height);
    display_print("\n");
    
    display_print("   Logical Desktop Space   : ");
    display_print_dec(g_agdae_metrics.logical_width);
    display_print(" x ");
    display_print_dec(g_agdae_metrics.logical_height);
    display_print("\n");

    display_print("2. Scaling Profile:\n");
    display_print("   Scale Factor : ");
    display_print_dec(g_agdae_metrics.scale_factor_pct);
    display_print(" %\n");
    display_print("   HiDPI Mode   : ");
    display_print(g_agdae_metrics.is_hidpi ? "YES" : "NO");
    display_print("\n");

    display_print("3. Authoritative Geometry:\n");
    display_print("   Desktop Rect : ");
    display_print_dec((uint32_t)g_agdae_metrics.desktop_rect.width);
    display_print(" x ");
    display_print_dec((uint32_t)g_agdae_metrics.desktop_rect.height);
    display_print("\n");

    display_print("   Taskbar Rect : Y=");
    display_print_dec((uint32_t)g_agdae_metrics.taskbar_rect.y);
    display_print(", H=");
    display_print_dec((uint32_t)g_agdae_metrics.taskbar_rect.height);
    display_print("\n");

    display_print("   Safe Area    : Inset 16px (Scaled)\n");
    
    display_print("=================================================\n\n");
}
