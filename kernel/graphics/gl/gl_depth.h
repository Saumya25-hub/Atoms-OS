#ifndef ATOMS_OS_GL_DEPTH_H
#define ATOMS_OS_GL_DEPTH_H

#include "kernel/graphics/gl/gl_types.h"
#include "kernel/graphics/gl/gl_constants.h"
#include "kernel/graphics/bgl/bgl_drawable.h"
#include "kernel/graphics/gl/gl_fbo.h"

#ifdef __cplusplus
extern "C" {
#endif

bool gl_depth_test_pixel(GLenum depth_func, bool writemask, float frag_z, float* depth_ptr);
void gl_depth_clear(const GLRenderTarget* target, float clear_depth_val);

#ifdef __cplusplus
}
#endif

#endif // ATOMS_OS_GL_DEPTH_H
