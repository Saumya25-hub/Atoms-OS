#ifndef ATOMS_OS_GL_TYPES_H
#define ATOMS_OS_GL_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

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
typedef double         GLdouble;
typedef float          GLclampf;
typedef double         GLclampd;
typedef void           GLvoid;

#define GL_TRUE  1
#define GL_FALSE 0

#ifdef __cplusplus
}
#endif

#endif // ATOMS_OS_GL_TYPES_H
