#include "atoms_graph_3d.h"
#include "atoms_graph_benchmark.h"
#include "kernel/wm/bwe/include/bwe.h"
#include "kernel/engine/horse_engine.h"
#include "kernel/core/memory/heap/include/heap.h"

extern void display_print(const char* s);
extern uint64_t timer_get_ticks(void);

static uint32_t s_win_id = 0;
static bool s_active = false;
static AtomsGraphBenchmark s_benchmark;
static uint64_t s_last_tick_ms = 0;

static void atoms_graph_3d_render_callback(BWE_Window* self) {
    if (!self || !s_active) return;
    
    uint64_t now = timer_get_ticks();
    float delta_ms = (s_last_tick_ms > 0) ? (float)(now - s_last_tick_ms) : 16.6f;
    s_last_tick_ms = now;
    
    atoms_graph_benchmark_step(&s_benchmark, delta_ms);
}

static void atoms_graph_3d_event_callback(uint32_t win_id, const BWE_Event* event) {
    if (!event) return;
    if (event->type == BWE_EVENT_WINDOW_CLOSE) {
        atoms_graph_3d_close();
    }
}

int atoms_graph_3d_launch(uint32_t* out_win_id) {
    if (s_active && s_win_id != 0) {
        BWE_Window* win = BWE_GetWindow(s_win_id);
        if (win) {
            BWE_InvalidateWindow(s_win_id);
            if (out_win_id) *out_win_id = s_win_id;
            return 0;
        }
    }
    
    display_print("[ATOMS-GRAPH] Launching ATOMS Graph 3D Native Window...\n");
    
    int32_t win_w = 900;
    int32_t win_h = 512;
    int32_t win_x = 420;
    int32_t win_y = 180;
    
    bwe_error_t err = BOS_CreateSurface(BWE_DESKTOP_ID, win_x, win_y, win_w, win_h,
                                        BWE_WINDOW_CHILD | BWE_WINDOW_MOVABLE,
                                        &s_win_id);
    if (err != BWE_SUCCESS) {
        display_print("[ATOMS-GRAPH] FAIL: Failed to create window surface!\n");
        return -1;
    }
    
    BWE_Window* win = BWE_GetWindow(s_win_id);
    if (win) {
        win->type = BWE_TYPE_WINDOW;
        win->on_render = atoms_graph_3d_render_callback;
        win->on_event = atoms_graph_3d_event_callback;
        win->user_data = (void*)APP_ID_GRAPH_3D;
        
        // Set window titlebar text cleanly
        BOS_SetText(s_win_id, "ATOMS GRAPH 3D");
        
        // Calculate actual client bounds for benchmark initialization
        extern void BWE_Geometry_CalculateClientBounds(BWE_Window* w, BWE_Rect* o);
        BWE_Rect client_rect;
        BWE_Geometry_CalculateClientBounds(win, &client_rect);
        
        uint32_t client_w = (client_rect.width > 0) ? (uint32_t)client_rect.width : (uint32_t)(win_w - 10);
        uint32_t client_h = (client_rect.height > 0) ? (uint32_t)client_rect.height : (uint32_t)(win_h - 40);
        
        atoms_graph_benchmark_init(&s_benchmark, s_win_id, client_w, client_h, 30000); // 30s per stage = 300s (5 mins)
        atoms_graph_benchmark_start(&s_benchmark);
        
        s_active = true;
        s_last_tick_ms = timer_get_ticks();
        BWE_InvalidateWindow(s_win_id);
    }
    
    if (out_win_id) *out_win_id = s_win_id;
    display_print("[ATOMS-GRAPH] SUCCESS: ATOMS Graph 3D Native Window Active!\n");
    return 0;
}

void atoms_graph_3d_close(void) {
    if (s_win_id != 0) {
        display_print("[ATOMS-GRAPH] Closing ATOMS Graph 3D Window...\n");
        atoms_graph_benchmark_cleanup(&s_benchmark);
        BWE_Window* win = BWE_GetWindow(s_win_id);
        if (win) {
            win->user_data = NULL;
        }
        BOS_DestroySurface(s_win_id);
        s_win_id = 0;
        s_active = false;
    }
}

void atoms_graph_3d_pump_frame(float delta_ms) {
    if (s_active && s_benchmark.is_running) {
        atoms_graph_benchmark_step(&s_benchmark, delta_ms);
    }
}

bool atoms_graph_3d_is_active(void) {
    return s_active;
}
