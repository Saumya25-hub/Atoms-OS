#ifndef ATOMS_OS_GL_MATH_H
#define ATOMS_OS_GL_MATH_H

#include "kernel/graphics/gl/gl_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GL_PI 3.14159265358979323846f

typedef struct {
    GLfloat m[16]; // Column-major 4x4 matrix
} GLMatrix4x4;

typedef struct {
    GLfloat x, y, z, w;
} GLVec4;

/* --- Floating-point Math Helpers --- */
GLfloat gl_fabsf(GLfloat x);
GLfloat gl_sqrtf(GLfloat x);
GLfloat gl_sinf(GLfloat rad);
GLfloat gl_cosf(GLfloat rad);
GLfloat gl_tanf(GLfloat rad);
GLfloat gl_powf(GLfloat base, GLfloat exp);

/* --- Matrix Operations --- */
void gl_matrix_identity(GLMatrix4x4* out);
void gl_matrix_copy(GLMatrix4x4* out, const GLMatrix4x4* in);
void gl_matrix_multiply(GLMatrix4x4* out, const GLMatrix4x4* A, const GLMatrix4x4* B);
void gl_matrix_translate(GLMatrix4x4* out, const GLMatrix4x4* in, GLfloat tx, GLfloat ty, GLfloat tz);
void gl_matrix_scale(GLMatrix4x4* out, const GLMatrix4x4* in, GLfloat sx, GLfloat sy, GLfloat sz);
void gl_matrix_rotate(GLMatrix4x4* out, const GLMatrix4x4* in, GLfloat angle_deg, GLfloat x, GLfloat y, GLfloat z);
void gl_matrix_ortho(GLMatrix4x4* out, GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar);
void gl_matrix_frustum(GLMatrix4x4* out, GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar);

/* --- 3x3 Normal Matrix & Vector Operations --- */
bool gl_matrix_inverse_transpose_3x3(float out[9], const GLMatrix4x4* M);
void gl_transform_normal3(const float inv_trans[9], const float in_norm[3], float out_norm[3]);
void gl_vec3_normalize(float v[3]);
float gl_vec3_dot(const float a[3], const float b[3]);
void gl_vec3_cross(float out[3], const float a[3], const float b[3]);

/* --- Vector Transformation --- */
GLVec4 gl_transform_point4(const GLMatrix4x4* M, GLVec4 v);

#ifdef __cplusplus
}
#endif

#endif // ATOMS_OS_GL_MATH_H
