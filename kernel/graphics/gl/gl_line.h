#ifndef ATOMS_OS_GL_LINE_H
#define ATOMS_OS_GL_LINE_H

#include "kernel/graphics/gl/gl_types.h"
#include "kernel/graphics/gl/gl_viewport.h"
#include "kernel/graphics/bgl/bgl.h"
#include "kernel/graphics/gl/gl_fbo.h"

#ifdef __cplusplus
extern "C" {
#endif

struct GLContextState;

void gl_rasterize_line(const GLScreenVertex* v0, const GLScreenVertex* v1, float width, const GLRenderTarget* target, struct GLContextState* state);

#ifdef __cplusplus
}
#endif

#endif // ATOMS_OS_GL_LINE_H
