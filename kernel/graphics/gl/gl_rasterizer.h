#ifndef ATOMS_OS_GL_RASTERIZER_H
#define ATOMS_OS_GL_RASTERIZER_H

#include "kernel/graphics/gl/gl_types.h"
#include "kernel/graphics/gl/gl_constants.h"
#include "kernel/graphics/gl/gl_viewport.h"
#include "kernel/graphics/bgl/bgl_drawable.h"
#include "kernel/graphics/gl/gl_state.h"
#include "kernel/graphics/gl/gl_fbo.h"

#ifdef __cplusplus
extern "C" {
#endif

void gl_rasterize_triangle(const GLScreenVertex in_v[3], const GLRenderTarget* target, GLContextState* state);

#ifdef __cplusplus
}
#endif

#endif // ATOMS_OS_GL_RASTERIZER_H
