#include "../include/opengl32_api.h"

const GLubyte* glGetString(GLenum name) {
    switch (name) {
        case GL_VENDOR: return (const GLubyte*)"ATOMS OS Corporation";
        case GL_RENDERER: return (const GLubyte*)"ATOMS AGP V1.0 Acceleration Engine";
        case GL_VERSION: return (const GLubyte*)"3.2 ATOMS OS OpenGL32.sll V1.0";
        case GL_EXTENSIONS: return (const GLubyte*)"GL_ARB_vbo GL_ARB_shader_objects GL_EXT_texture3D";
        default: return (const GLubyte*)"";
    }
}
