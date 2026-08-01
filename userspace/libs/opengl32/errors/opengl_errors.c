#include "../include/opengl32_api.h"

static GLenum g_last_error = GL_NO_ERROR;

GLenum glGetError(void) {
    GLenum err = g_last_error;
    g_last_error = GL_NO_ERROR;
    return err;
}

void OpenGLSetError(GLenum err) {
    g_last_error = err;
}
