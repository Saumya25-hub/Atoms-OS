#include "../include/bishop_vector.h"
#include "../include/bishop_basic.h"
#include "../internal/bishop_internal.h"

void bishop_vec3_create(BishopVec3* out, double x, double y, double z) {
    if (!out) return;
    out->x = x; out->y = y; out->z = z;
}

void bishop_vec3_zero(BishopVec3* out) {
    if (!out) return;
    out->x = 0.0; out->y = 0.0; out->z = 0.0;
}

void bishop_vec3_add(const BishopVec3* a, const BishopVec3* b, BishopVec3* out) {
    if (!a || !b || !out) return;
    out->x = a->x + b->x; out->y = a->y + b->y; out->z = a->z + b->z;
}

void bishop_vec3_sub(const BishopVec3* a, const BishopVec3* b, BishopVec3* out) {
    if (!a || !b || !out) return;
    out->x = a->x - b->x; out->y = a->y - b->y; out->z = a->z - b->z;
}

void bishop_vec3_scale(const BishopVec3* v, double s, BishopVec3* out) {
    if (!v || !out) return;
    out->x = v->x * s; out->y = v->y * s; out->z = v->z * s;
}

BishopError bishop_vec3_dot(const BishopVec3* a, const BishopVec3* b, double* out) {
    if (!a || !b || !out) return BISHOP_ERR_INVALID_ARG;
    *out = a->x * b->x + a->y * b->y + a->z * b->z;
    return BISHOP_OK;
}

void bishop_vec3_cross(const BishopVec3* a, const BishopVec3* b, BishopVec3* out) {
    if (!a || !b || !out) return;
    out->x = a->y * b->z - a->z * b->y;
    out->y = a->z * b->x - a->x * b->z;
    out->z = a->x * b->y - a->y * b->x;
}

BishopError bishop_vec3_magnitude(const BishopVec3* v, double* out) {
    if (!v || !out) return BISHOP_ERR_INVALID_ARG;
    double mag_sq = v->x * v->x + v->y * v->y + v->z * v->z;
    return bishop_sqrt(mag_sq, out);
}

BishopError bishop_vec3_normalize(const BishopVec3* v, BishopVec3* out) {
    if (!v || !out) return BISHOP_ERR_INVALID_ARG;
    double mag;
    BishopError e = bishop_vec3_magnitude(v, &mag);
    if (e != BISHOP_OK) return e;
    if (mag < BISHOP_EPSILON) { bishop_set_error(BISHOP_ERR_DIV_ZERO); return BISHOP_ERR_DIV_ZERO; }
    double inv = 1.0 / mag;
    out->x = v->x * inv; out->y = v->y * inv; out->z = v->z * inv;
    return BISHOP_OK;
}

BishopError bishop_vec3_distance(const BishopVec3* a, const BishopVec3* b, double* out) {
    if (!a || !b || !out) return BISHOP_ERR_INVALID_ARG;
    BishopVec3 diff;
    bishop_vec3_sub(a, b, &diff);
    return bishop_vec3_magnitude(&diff, out);
}

static void bvec_print_num(int64_t val) {
    if (val < 0) { bos_print("-"); val = -val; }
    char buf[20]; int i = 18; buf[19] = '\0';
    if (val == 0) { buf[i--] = '0'; } else { while (val > 0) { buf[i--] = (val % 10) + '0'; val /= 10; } }
    bos_print(&buf[i+1]);
}

void bishop_vec3_print(const BishopVec3* v) {
    if (!v) return;
    bos_print("Vec3(");
    bvec_print_num((int64_t)(v->x * 100.0));
    bos_print(", ");
    bvec_print_num((int64_t)(v->y * 100.0));
    bos_print(", ");
    bvec_print_num((int64_t)(v->z * 100.0));
    bos_print(")/100\n");
}
