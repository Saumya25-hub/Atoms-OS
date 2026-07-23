#ifndef ATOMS_OS_GL_VIEWPORT_H
#define ATOMS_OS_GL_VIEWPORT_H

#include "kernel/graphics/gl/gl_types.h"
#include "kernel/graphics/gl/gl_constants.h"
#include "kernel/graphics/gl/gl_pipeline.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float x;
    float y;
    float z;
    float inv_w;
    float s_over_w;
    float t_over_w;
    float fog_z_over_w;
    float r;
    float g;
    float b;
    float a;
} GLScreenVertex;

bool gl_viewport_transform_vertex(const GLVertex* clip_v, int vp_x, int vp_y, int vp_w, int vp_h, GLScreenVertex* out_v);

#ifdef __cplusplus
}
#endif

#endif // ATOMS_OS_GL_VIEWPORT_H
