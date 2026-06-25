#ifndef BISHOP_BASIC_H
#define BISHOP_BASIC_H

#include "bishop_error.h"

// =============================================
// BISHOP X ENGINE - Basic Math (X-2)
// All results returned via output pointer.
// Functions return BishopError status.
// =============================================

BishopError bishop_add(double a, double b, double* out);
BishopError bishop_sub(double a, double b, double* out);
BishopError bishop_mul(double a, double b, double* out);
BishopError bishop_div(double a, double b, double* out);
BishopError bishop_mod(double a, double b, double* out);
BishopError bishop_pow(double base, int exp, double* out);
BishopError bishop_sqrt(double x, double* out);
BishopError bishop_abs_val(double x, double* out);
BishopError bishop_min_val(double a, double b, double* out);
BishopError bishop_max_val(double a, double b, double* out);
BishopError bishop_clamp(double x, double lo, double hi, double* out);

#endif // BISHOP_BASIC_H
