#include "../include/opengl32_api.h"

static GLuint g_fbo_counter = 1;

void glGenFramebuffers(GLsizei n, GLuint* framebuffers) {
    if (!framebuffers) return;
    for (GLsizei i = 0; i < n; i++) {
        framebuffers[i] = g_fbo_counter++;
    }
}

void glBindFramebuffer(GLenum target, GLuint framebuffer) {
    (void)target; (void)framebuffer;
}
