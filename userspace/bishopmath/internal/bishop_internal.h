#ifndef BISHOP_INTERNAL_H
#define BISHOP_INTERNAL_H

// =============================================
// BISHOP X ENGINE - Internal Macros & Constants
// =============================================

#include "../../atoms/internal/atom_memory.h"
#include "../../libbos/include/bos.h"
#include "../include/bishop_error.h"

// Memory bridge
#define BISHOP_ALLOC(size)  atom_alloc(size)
#define BISHOP_FREE(ptr)    atom_free(ptr)

// Mathematical constants (high precision)
#define BISHOP_PI       3.14159265358979323846
#define BISHOP_TAU      6.28318530717958647692
#define BISHOP_E        2.71828182845904523536
#define BISHOP_PI_2     1.57079632679489661923
#define BISHOP_PI_4     0.78539816339744830962
#define BISHOP_INV_PI   0.31830988618379067154
#define BISHOP_LN2      0.69314718055994530942
#define BISHOP_LN10     2.30258509299404568402
#define BISHOP_SQRT2    1.41421356237309504880

// Safety limits
#define BISHOP_EPSILON       1e-12
#define BISHOP_MAX_MATRIX    16

#endif // BISHOP_INTERNAL_H
