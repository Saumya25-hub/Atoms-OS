#include "../include/bwe_diagnostics.h"
#include "../include/bwe_geometry.h"
#include "kernel/core/lib/include/string.h"

extern uint64_t timer_get_ticks(void);
extern void display_print(const char* str);
extern void display_print_dec(uint32_t val);

static BWE_DiagLog s_logs[BWE_DIAG_MAX_LOGS];
static uint32_t s_log_head = 0;
static uint32_t s_log_count = 0;

void BWE_Diag_Init(void) {
    s_log_head = 0;
    s_log_count = 0;
    memset(s_logs, 0, sizeof(s_logs));
}

void BWE_Diag_LogViolation(uint32_t window_id, const char* reason) {
    BWE_DiagLog* log = &s_logs[s_log_head];
    log->window_id = window_id;
    strncpy(log->reason, reason, sizeof(log->reason) - 1);
    log->reason[sizeof(log->reason) - 1] = '\0';
    log->timestamp = timer_get_ticks();
    
    s_log_head = (s_log_head + 1) % BWE_DIAG_MAX_LOGS;
    if (s_log_count < BWE_DIAG_MAX_LOGS) {
        s_log_count++;
    }
    
    display_print("[BWE_DIAG] Geometry Violation - ID ");
    display_print_dec(window_id);
    display_print(": ");
    display_print(reason);
    display_print("\n");
}

uint32_t BWE_Diag_GetLogs(BWE_DiagLog* out_logs, uint32_t max_logs) {
    if (!out_logs || max_logs == 0) return 0;
    
    uint32_t count = (s_log_count < max_logs) ? s_log_count : max_logs;
    uint32_t idx = (s_log_head >= count) ? (s_log_head - count) : (BWE_DIAG_MAX_LOGS - (count - s_log_head));
    
    for (uint32_t i = 0; i < count; i++) {
        out_logs[i] = s_logs[idx];
        idx = (idx + 1) % BWE_DIAG_MAX_LOGS;
    }
    
    return count;
}

void BWE_Diag_ClearLogs(void) {
    s_log_head = 0;
    s_log_count = 0;
}

void BWE_Diag_GuardBounds(BWE_Window* parent, BWE_Window* child) {
    if (!parent || !child || parent->id == BWE_DESKTOP_ID) return;
    
    BWE_Rect p_client;
    BWE_Geometry_CalculateClientBounds(parent, &p_client);
    
    BWE_Rect c_screen = child->screen_bounds;
    
    // Check if child is completely outside client area or overlaps the edges
    if (c_screen.x < p_client.x || c_screen.y < p_client.y || 
        c_screen.x + c_screen.width > p_client.x + p_client.width ||
        c_screen.y + c_screen.height > p_client.y + p_client.height) {
        
        BWE_Diag_LogViolation(child->id, "Child bounds exceed parent client bounds");
    }
}
