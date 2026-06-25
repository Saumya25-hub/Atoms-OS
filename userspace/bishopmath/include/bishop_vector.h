#ifndef BISHOP_VECTOR_H
#define BISHOP_VECTOR_H

#include "bishop_error.h"

typedef struct {
    double x, y, z;
} BishopVec3;

void          bishop_vec3_create(BishopVec3* out, double x, double y, double z);
void          bishop_vec3_zero(BishopVec3* out);
void          bishop_vec3_add(const BishopVec3* a, const BishopVec3* b, BishopVec3* out);
void          bishop_vec3_sub(const BishopVec3* a, const BishopVec3* b, BishopVec3* out);
void          bishop_vec3_scale(const BishopVec3* v, double s, BishopVec3* out);
BishopError   bishop_vec3_dot(const BishopVec3* a, const BishopVec3* b, double* out);
void          bishop_vec3_cross(const BishopVec3* a, const BishopVec3* b, BishopVec3* out);
BishopError   bishop_vec3_magnitude(const BishopVec3* v, double* out);
BishopError   bishop_vec3_normalize(const BishopVec3* v, BishopVec3* out);
BishopError   bishop_vec3_distance(const BishopVec3* a, const BishopVec3* b, double* out);
void          bishop_vec3_print(const BishopVec3* v);

#endif // BISHOP_VECTOR_H
