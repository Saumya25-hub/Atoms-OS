#ifndef ATOMS_OS_GL_DISPLAY_LIST_H
#define ATOMS_OS_GL_DISPLAY_LIST_H

#include "kernel/graphics/gl/gl_types.h"
#include "kernel/graphics/gl/gl_state.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    GL_CMD_BEGIN,
    GL_CMD_END,
    GL_CMD_VERTEX4F,
    GL_CMD_COLOR4F,
    GL_CMD_NORMAL3F,
    GL_CMD_TEXCOORD4F,
    GL_CMD_ENABLE,
    GL_CMD_DISABLE,
    GL_CMD_MATRIX_MODE,
    GL_CMD_LOAD_IDENTITY,
    GL_CMD_TRANSLATEF,
    GL_CMD_ROTATEF,
    GL_CMD_SCALEF,
    GL_CMD_LIGHTFV,
    GL_CMD_MATERIALFV,
    GL_CMD_CALL_LIST,
    GL_CMD_TEX_IMAGE_2D,
    GL_CMD_TEX_SUB_IMAGE_2D,
    GL_CMD_TEX_PARAMETERI,
    GL_CMD_GENERATE_MIPMAP,
    GL_CMD_PIXEL_STOREI,
    GL_CMD_STENCIL_FUNC,
    GL_CMD_STENCIL_MASK,
    GL_CMD_STENCIL_OP,
    GL_CMD_CLEAR_STENCIL,
    GL_CMD_DEPTH_MASK,
    GL_CMD_FRONT_FACE,
    GL_CMD_CULL_FACE,
    GL_CMD_POLYGON_OFFSET
} GLCommandType;

typedef struct {
    GLCommandType type;
    union {
        struct { GLenum mode; } begin;
        struct { GLfloat x, y, z, w; } vertex;
        struct { GLfloat r, g, b, a; } color;
        struct { GLfloat x, y, z; } normal;
        struct { GLfloat s, t, r, q; } texcoord;
        struct { GLenum cap; } enable;
        struct { GLenum cap; } disable;
        struct { GLenum mode; } matrix_mode;
        struct { GLfloat x, y, z; } translate;
        struct { GLfloat angle, x, y, z; } rotate;
        struct { GLfloat x, y, z; } scale;
        struct { GLenum light; GLenum pname; GLfloat params[4]; } light;
        struct { GLenum face; GLenum pname; GLfloat params[4]; } material;
        struct { GLuint list; } call_list;
        struct { GLint level; GLint internalformat; GLsizei width; GLsizei height; GLint border; GLenum format; GLenum type; void* pixel_payload; size_t payload_size; } tex_image_2d;
        struct { GLint level; GLint xoffset; GLint yoffset; GLsizei width; GLsizei height; GLenum format; GLenum type; void* pixel_payload; size_t payload_size; } tex_sub_image_2d;
        struct { GLenum pname; GLint param; } tex_param;
        struct { GLenum pname; GLint param; } pixel_store;
        struct { GLenum func; GLint ref; GLuint mask; } stencil_func;
        struct { GLuint mask; } stencil_mask;
        struct { GLenum sfail; GLenum dpfail; GLenum dppass; } stencil_op;
        struct { GLint s; } clear_stencil;
        struct { GLboolean flag; } depth_mask;
        struct { GLenum mode; } front_face;
        struct { GLenum mode; } cull_face;
        struct { GLfloat factor; GLfloat units; } polygon_offset;
    } data;
} GLCommand;

typedef struct {
    GLuint      id;
    bool        active;
    GLCommand*  commands;
    uint32_t    command_count;
    uint32_t    command_capacity;
} GLDisplayList;

GLuint    glGenLists(GLsizei range);
void      glNewList(GLuint list, GLenum mode);
void      glEndList(void);
void      glCallList(GLuint list);
void      glDeleteLists(GLuint list, GLsizei range);
GLboolean glIsList(GLuint list);

bool gl_display_list_is_compiling(GLContextState* state);
void gl_display_list_record_command(GLContextState* state, const GLCommand* cmd);

#ifdef __cplusplus
}
#endif

#endif // ATOMS_OS_GL_DISPLAY_LIST_H
