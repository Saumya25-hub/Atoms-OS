#ifndef ATOMS_OS_GL_TEXTURE_H
#define ATOMS_OS_GL_TEXTURE_H

#include "kernel/graphics/gl/gl_types.h"
#include "kernel/graphics/gl/gl_constants.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    bool      defined;
    GLsizei   width;
    GLsizei   height;
    GLenum    internal_format;
    GLenum    source_format;
    GLenum    source_type;
    uint8_t*  pixel_data; // Canonical RGBA8 buffer (width * height * 4)
    size_t    data_size;
} GLTextureLevel;

typedef struct {
    GLuint          id;
    GLTextureLevel  levels[GL_MAX_MIP_LEVELS];

    GLenum          wrap_s;
    GLenum          wrap_t;

    GLenum          min_filter;
    GLenum          mag_filter;

    bool            defined;
    bool            allocated;
    uint32_t        generation;
} GLTextureObject;

void gl_texture_init_object(GLTextureObject* tex, GLuint id);
void gl_texture_free_object(GLTextureObject* tex);
bool gl_texture_upload_image(GLTextureObject* tex, GLint level, GLint internalformat, GLsizei w, GLsizei h, GLint border, GLenum format, GLenum type, const void* pixels, GLint unpack_alignment);
bool gl_texture_sub_upload_image(GLTextureObject* tex, GLint level, GLint xoffset, GLint yoffset, GLsizei w, GLsizei h, GLenum format, GLenum type, const void* pixels, GLint unpack_alignment);
bool gl_texture_generate_mipmaps(GLTextureObject* tex);
bool gl_texture_is_complete(const GLTextureObject* tex);
bool gl_texture_set_parameter(GLTextureObject* tex, GLenum pname, GLint param);

#ifdef __cplusplus
}
#endif

#endif // ATOMS_OS_GL_TEXTURE_H
