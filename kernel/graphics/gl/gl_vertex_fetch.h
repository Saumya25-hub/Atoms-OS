#ifndef ATOMS_OS_GL_VERTEX_FETCH_H
#define ATOMS_OS_GL_VERTEX_FETCH_H

#include "kernel/graphics/gl/gl_types.h"
#include "kernel/graphics/gl/gl_state.h"
#include "kernel/graphics/gl/gl_pipeline.h"

#ifdef __cplusplus
extern "C" {
#endif

uint32_t gl_client_array_get_effective_stride(GLint size, GLenum type, GLsizei stride);
bool     gl_fetch_client_vertex(GLContextState* state, uint32_t index, GLVertex* out_v);

#ifdef __cplusplus
}
#endif

#endif // ATOMS_OS_GL_VERTEX_FETCH_H
