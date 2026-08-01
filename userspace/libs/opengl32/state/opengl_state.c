#include "../include/opengl32_api.h"

void glGetIntegerv(GLenum pname, GLint* params) {
    if (!params) return;
    switch (pname) {
        case GL_VIEWPORT:
            params[0] = 0; params[1] = 0; params[2] = 1024; params[3] = 768;
            break;
        default:
            params[0] = 0;
            break;
    }
}
