#ifndef OPENGL32_H
#define OPENGL32_H

#include <stdint.h>
#include <stdbool.h>

// Standard OpenGL Types
typedef unsigned int   GLenum;
typedef unsigned char  GLboolean;
typedef unsigned int   GLbitfield;
typedef signed char    GLbyte;
typedef short          GLshort;
typedef int            GLint;
typedef int            GLsizei;
typedef unsigned char  GLubyte;
typedef unsigned short GLushort;
typedef unsigned int   GLuint;
typedef float          GLfloat;
typedef float          GLclampf;
typedef double         GLdouble;
typedef void           GLvoid;

// Standard Constants
#define GL_COLOR_BUFFER_BIT   0x00004000
#define GL_DEPTH_BUFFER_BIT   0x00000100
#define GL_STENCIL_BUFFER_BIT 0x00000400

#define GL_TRIANGLES          0x0004
#define GL_LINES              0x0001
#define GL_POINTS             0x0000
#define GL_QUADS              0x0007

#define GL_TEXTURE_2D         0x0DE1
#define GL_RGBA               0x1908
#define GL_RGB                0x1907
#define GL_UNSIGNED_BYTE      0x1401

#define GL_DEPTH_TEST         0x0B71
#define GL_BLEND              0x0BE2
#define GL_CULL_FACE          0x0B44

#define GL_VERTEX_SHADER      0x8B31
#define GL_FRAGMENT_SHADER    0x8B30

// Classic OpenGL Exports
void glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
void glClearDepth(GLclampf depth);
void glClear(GLbitfield mask);
void glViewport(GLint x, GLint y, GLsizei width, GLsizei height);
void glEnable(GLenum cap);
void glDisable(GLenum cap);

void glBegin(GLenum mode);
void glEnd(void);
void glVertex2f(GLfloat x, GLfloat y);
void glVertex3f(GLfloat x, GLfloat y, GLfloat z);
void glColor3f(GLfloat red, GLfloat green, GLfloat blue);
void glColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
void glTexCoord2f(GLfloat s, GLfloat t);

void glGenTextures(GLsizei n, GLuint* textures);
void glBindTexture(GLenum target, GLuint texture);
void glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid* pixels);
void glDeleteTextures(GLsizei n, const GLuint* textures);

GLuint glCreateShader(GLenum type);
void glShaderSource(GLuint shader, GLsizei count, const char** string, const GLint* length);
void glCompileShader(GLuint shader);
GLuint glCreateProgram(void);
void glAttachShader(GLuint program, GLuint shader);
void glLinkProgram(GLuint program);
void glUseProgram(GLuint program);
void glDeleteShader(GLuint shader);

void glDrawArrays(GLenum mode, GLint first, GLsizei count);

// WGL / EGL Window System Swap Buffers
void SwapBuffers(void);
void wglSwapBuffers(void);
void eglSwapBuffers(void);

#endif // OPENGL32_H
