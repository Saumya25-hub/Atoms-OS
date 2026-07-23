#ifndef ATOMS_OS_GL_H
#define ATOMS_OS_GL_H

#include "kernel/graphics/gl/gl_types.h"
#include "kernel/graphics/gl/gl_constants.h"

#ifdef __cplusplus
extern "C" {
#endif

/* --- Error Handling --- */
GLenum glGetError(void);

/* --- Capability & State Control --- */
void glEnable(GLenum cap);
void glDisable(GLenum cap);
GLboolean glIsEnabled(GLenum cap);

/* --- Depth & Clear --- */
void glViewport(GLint x, GLint y, GLsizei width, GLsizei height);
void glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
void glClearDepth(GLclampf depth);
void glClear(GLbitfield mask);

void glDepthFunc(GLenum func);
void glDepthMask(GLboolean flag);

/* --- Phase 6 Fragment & Rasterization APIs --- */
void glAlphaFunc(GLenum func, GLclampf ref);
void glBlendFunc(GLenum sfactor, GLenum dfactor);

void glFogf(GLenum pname, GLfloat param);
void glFogfv(GLenum pname, const GLfloat *params);
void glFogi(GLenum pname, GLint param);
void glFogiv(GLenum pname, const GLint *params);

void glScissor(GLint x, GLint y, GLsizei width, GLsizei height);
void glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
void glDepthRange(GLclampd nearVal, GLclampd farVal);

void glPointSize(GLfloat size);
void glLineWidth(GLfloat width);
void glPolygonMode(GLenum face, GLenum mode);

/* --- Face Culling --- */
void glCullFace(GLenum mode);
void glFrontFace(GLenum mode);

/* --- Shading & Color Material --- */
void glShadeModel(GLenum mode);
void glColorMaterial(GLenum face, GLenum mode);

/* --- Normals --- */
void glNormal3f(GLfloat nx, GLfloat ny, GLfloat nz);
void glNormal3fv(const GLfloat *v);
void glNormal3d(GLdouble nx, GLdouble ny, GLdouble nz);
void glNormal3dv(const GLdouble *v);

/* --- Material Properties --- */
void glMaterialf(GLenum face, GLenum pname, GLfloat param);
void glMaterialfv(GLenum face, GLenum pname, const GLfloat *params);

/* --- Lighting & Light Sources --- */
void glLightfv(GLenum light, GLenum pname, const GLfloat *params);
void glLightf(GLenum light, GLenum pname, GLfloat param);
void glLightModelf(GLenum pname, GLfloat param);
void glLightModelfv(GLenum pname, const GLfloat *params);

/* --- Texture Object & Sampling API --- */
void glGenTextures(GLsizei n, GLuint *textures);
void glDeleteTextures(GLsizei n, const GLuint *textures);
void glBindTexture(GLenum target, GLuint texture);
void glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
void glTexParameteri(GLenum target, GLenum pname, GLint param);
void glTexEnvi(GLenum target, GLenum pname, GLint param);

/* --- Texture Coordinates --- */
void glTexCoord1f(GLfloat s);
void glTexCoord2f(GLfloat s, GLfloat t);
void glTexCoord3f(GLfloat s, GLfloat t, GLfloat r);
void glTexCoord4f(GLfloat s, GLfloat t, GLfloat r, GLfloat q);
void glTexCoord1fv(const GLfloat *v);
void glTexCoord2fv(const GLfloat *v);
void glTexCoord3fv(const GLfloat *v);
void glTexCoord4fv(const GLfloat *v);

/* --- Matrix Operations --- */
void glMatrixMode(GLenum mode);
void glLoadIdentity(void);
void glLoadMatrixf(const GLfloat *m);
void glMultMatrixf(const GLfloat *m);
void glPushMatrix(void);
void glPopMatrix(void);

void glTranslatef(GLfloat x, GLfloat y, GLfloat z);
void glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
void glScalef(GLfloat x, GLfloat y, GLfloat z);
void glOrtho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
void glFrustum(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);

/* --- Immediate Mode Primitives --- */
void glBegin(GLenum mode);
void glEnd(void);

void glVertex2f(GLfloat x, GLfloat y);
void glVertex3f(GLfloat x, GLfloat y, GLfloat z);
void glVertex4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w);
void glVertex2fv(const GLfloat *v);
void glVertex3fv(const GLfloat *v);
void glVertex4fv(const GLfloat *v);

void glColor3f(GLfloat red, GLfloat green, GLfloat blue);
void glColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
void glColor3ub(GLubyte red, GLubyte green, GLubyte blue);
void glColor4ub(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha);
void glColor4fv(const GLfloat *v);

/* --- Client Vertex Arrays & Draw Operations --- */
void glEnableClientState(GLenum cap);
void glDisableClientState(GLenum cap);

void glVertexPointer(GLint size, GLenum type, GLsizei stride, const void* pointer);
void glColorPointer(GLint size, GLenum type, GLsizei stride, const void* pointer);
void glNormalPointer(GLenum type, GLsizei stride, const void* pointer);
void glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const void* pointer);

void glDrawArrays(GLenum mode, GLint first, GLsizei count);
void glDrawElements(GLenum mode, GLsizei count, GLenum type, const void* indices);

/* --- Phase 8 Pixel Storage, Sub-Images, Mipmapping & Readback --- */
void glPixelStorei(GLenum pname, GLint param);
void glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels);
void glGenerateMipmap(GLenum target);
void glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void *pixels);
void glCopyTexImage2D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
void glCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);

/* --- Phase 9 Stencil, Polygon Offset & Raster State APIs --- */
void glStencilFunc(GLenum func, GLint ref, GLuint mask);
void glStencilMask(GLuint mask);
void glStencilOp(GLenum sfail, GLenum dpfail, GLenum dppass);
void glClearStencil(GLint s);
void glPolygonOffset(GLfloat factor, GLfloat units);

/* --- Phase 10 Framebuffer & Renderbuffer Object APIs --- */
void glGenFramebuffers(GLsizei n, GLuint *framebuffers);
void glDeleteFramebuffers(GLsizei n, const GLuint *framebuffers);
void glBindFramebuffer(GLenum target, GLuint framebuffer);
GLboolean glIsFramebuffer(GLuint framebuffer);
GLenum glCheckFramebufferStatus(GLenum target);
void glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
void glFramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);

void glGenRenderbuffers(GLsizei n, GLuint *renderbuffers);
void glDeleteRenderbuffers(GLsizei n, const GLuint *renderbuffers);
void glBindRenderbuffer(GLenum target, GLuint renderbuffer);
GLboolean glIsRenderbuffer(GLuint renderbuffer);
void glRenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);

/* --- Display Lists --- */
GLuint glGenLists(GLsizei range);
void glNewList(GLuint list, GLenum mode);
void glEndList(void);
void glCallList(GLuint list);
void glDeleteLists(GLuint list, GLsizei range);

#ifdef __cplusplus
}
#endif

#endif // ATOMS_OS_GL_H
