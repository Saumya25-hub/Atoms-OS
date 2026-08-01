#include "../include/opengl32_api.h"

static GLuint g_tex_counter = 1;

void glGenTextures(GLsizei n, GLuint* textures) {
    if (!textures) return;
    for (GLsizei i = 0; i < n; i++) {
        textures[i] = g_tex_counter++;
    }
}

void glBindTexture(GLenum target, GLuint texture) {
    (void)target; (void)texture;
}

void glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void* pixels) {
    (void)target; (void)level; (void)internalformat; (void)width; (void)height; (void)border; (void)format; (void)type; (void)pixels;
}
