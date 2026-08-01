#include "../include/opengl32_api.h"

extern void* kmalloc(size_t size);

static GLuint g_buf_counter = 1;

void glGenBuffers(GLsizei n, GLuint* buffers) {
    if (!buffers) return;
    for (GLsizei i = 0; i < n; i++) {
        buffers[i] = g_buf_counter++;
    }
}

void glBindBuffer(GLenum target, GLuint buffer) {
    (void)target; (void)buffer;
}

void glBufferData(GLenum target, GLsizeiptr size, const void* data, GLenum usage) {
    (void)target; (void)size; (void)data; (void)usage;
}

void* glMapBuffer(GLenum target, GLenum access) {
    (void)target; (void)access;
    return kmalloc(4096);
}

GLboolean glUnmapBuffer(GLenum target) {
    (void)target;
    return GL_TRUE;
}
