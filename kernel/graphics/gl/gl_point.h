#ifndef ATOMS_OS_GL_POINT_H
#define ATOMS_OS_GL_POINT_H

#include "kernel/graphics/gl/gl_types.h"
#include "kernel/graphics/gl/gl_viewport.h"
#include "kernel/graphics/bgl/bgl.h"
#include "kernel/graphics/gl/gl_fbo.h"

#ifdef __cplusplus
extern "C" {
#endif

struct GLContextState;

void gl_rasterize_point(const GLScreenVertex* v, float size, const GLRenderTarget* target, struct GLContextState* state);

#ifdef __cplusplus
}
#endif

#endif // ATOMS_OS_GL_POINT_H
