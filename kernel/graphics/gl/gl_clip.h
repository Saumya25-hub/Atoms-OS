#ifndef ATOMS_OS_GL_CLIP_H
#define ATOMS_OS_GL_CLIP_H

#include "kernel/graphics/gl/gl_types.h"
#include "kernel/graphics/gl/gl_constants.h"
#include "kernel/graphics/gl/gl_pipeline.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GL_MAX_CLIPPED_POLYGON_VERTICES 16

uint32_t gl_clip_triangle(const GLVertex in_tri[3], GLVertex out_polygon[GL_MAX_CLIPPED_POLYGON_VERTICES]);
uint32_t gl_triangulate_polygon(const GLVertex* polygon, uint32_t poly_vertex_count, GLPrimitive* out_triangles, uint32_t max_triangles);

#ifdef __cplusplus
}
#endif

#endif // ATOMS_OS_GL_CLIP_H
