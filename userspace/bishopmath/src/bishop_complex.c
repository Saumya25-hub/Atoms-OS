#include "../include/bishop_complex.h"
#include "../include/bishop_basic.h"
#include "../internal/bishop_internal.h"

void bishop_complex_create(BishopComplex* out, double real, double imag) {
    if (!out) return;
    out->real = real; out->imag = imag;
}

void bishop_complex_add(const BishopComplex* a, const BishopComplex* b, BishopComplex* out) {
    if (!a || !b || !out) return;
    out->real = a->real + b->real; out->imag = a->imag + b->imag;
}

void bishop_complex_sub(const BishopComplex* a, const BishopComplex* b, BishopComplex* out) {
    if (!a || !b || !out) return;
    out->real = a->real - b->real; out->imag = a->imag - b->imag;
}

void bishop_complex_mul(const BishopComplex* a, const BishopComplex* b, BishopComplex* out) {
    if (!a || !b || !out) return;
    out->real = a->real * b->real - a->imag * b->imag;
    out->imag = a->real * b->imag + a->imag * b->real;
}

void bishop_complex_conjugate(const BishopComplex* z, BishopComplex* out) {
    if (!z || !out) return;
    out->real = z->real; out->imag = -z->imag;
}

BishopError bishop_complex_div(const BishopComplex* a, const BishopComplex* b, BishopComplex* out) {
    if (!a || !b || !out) return BISHOP_ERR_INVALID_ARG;
    double denom = b->real * b->real + b->imag * b->imag;
    if (denom < BISHOP_EPSILON && denom > -BISHOP_EPSILON) {
        bishop_set_error(BISHOP_ERR_DIV_ZERO);
        return BISHOP_ERR_DIV_ZERO;
    }
    out->real = (a->real * b->real + a->imag * b->imag) / denom;
    out->imag = (a->imag * b->real - a->real * b->imag) / denom;
    return BISHOP_OK;
}

BishopError bishop_complex_magnitude(const BishopComplex* z, double* out) {
    if (!z || !out) return BISHOP_ERR_INVALID_ARG;
    double mag_sq = z->real * z->real + z->imag * z->imag;
    return bishop_sqrt(mag_sq, out);
}

static void bcplx_print_num(int64_t val) {
    if (val < 0) { bos_print("-"); val = -val; }
    char buf[20]; int i = 18; buf[19] = '\0';
    if (val == 0) { buf[i--] = '0'; } else { while (val > 0) { buf[i--] = (val % 10) + '0'; val /= 10; } }
    bos_print(&buf[i+1]);
}

void bishop_complex_print(const BishopComplex* z) {
    if (!z) return;
    bos_print("(");
    bcplx_print_num((int64_t)z->real);
    if (z->imag >= 0.0) bos_print(" + "); else bos_print(" - ");
    double ai = z->imag < 0.0 ? -z->imag : z->imag;
    bcplx_print_num((int64_t)ai);
    bos_print("i)\n");
}
