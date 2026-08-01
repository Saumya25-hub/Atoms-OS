#include "../include/opengl32_api.h"

void glClear(GLbitfield mask) {
    (void)mask;
}

void glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha) {
    (void)red; (void)green; (void)blue; (void)alpha;
}

void glViewport(GLint x, GLint y, GLsizei width, GLsizei height) {
    (void)x; (void)y; (void)width; (void)height;
}

void glEnable(GLenum cap) { (void)cap; }
void glDisable(GLenum cap) { (void)cap; }
void glBlendFunc(GLenum sfactor, GLenum dfactor) { (void)sfactor; (void)dfactor; }
void glDepthFunc(GLenum func) { (void)func; }
void glStencilFunc(GLenum func, GLint ref, GLuint mask) { (void)func; (void)ref; (void)mask; }
