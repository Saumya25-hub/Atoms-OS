#ifndef BOS_OPENGL32_API_H
#define BOS_OPENGL32_API_H

#include "opengl32_types.h"

// Symbol Mapping to prevent collision with kernel gl.c
#define glClear gl32_glClear
#define glClearColor gl32_glClearColor
#define glViewport gl32_glViewport
#define glGenBuffers gl32_glGenBuffers
#define glBindBuffer gl32_glBindBuffer
#define glBufferData gl32_glBufferData
#define glMapBuffer gl32_glMapBuffer
#define glUnmapBuffer gl32_glUnmapBuffer
#define glGenTextures gl32_glGenTextures
#define glBindTexture gl32_glBindTexture
#define glTexImage2D gl32_glTexImage2D
#define glCreateShader gl32_glCreateShader
#define glCompileShader gl32_glCompileShader
#define glCreateProgram gl32_glCreateProgram
#define glAttachShader gl32_glAttachShader
#define glLinkProgram gl32_glLinkProgram
#define glUseProgram gl32_glUseProgram
#define glEnable gl32_glEnable
#define glDisable gl32_glDisable
#define glBlendFunc gl32_glBlendFunc
#define glDepthFunc gl32_glDepthFunc
#define glStencilFunc gl32_glStencilFunc
#define glDrawArrays gl32_glDrawArrays
#define glDrawElements gl32_glDrawElements
#define glGetError gl32_glGetError
#define glGetString gl32_glGetString
#define glGetIntegerv gl32_glGetIntegerv
#define glFlush gl32_glFlush
#define glFinish gl32_glFinish
#define glGenFramebuffers gl32_glGenFramebuffers
#define glBindFramebuffer gl32_glBindFramebuffer
#define glFramebufferTexture2D gl32_glFramebufferTexture2D

#ifdef __cplusplus
extern "C" {
#endif

int32_t OpenGLInitialize(void);
int32_t OpenGLShutdown(void);

// WGL Context & Presentation
HGLRC wglCreateContext(HANDLE hdc);
BOOL  wglDeleteContext(HGLRC hglrc);
BOOL  wglMakeCurrent(HANDLE hdc, HGLRC hglrc);
BOOL  wglSwapBuffers(HANDLE hdc);
void* wglGetProcAddress(LPCSTR lpszProc);

int   wglChoosePixelFormat(HANDLE hdc, const PIXELFORMATDESCRIPTOR* ppfd);
BOOL  wglSetPixelFormat(HANDLE hdc, int iPixelFormat, const PIXELFORMATDESCRIPTOR* ppfd);

// OpenGL Core API
void glClear(GLbitfield mask);
void glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
void glViewport(GLint x, GLint y, GLsizei width, GLsizei height);

void glGenBuffers(GLsizei n, GLuint* buffers);
void glBindBuffer(GLenum target, GLuint buffer);
void glBufferData(GLenum target, GLsizeiptr size, const void* data, GLenum usage);
void* glMapBuffer(GLenum target, GLenum access);
GLboolean glUnmapBuffer(GLenum target);

void glGenTextures(GLsizei n, GLuint* textures);
void glBindTexture(GLenum target, GLuint texture);
void glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void* pixels);

GLuint glCreateShader(GLenum type);
void   glCompileShader(GLuint shader);
GLuint glCreateProgram(void);
void   glAttachShader(GLuint program, GLuint shader);
void   glLinkProgram(GLuint program);
void   glUseProgram(GLuint program);

void glEnable(GLenum cap);
void glDisable(GLenum cap);
void glBlendFunc(GLenum sfactor, GLenum dfactor);
void glDepthFunc(GLenum func);
void glStencilFunc(GLenum func, GLint ref, GLuint mask);

void glDrawArrays(GLenum mode, GLint first, GLsizei count);
void glDrawElements(GLenum mode, GLsizei count, GLenum type, const void* indices);

GLenum glGetError(void);
const GLubyte* glGetString(GLenum name);
void glGetIntegerv(GLenum pname, GLint* params);

void glGenFramebuffers(GLsizei n, GLuint* framebuffers);
void glBindFramebuffer(GLenum target, GLuint framebuffer);
void glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);

void glFlush(void);
void glFinish(void);

void opengl32_run_certification_suite(void);

#ifdef __cplusplus
}
#endif

#endif // BOS_OPENGL32_API_H

