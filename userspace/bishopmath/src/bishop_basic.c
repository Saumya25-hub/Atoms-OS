#include "../include/bishop_basic.h"
#include "../internal/bishop_internal.h"

// =============================================
// BISHOP X ENGINE - Basic Math (X-2)
// =============================================

BishopError bishop_add(double a, double b, double* out) {
    if (!out) return BISHOP_ERR_INVALID_ARG;
    if (!bishop_is_valid_p(&a) || !bishop_is_valid_p(&b)) return BISHOP_ERR_INVALID_ARG;
    *out = a + b;
    if (bishop_is_inf_p(out)) { bishop_set_error(BISHOP_ERR_OVERFLOW); return BISHOP_ERR_OVERFLOW; }
    return BISHOP_OK;
}

BishopError bishop_sub(double a, double b, double* out) {
    if (!out) return BISHOP_ERR_INVALID_ARG;
    if (!bishop_is_valid_p(&a) || !bishop_is_valid_p(&b)) return BISHOP_ERR_INVALID_ARG;
    *out = a - b;
    if (bishop_is_inf_p(out)) { bishop_set_error(BISHOP_ERR_OVERFLOW); return BISHOP_ERR_OVERFLOW; }
    return BISHOP_OK;
}

BishopError bishop_mul(double a, double b, double* out) {
    if (!out) return BISHOP_ERR_INVALID_ARG;
    if (!bishop_is_valid_p(&a) || !bishop_is_valid_p(&b)) return BISHOP_ERR_INVALID_ARG;
    *out = a * b;
    if (bishop_is_inf_p(out)) { bishop_set_error(BISHOP_ERR_OVERFLOW); return BISHOP_ERR_OVERFLOW; }
    if (bishop_is_nan_p(out)) { bishop_set_error(BISHOP_ERR_NAN); return BISHOP_ERR_NAN; }
    return BISHOP_OK;
}

BishopError bishop_div(double a, double b, double* out) {
    if (!out) return BISHOP_ERR_INVALID_ARG;
    if (!bishop_is_valid_p(&a) || !bishop_is_valid_p(&b)) return BISHOP_ERR_INVALID_ARG;
    if (b == 0.0) { bishop_set_error(BISHOP_ERR_DIV_ZERO); return BISHOP_ERR_DIV_ZERO; }
    *out = a / b;
    if (bishop_is_inf_p(out)) { bishop_set_error(BISHOP_ERR_OVERFLOW); return BISHOP_ERR_OVERFLOW; }
    if (bishop_is_nan_p(out)) { bishop_set_error(BISHOP_ERR_NAN); return BISHOP_ERR_NAN; }
    return BISHOP_OK;
}

BishopError bishop_mod(double a, double b, double* out) {
    if (!out) return BISHOP_ERR_INVALID_ARG;
    if (!bishop_is_valid_p(&a) || !bishop_is_valid_p(&b)) return BISHOP_ERR_INVALID_ARG;
    if (b == 0.0) { bishop_set_error(BISHOP_ERR_DIV_ZERO); return BISHOP_ERR_DIV_ZERO; }
    double quotient = a / b;
    int64_t trunc_q = (int64_t)quotient;
    *out = a - ((double)trunc_q * b);
    return BISHOP_OK;
}

BishopError bishop_pow(double base, int exp, double* out) {
    if (!out) return BISHOP_ERR_INVALID_ARG;
    if (!bishop_is_valid_p(&base)) return BISHOP_ERR_INVALID_ARG;
    if (exp == 0) { *out = 1.0; return BISHOP_OK; }

    bool neg_exp = false;
    if (exp < 0) {
        if (base == 0.0) { bishop_set_error(BISHOP_ERR_DIV_ZERO); return BISHOP_ERR_DIV_ZERO; }
        neg_exp = true;
        exp = -exp;
    }

    double result = 1.0;
    double b = base;
    int e = exp;
    while (e > 0) {
        if (e & 1) {
            result *= b;
            if (bishop_is_inf_p(&result) || bishop_is_nan_p(&result)) {
                bishop_set_error(BISHOP_ERR_OVERFLOW);
                return BISHOP_ERR_OVERFLOW;
            }
        }
        b *= b;
        if (bishop_is_inf_p(&b) && e > 1) {
            bishop_set_error(BISHOP_ERR_OVERFLOW);
            return BISHOP_ERR_OVERFLOW;
        }
        e >>= 1;
    }

    if (neg_exp) result = 1.0 / result;
    *out = result;
    return BISHOP_OK;
}

BishopError bishop_sqrt(double x, double* out) {
    if (!out) return BISHOP_ERR_INVALID_ARG;
    if (!bishop_is_valid_p(&x)) return BISHOP_ERR_INVALID_ARG;
    if (x < 0.0) { bishop_set_error(BISHOP_ERR_DOMAIN); return BISHOP_ERR_DOMAIN; }
    if (x == 0.0) { *out = 0.0; return BISHOP_OK; }

    double guess = x;
    if (x > 1.0) guess = x / 2.0;
    for (int i = 0; i < 50; i++) {
        double next = 0.5 * (guess + x / guess);
        double diff = next - guess;
        if (diff < 0) diff = -diff;
        if (diff < BISHOP_EPSILON) break;
        guess = next;
    }
    *out = guess;
    return BISHOP_OK;
}

BishopError bishop_abs_val(double x, double* out) {
    if (!out) return BISHOP_ERR_INVALID_ARG;
    *out = x < 0.0 ? -x : x;
    return BISHOP_OK;
}

BishopError bishop_min_val(double a, double b, double* out) {
    if (!out) return BISHOP_ERR_INVALID_ARG;
    *out = a < b ? a : b;
    return BISHOP_OK;
}

BishopError bishop_max_val(double a, double b, double* out) {
    if (!out) return BISHOP_ERR_INVALID_ARG;
    *out = a > b ? a : b;
    return BISHOP_OK;
}

BishopError bishop_clamp(double x, double lo, double hi, double* out) {
    if (!out) return BISHOP_ERR_INVALID_ARG;
    if (lo > hi) return BISHOP_ERR_INVALID_ARG;
    if (x < lo) *out = lo;
    else if (x > hi) *out = hi;
    else *out = x;
    return BISHOP_OK;
}
