#include "atoms_graph_renderer.h"
#include "kernel/core/lib/include/string.h"
#include "bovisual/Include/bovisual_types.h"
#include "kernel/wm/bwe/include/bwe.h"

extern void display_print(const char* s);

bool atoms_graph_renderer_init(AtomsGraphRenderer* r, uint32_t win_id, uint32_t width, uint32_t height) {
    if (!r || win_id == 0 || width == 0 || height == 0) return false;
    
    r->window_id = win_id;
    r->total_width = width;
    r->total_height = height;
    
    r->panel_w = 260; // 260px right side panel for metrics & live stats
    r->viewport_w = (width > r->panel_w) ? (width - r->panel_w) : (width / 2);
    r->viewport_h = height;
    
    r->drawable = bglCreateDrawableForWindow(win_id);
    if (!r->drawable) {
        display_print("[ATOMS-GRAPH] FAIL: Failed to create BGLDrawable for window!\n");
        return false;
    }
    
    // Store actual client-area dimensions from BGL drawable
    r->total_width = r->drawable->width;
    r->total_height = r->drawable->height;
    
    r->panel_w = 260; // 260px right side panel for metrics & live stats
    r->viewport_w = (r->total_width > r->panel_w) ? (r->total_width - r->panel_w) : (r->total_width / 2);
    r->viewport_h = r->total_height;
    
    r->context = bglCreateContext(r->drawable);
    if (!r->context) {
        display_print("[ATOMS-GRAPH] FAIL: Failed to create BGLContext!\n");
        bglDestroyDrawable(r->drawable);
        r->drawable = NULL;
        return false;
    }
    
    if (!bglMakeCurrent(r->context, r->drawable)) {
        display_print("[ATOMS-GRAPH] FAIL: bglMakeCurrent failed!\n");
        bglDestroyContext(r->context);
        bglDestroyDrawable(r->drawable);
        r->context = NULL;
        r->drawable = NULL;
        return false;
    }
    
    atoms_graph_scene_init_gl(&r->gl_res, r->viewport_w, r->viewport_h);
    r->is_initialized = true;
    
    display_print("[ATOMS-GRAPH] Renderer initialized successfully.\n");
    return true;
}

void atoms_graph_renderer_cleanup(AtomsGraphRenderer* r) {
    if (!r || !r->is_initialized) return;
    
    if (r->context && r->drawable) {
        bglMakeCurrent(r->context, r->drawable);
        atoms_graph_scene_cleanup_gl(&r->gl_res);
    }
    
    bglReleaseCurrent();
    
    if (r->context) {
        bglDestroyContext(r->context);
        r->context = NULL;
    }
    
    if (r->drawable) {
        bglDestroyDrawable(r->drawable);
        r->drawable = NULL;
    }
    
    r->is_initialized = false;
    display_print("[ATOMS-GRAPH] Renderer cleaned up successfully.\n");
}

bool atoms_graph_renderer_render_frame(AtomsGraphRenderer* r, uint32_t stage_idx, float angle_deg,
                                       uint32_t* out_triangles, uint32_t* out_draw_calls) {
    if (!r || !r->is_initialized || !r->context || !r->drawable) return false;
    
    if (!bglMakeCurrent(r->context, r->drawable)) return false;
    
    atoms_graph_scene_render_stage(stage_idx, angle_deg, &r->gl_res,
                                   r->viewport_w, r->viewport_h,
                                   out_triangles, out_draw_calls);
                                   
    return true;
}

void atoms_graph_renderer_render_hud(AtomsGraphRenderer* r, const AtomsGraphMetrics* m, bool is_finished, uint32_t score) {
    if (!r || !r->drawable || !r->drawable->color_buffer || !m) return;
    
    BVFramebuffer client_fb;
    client_fb.width = r->drawable->width;
    client_fb.height = r->drawable->height;
    client_fb.pitch = r->drawable->pitch;
    client_fb.buffer = r->drawable->color_buffer;
    
    extern void atoms_graph_ui_render_panel(const BVFramebuffer* fb, BWE_Rect win_bounds, const AtomsGraphMetrics* m, bool is_finished, uint32_t score);
    extern void atoms_graph_ui_render_results_screen(const BVFramebuffer* fb, BWE_Rect win_bounds, const AtomsGraphMetrics* m, uint32_t score);
    
    BWE_Rect client_bounds = {0, 0, (int32_t)r->drawable->width, (int32_t)r->drawable->height};
    atoms_graph_ui_render_panel(&client_fb, client_bounds, m, is_finished, score);
    
    if (is_finished) {
        atoms_graph_ui_render_results_screen(&client_fb, client_bounds, m, score);
    }
}
