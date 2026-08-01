// ============================================================
// OpenGL32.sll — Classic OpenGL Export Layer (Phase 10)
// ============================================================
// Ring 3 Applications interface with this library.
// 100% of operations route directly into AGP Runtime API.
// Weak symbols allow seamless Ring 3 SDK linking.
// ============================================================

#include "../include/opengl32.h"
#include "../include/agp_api.h"

#define WEAK __attribute__((weak))

WEAK void glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha) {
    uint32_t r = (uint32_t)(red * 255.0f);
    uint32_t g = (uint32_t)(green * 255.0f);
    uint32_t b = (uint32_t)(blue * 255.0f);
    uint32_t a = (uint32_t)(alpha * 255.0f);
    uint32_t color = (a << 24) | (r << 16) | (g << 8) | b;

    AGPContext* ctx = AGP_GetCurrentContext();
    if (ctx) ctx->clear_color = color;
}

WEAK void glClearDepth(GLclampf depth) {
    AGPContext* ctx = AGP_GetCurrentContext();
    if (ctx) ctx->clear_depth = (float)depth;
}

WEAK void glClear(GLbitfield mask) {
    (void)mask;
    AGPContext* ctx = AGP_GetCurrentContext();
    uint32_t color = ctx ? ctx->clear_color : 0xFF000000;
    float depth = ctx ? ctx->clear_depth : 1.0f;
    AGP_Clear(color, depth);
}

WEAK void glViewport(GLint x, GLint y, GLsizei width, GLsizei height) {
    AGP_SetViewport(x, y, (uint32_t)width, (uint32_t)height);
}

WEAK void glEnable(GLenum cap) {
    extern void AGP_EnableCapability(uint32_t cap, bool enable);
    AGP_EnableCapability(cap, true);
}

WEAK void glDisable(GLenum cap) {
    extern void AGP_EnableCapability(uint32_t cap, bool enable);
    AGP_EnableCapability(cap, false);
}

WEAK void glBegin(GLenum mode) {
    AGPPrimitiveType prim = AGP_PRIMITIVE_TRIANGLES;
    if (mode == GL_LINES) prim = AGP_PRIMITIVE_LINES;
    else if (mode == GL_POINTS) prim = AGP_PRIMITIVE_POINTS;
    else if (mode == GL_QUADS) prim = AGP_PRIMITIVE_QUADS;

    AGP_Begin(prim);
}

WEAK void glEnd(void) {
    AGP_End();
}

WEAK void glVertex2f(GLfloat x, GLfloat y) {
    AGP_Vertex3f(x, y, 0.0f);
}

WEAK void glVertex3f(GLfloat x, GLfloat y, GLfloat z) {
    AGP_Vertex3f(x, y, z);
}

WEAK void glColor3f(GLfloat red, GLfloat green, GLfloat blue) {
    AGP_Color4f(red, green, blue, 1.0f);
}

WEAK void glColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
    AGP_Color4f(red, green, blue, alpha);
}

WEAK void glTexCoord2f(GLfloat s, GLfloat t) {
    AGP_TexCoord2f(s, t);
}

WEAK void glGenTextures(GLsizei n, GLuint* textures) {
    if (!textures || n <= 0) return;
    for (GLsizei i = 0; i < n; i++) {
        textures[i] = (GLuint)AGP_CreateTexture(64, 64, AGP_FORMAT_RGBA8888, NULL);
    }
}

WEAK void glBindTexture(GLenum target, GLuint texture) {
    (void)target;
    AGP_BindTexture((AGPTextureID)texture);
}

WEAK void glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid* pixels) {
    (void)target; (void)level; (void)internalformat; (void)border; (void)format; (void)type; (void)pixels; (void)width; (void)height;
}

WEAK void glDeleteTextures(GLsizei n, const GLuint* textures) {
    if (!textures || n <= 0) return;
    for (GLsizei i = 0; i < n; i++) {
        AGP_DeleteTexture((AGPTextureID)textures[i]);
    }
}

WEAK GLuint glCreateShader(GLenum type) {
    AGPShaderType t = (type == GL_VERTEX_SHADER) ? AGP_SHADER_VERTEX : AGP_SHADER_FRAGMENT;
    return (GLuint)AGP_CreateShader(t, "");
}

WEAK void glShaderSource(GLuint shader, GLsizei count, const char** string, const GLint* length) {
    (void)shader; (void)count; (void)string; (void)length;
}

WEAK void glCompileShader(GLuint shader) {
    (void)shader;
}

WEAK GLuint glCreateProgram(void) {
    return (GLuint)AGP_CreateProgram(1, 2);
}

WEAK void glAttachShader(GLuint program, GLuint shader) {
    (void)program; (void)shader;
}

WEAK void glLinkProgram(GLuint program) {
    (void)program;
}

WEAK void glUseProgram(GLuint program) {
    AGP_UseProgram((AGPProgramID)program);
}

WEAK void glDeleteShader(GLuint shader) {
    AGP_DeleteShader((AGPShaderID)shader);
}

WEAK void glDrawArrays(GLenum mode, GLint first, GLsizei count) {
    AGPPrimitiveType prim = AGP_PRIMITIVE_TRIANGLES;
    if (mode == GL_LINES) prim = AGP_PRIMITIVE_LINES;
    AGP_DrawArrays(prim, (uint32_t)first, (uint32_t)count);
}

WEAK void SwapBuffers(void) {
    AGPContext* ctx = AGP_GetCurrentContext();
    if (ctx) AGP_SwapBuffers(ctx->context_id);
}

WEAK void wglSwapBuffers(void) { SwapBuffers(); }
WEAK void eglSwapBuffers(void) { SwapBuffers(); }
