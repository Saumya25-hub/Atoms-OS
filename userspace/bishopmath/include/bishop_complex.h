#ifndef BISHOP_COMPLEX_H
#define BISHOP_COMPLEX_H

#include "bishop_error.h"

typedef struct {
    double real;
    double imag;
} BishopComplex;

void          bishop_complex_create(BishopComplex* out, double real, double imag);
void          bishop_complex_add(const BishopComplex* a, const BishopComplex* b, BishopComplex* out);
void          bishop_complex_sub(const BishopComplex* a, const BishopComplex* b, BishopComplex* out);
void          bishop_complex_mul(const BishopComplex* a, const BishopComplex* b, BishopComplex* out);
void          bishop_complex_conjugate(const BishopComplex* z, BishopComplex* out);
BishopError   bishop_complex_div(const BishopComplex* a, const BishopComplex* b, BishopComplex* out);
BishopError   bishop_complex_magnitude(const BishopComplex* z, double* out);
void          bishop_complex_print(const BishopComplex* z);

#endif // BISHOP_COMPLEX_H
