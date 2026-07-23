#ifndef ATOMS_GRAPH_RENDERER_H
#define ATOMS_GRAPH_RENDERER_H

#include <stdint.h>
#include <stdbool.h>
#include "kernel/graphics/bgl/bgl.h"
#include "atoms_graph_scene.h"
#include "atoms_graph_metrics.h"

typedef struct {
    uint32_t window_id;
    BGLDrawable* drawable;
    BGLContext*  context;
    
    uint32_t total_width;
    uint32_t total_height;
    uint32_t viewport_w;
    uint32_t viewport_h;
    uint32_t panel_w;
    
    AtomsGraphGLResources gl_res;
    bool is_initialized;
} AtomsGraphRenderer;

bool atoms_graph_renderer_init(AtomsGraphRenderer* r, uint32_t win_id, uint32_t width, uint32_t height);
void atoms_graph_renderer_cleanup(AtomsGraphRenderer* r);

bool atoms_graph_renderer_render_frame(AtomsGraphRenderer* r, uint32_t stage_idx, float angle_deg,
                                       uint32_t* out_triangles, uint32_t* out_draw_calls);
void atoms_graph_renderer_render_hud(AtomsGraphRenderer* r, const AtomsGraphMetrics* m, bool is_finished, uint32_t score);

#endif // ATOMS_GRAPH_RENDERER_H
