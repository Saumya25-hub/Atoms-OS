#ifndef ATOMS_OS_GL_PIPELINE_H
#define ATOMS_OS_GL_PIPELINE_H

#include "kernel/graphics/gl/gl_types.h"
#include "kernel/graphics/gl/gl_constants.h"
#include "kernel/graphics/gl/gl_math.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GL_OUTCODE_INSIDE 0
#define GL_OUTCODE_LEFT   (1 << 0)
#define GL_OUTCODE_RIGHT  (1 << 1)
#define GL_OUTCODE_BOTTOM (1 << 2)
#define GL_OUTCODE_TOP    (1 << 3)
#define GL_OUTCODE_NEAR   (1 << 4)
#define GL_OUTCODE_FAR    (1 << 5)

typedef struct {
    GLVec4   obj_pos;
    GLVec4   eye_pos;
    GLVec4   clip_pos;
    GLfloat  color[4];
    GLfloat  texcoord[4];
    GLfloat  normal[3];
    GLfloat  fog_coord;
    GLfloat  inv_w;
    uint32_t clip_outcode;
    bool     is_valid;
} GLVertex;

typedef struct {
    GLenum   type;
    uint32_t vertex_count;
    GLVertex vertices[4];
} GLPrimitive;

typedef struct {
    uint32_t primitive_count;
    GLPrimitive* primitives;
} GLProcessedPrimitivePacket;

void gl_pipeline_transform_vertex(GLVertex* v, const GLMatrix4x4* modelview, const GLMatrix4x4* projection);
uint32_t gl_pipeline_classify_clip(GLVec4 clip_pos);
uint32_t gl_pipeline_assemble_primitives(GLenum mode, const GLVertex* vertices, uint32_t count, GLPrimitive* out_primitives, uint32_t max_primitives);

#ifdef __cplusplus
}
#endif

#endif // ATOMS_OS_GL_PIPELINE_H
