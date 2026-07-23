#ifndef ATOMS_OS_GL_SAMPLER_H
#define ATOMS_OS_GL_SAMPLER_H

#include "kernel/graphics/gl/gl_types.h"
#include "kernel/graphics/gl/gl_constants.h"
#include "kernel/graphics/gl/gl_texture.h"

#ifdef __cplusplus
extern "C" {
#endif

void gl_sample_texture_lod(const GLTextureObject* tex, float u, float v, float lod, float out_rgba[4]);

static inline void gl_sample_texture(const GLTextureObject* tex, float u, float v, float out_rgba[4]) {
    gl_sample_texture_lod(tex, u, v, 0.0f, out_rgba);
}

#ifdef __cplusplus
}
#endif

#endif // ATOMS_OS_GL_SAMPLER_H
