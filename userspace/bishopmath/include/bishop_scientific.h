#ifndef BISHOP_SCIENTIFIC_H
#define BISHOP_SCIENTIFIC_H

#include "bishop_error.h"

// =============================================
// BISHOP X ENGINE - Scientific Math (X-2)
// All results via output pointer.
// =============================================

BishopError bishop_sin(double x, double* out);
BishopError bishop_cos(double x, double* out);
BishopError bishop_tan(double x, double* out);
BishopError bishop_asin(double x, double* out);
BishopError bishop_acos(double x, double* out);
BishopError bishop_atan(double x, double* out);
BishopError bishop_atan2(double y, double x, double* out);
BishopError bishop_exp(double x, double* out);
BishopError bishop_log(double x, double* out);
BishopError bishop_log10(double x, double* out);

#endif // BISHOP_SCIENTIFIC_H
