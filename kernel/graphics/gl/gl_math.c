#include "kernel/graphics/gl/gl_math.h"

GLfloat gl_fabsf(GLfloat x) {
    return (x < 0.0f) ? -x : x;
}

GLfloat gl_sqrtf(GLfloat x) {
    if (x <= 0.0f) return 0.0f;
    GLfloat val = x;
    for (int i = 0; i < 10; i++) {
        val = 0.5f * (val + x / val);
    }
    return val;
}

GLfloat gl_sinf(GLfloat rad) {
    while (rad > GL_PI)  rad -= 2.0f * GL_PI;
    while (rad < -GL_PI) rad += 2.0f * GL_PI;

    GLfloat x2 = rad * rad;
    GLfloat x3 = rad * x2;
    GLfloat x5 = x3 * x2;
    GLfloat x7 = x5 * x2;
    GLfloat x9 = x7 * x2;

    return rad - (x3 / 6.0f) + (x5 / 120.0f) - (x7 / 5040.0f) + (x9 / 362880.0f);
}

GLfloat gl_cosf(GLfloat rad) {
    while (rad > GL_PI)  rad -= 2.0f * GL_PI;
    while (rad < -GL_PI) rad += 2.0f * GL_PI;

    GLfloat x2  = rad * rad;
    GLfloat x4  = x2 * x2;
    GLfloat x6  = x4 * x2;
    GLfloat x8  = x6 * x2;
    GLfloat x10 = x8 * x2;

    return 1.0f - (x2 / 2.0f) + (x4 / 24.0f) - (x6 / 720.0f) + (x8 / 40320.0f) - (x10 / 3628800.0f);
}

GLfloat gl_tanf(GLfloat rad) {
    GLfloat c = gl_cosf(rad);
    if (gl_fabsf(c) < 1e-7f) return 0.0f;
    return gl_sinf(rad) / c;
}

GLfloat gl_powf(GLfloat base, GLfloat exp) {
    if (base <= 0.0f) return 0.0f;
    if (exp == 0.0f) return 1.0f;
    if (exp == 1.0f) return base;

    // Integer exponent fast path
    int iexp = (int)exp;
    if ((float)iexp == exp && iexp >= 1 && iexp <= 128) {
        float res = 1.0f;
        float b = base;
        while (iexp > 0) {
            if (iexp & 1) res *= b;
            b *= b;
            iexp >>= 1;
        }
        return res;
    }

    // Binary exponentiation fallback for non-integer
    float res = 1.0f;
    float current_power = base;
    int p = (int)exp;
    if (p < 0) p = 0;
    if (p > 128) p = 128;

    while (p > 0) {
        if (p & 1) res *= current_power;
        current_power *= current_power;
        p >>= 1;
    }
    return res;
}

void gl_matrix_identity(GLMatrix4x4* out) {
    if (!out) return;
    for (int i = 0; i < 16; i++) {
        out->m[i] = 0.0f;
    }
    out->m[0]  = 1.0f;
    out->m[5]  = 1.0f;
    out->m[10] = 1.0f;
    out->m[15] = 1.0f;
}

void gl_matrix_copy(GLMatrix4x4* out, const GLMatrix4x4* in) {
    if (!out || !in) return;
    for (int i = 0; i < 16; i++) {
        out->m[i] = in->m[i];
    }
}

void gl_matrix_multiply(GLMatrix4x4* out, const GLMatrix4x4* A, const GLMatrix4x4* B) {
    if (!out || !A || !B) return;
    GLMatrix4x4 res;
    for (int col = 0; col < 4; col++) {
        for (int row = 0; row < 4; row++) {
            res.m[col * 4 + row] = 
                A->m[0 * 4 + row] * B->m[col * 4 + 0] +
                A->m[1 * 4 + row] * B->m[col * 4 + 1] +
                A->m[2 * 4 + row] * B->m[col * 4 + 2] +
                A->m[3 * 4 + row] * B->m[col * 4 + 3];
        }
    }
    gl_matrix_copy(out, &res);
}

void gl_matrix_translate(GLMatrix4x4* out, const GLMatrix4x4* in, GLfloat tx, GLfloat ty, GLfloat tz) {
    if (!out || !in) return;
    GLMatrix4x4 T;
    gl_matrix_identity(&T);
    T.m[12] = tx;
    T.m[13] = ty;
    T.m[14] = tz;
    gl_matrix_multiply(out, in, &T);
}

void gl_matrix_scale(GLMatrix4x4* out, const GLMatrix4x4* in, GLfloat sx, GLfloat sy, GLfloat sz) {
    if (!out || !in) return;
    GLMatrix4x4 S;
    gl_matrix_identity(&S);
    S.m[0]  = sx;
    S.m[5]  = sy;
    S.m[10] = sz;
    gl_matrix_multiply(out, in, &S);
}

void gl_matrix_rotate(GLMatrix4x4* out, const GLMatrix4x4* in, GLfloat angle_deg, GLfloat x, GLfloat y, GLfloat z) {
    if (!out || !in) return;

    GLfloat len = gl_sqrtf(x * x + y * y + z * z);
    if (len < 1e-7f) return;

    x /= len;
    y /= len;
    z /= len;

    GLfloat rad = angle_deg * (GL_PI / 180.0f);
    GLfloat c = gl_cosf(rad);
    GLfloat s = gl_sinf(rad);
    GLfloat omc = 1.0f - c;

    GLMatrix4x4 R;
    gl_matrix_identity(&R);

    R.m[0]  = x * x * omc + c;
    R.m[1]  = y * x * omc + z * s;
    R.m[2]  = x * z * omc - y * s;

    R.m[4]  = x * y * omc - z * s;
    R.m[5]  = y * y * omc + c;
    R.m[6]  = y * z * omc + x * s;

    R.m[8]  = x * z * omc + y * s;
    R.m[9]  = y * z * omc - x * s;
    R.m[10] = z * z * omc + c;

    gl_matrix_multiply(out, in, &R);
}

void gl_matrix_ortho(GLMatrix4x4* out, GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar) {
    if (!out || left == right || bottom == top || zNear == zFar) return;

    GLMatrix4x4 O;
    gl_matrix_identity(&O);

    O.m[0]  = 2.0f / (right - left);
    O.m[5]  = 2.0f / (top - bottom);
    O.m[10] = -2.0f / (zFar - zNear);

    O.m[12] = -(right + left) / (right - left);
    O.m[13] = -(top + bottom) / (top - bottom);
    O.m[14] = -(zFar + zNear) / (zFar - zNear);

    gl_matrix_multiply(out, out, &O);
}

void gl_matrix_frustum(GLMatrix4x4* out, GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar) {
    if (!out || left == right || bottom == top || zNear <= 0.0f || zFar <= 0.0f || zNear >= zFar) return;

    GLMatrix4x4 F;
    gl_matrix_identity(&F);

    F.m[0]  = (2.0f * zNear) / (right - left);
    F.m[5]  = (2.0f * zNear) / (top - bottom);
    F.m[8]  = (right + left) / (right - left);
    F.m[9]  = (top + bottom) / (top - bottom);
    F.m[10] = -(zFar + zNear) / (zFar - zNear);
    F.m[11] = -1.0f;
    F.m[14] = -(2.0f * zFar * zNear) / (zFar - zNear);
    F.m[15] = 0.0f;

    gl_matrix_multiply(out, out, &F);
}

bool gl_matrix_inverse_transpose_3x3(float out[9], const GLMatrix4x4* M) {
    if (!out || !M) return false;

    // Extract upper 3x3 (Column-major)
    float a = M->m[0], b = M->m[4], c = M->m[8];
    float d = M->m[1], e = M->m[5], f = M->m[9];
    float g = M->m[2], h = M->m[6], i = M->m[10];

    float c00 = e * i - f * h;
    float c01 = -(d * i - f * g);
    float c02 = d * h - e * g;

    float det = a * c00 + b * c01 + c * c02;

    if (gl_fabsf(det) < 1e-7f) {
        // Singular matrix fallback to identity 3x3
        out[0] = 1.0f; out[1] = 0.0f; out[2] = 0.0f;
        out[3] = 0.0f; out[4] = 1.0f; out[5] = 0.0f;
        out[6] = 0.0f; out[7] = 0.0f; out[8] = 1.0f;
        return false;
    }

    float inv_det = 1.0f / det;

    float c10 = -(b * i - c * h);
    float c11 = a * i - c * g;
    float c12 = -(a * h - b * g);

    float c20 = b * f - c * e;
    float c21 = -(a * f - c * d);
    float c22 = a * e - b * d;

    // Inverse Transpose is Cofactor Matrix multiplied by 1/det
    out[0] = c00 * inv_det; out[1] = c01 * inv_det; out[2] = c02 * inv_det;
    out[3] = c10 * inv_det; out[4] = c11 * inv_det; out[5] = c12 * inv_det;
    out[6] = c20 * inv_det; out[7] = c21 * inv_det; out[8] = c22 * inv_det;

    return true;
}

void gl_transform_normal3(const float inv_trans[9], const float in_norm[3], float out_norm[3]) {
    if (!inv_trans || !in_norm || !out_norm) return;

    out_norm[0] = inv_trans[0] * in_norm[0] + inv_trans[3] * in_norm[1] + inv_trans[6] * in_norm[2];
    out_norm[1] = inv_trans[1] * in_norm[0] + inv_trans[4] * in_norm[1] + inv_trans[7] * in_norm[2];
    out_norm[2] = inv_trans[2] * in_norm[0] + inv_trans[5] * in_norm[1] + inv_trans[8] * in_norm[2];
}

void gl_vec3_normalize(float v[3]) {
    if (!v) return;
    float len = gl_sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    if (len > 1e-7f) {
        float inv_len = 1.0f / len;
        v[0] *= inv_len;
        v[1] *= inv_len;
        v[2] *= inv_len;
    } else {
        v[0] = 0.0f; v[1] = 0.0f; v[2] = 1.0f;
    }
}

float gl_vec3_dot(const float a[3], const float b[3]) {
    if (!a || !b) return 0.0f;
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

void gl_vec3_cross(float out[3], const float a[3], const float b[3]) {
    if (!out || !a || !b) return;
    out[0] = a[1] * b[2] - a[2] * b[1];
    out[1] = a[2] * b[0] - a[0] * b[2];
    out[2] = a[0] * b[1] - a[1] * b[0];
}

GLVec4 gl_transform_point4(const GLMatrix4x4* M, GLVec4 v) {
    GLVec4 res;
    if (!M) {
        res.x = 0; res.y = 0; res.z = 0; res.w = 0;
        return res;
    }
    res.x = M->m[0] * v.x + M->m[4] * v.y + M->m[8]  * v.z + M->m[12] * v.w;
    res.y = M->m[1] * v.x + M->m[5] * v.y + M->m[9]  * v.z + M->m[13] * v.w;
    res.z = M->m[2] * v.x + M->m[6] * v.y + M->m[10] * v.z + M->m[14] * v.w;
    res.w = M->m[3] * v.x + M->m[7] * v.y + M->m[11] * v.z + M->m[15] * v.w;
    return res;
}
