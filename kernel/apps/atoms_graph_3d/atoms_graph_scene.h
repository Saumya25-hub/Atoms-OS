#ifndef ATOMS_GRAPH_SCENE_H
#define ATOMS_GRAPH_SCENE_H

#include <stdint.h>
#include <stdbool.h>
#include "kernel/graphics/gl/gl.h"

typedef struct {
    GLuint tex_procedural;
    GLuint fbo_offscreen;
    GLuint fbo_color_tex;
    GLuint fbo_depth_rbo;
    uint32_t fbo_w;
    uint32_t fbo_h;
    bool gl_resources_init;
} AtomsGraphGLResources;

void atoms_graph_scene_init_gl(AtomsGraphGLResources* res, uint32_t viewport_w, uint32_t viewport_h);
void atoms_graph_scene_cleanup_gl(AtomsGraphGLResources* res);

void atoms_graph_scene_render_stage(uint32_t stage_idx, float angle_deg, AtomsGraphGLResources* res,
                                     uint32_t viewport_w, uint32_t viewport_h,
                                     uint32_t* out_triangles, uint32_t* out_draw_calls);

#endif // ATOMS_GRAPH_SCENE_H
