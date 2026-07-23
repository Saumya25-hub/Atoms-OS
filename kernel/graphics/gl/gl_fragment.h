#ifndef ATOMS_OS_GL_FRAGMENT_H
#define ATOMS_OS_GL_FRAGMENT_H

#include "kernel/graphics/gl/gl_types.h"
#include "kernel/graphics/gl/gl_constants.h"
#include "kernel/graphics/bgl/bgl.h"
#include "kernel/graphics/gl/gl_fbo.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int   x;
    int   y;
    float depth;
    float color[4];
    float fog_coord;
    float texcoord[4];
    float lod;
    bool  has_texture;
} GLFragment;

struct GLContextState; // Forward declaration

bool gl_fragment_process(struct GLContextState* state, const GLRenderTarget* target, const GLFragment* frag);

#ifdef __cplusplus
}
#endif

#endif // ATOMS_OS_GL_FRAGMENT_H
