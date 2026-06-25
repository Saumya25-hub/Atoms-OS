#include "../include/bishop_scientific.h"
#include "../include/bishop_basic.h"
#include "../internal/bishop_internal.h"

// =============================================
// BISHOP X ENGINE - Scientific Math (X-2)
// All doubles passed/returned via pointers.
// =============================================

// Range reduce x to [-π, π] in-place
static void reduce_angle(double* x) {
    while (*x > BISHOP_PI) *x -= BISHOP_TAU;
    while (*x < -BISHOP_PI) *x += BISHOP_TAU;
}

BishopError bishop_sin(double x, double* out) {
    if (!out) return BISHOP_ERR_INVALID_ARG;
    if (!bishop_is_valid_p(&x)) return BISHOP_ERR_INVALID_ARG;
    reduce_angle(&x);
    double term = x;
    double sum = x;
    double x2 = x * x;
    for (int n = 1; n <= 8; n++) {
        term *= -x2 / (double)((2 * n) * (2 * n + 1));
        sum += term;
    }
    *out = sum;
    return BISHOP_OK;
}

BishopError bishop_cos(double x, double* out) {
    if (!out) return BISHOP_ERR_INVALID_ARG;
    if (!bishop_is_valid_p(&x)) return BISHOP_ERR_INVALID_ARG;
    reduce_angle(&x);
    double term = 1.0;
    double sum = 1.0;
    double x2 = x * x;
    for (int n = 1; n <= 8; n++) {
        term *= -x2 / (double)((2 * n - 1) * (2 * n));
        sum += term;
    }
    *out = sum;
    return BISHOP_OK;
}

BishopError bishop_tan(double x, double* out) {
    if (!out) return BISHOP_ERR_INVALID_ARG;
    double s, c;
    BishopError e1 = bishop_sin(x, &s);
    if (e1 != BISHOP_OK) return e1;
    BishopError e2 = bishop_cos(x, &c);
    if (e2 != BISHOP_OK) return e2;
    if (c < BISHOP_EPSILON && c > -BISHOP_EPSILON) {
        bishop_set_error(BISHOP_ERR_DIV_ZERO);
        return BISHOP_ERR_DIV_ZERO;
    }
    *out = s / c;
    return BISHOP_OK;
}

BishopError bishop_asin(double x, double* out) {
    if (!out) return BISHOP_ERR_INVALID_ARG;
    if (!bishop_is_valid_p(&x)) return BISHOP_ERR_INVALID_ARG;
    if (x < -1.0 || x > 1.0) { bishop_set_error(BISHOP_ERR_DOMAIN); return BISHOP_ERR_DOMAIN; }
    if (x == 1.0) { *out = BISHOP_PI_2; return BISHOP_OK; }
    if (x == -1.0) { *out = -BISHOP_PI_2; return BISHOP_OK; }
    double x2 = x * x;
    double term = x;
    double sum = x;
    for (int n = 1; n <= 12; n++) {
        term *= x2 * (double)(2*n - 1) * (double)(2*n - 1) / ((double)(2*n) * (double)(2*n + 1));
        sum += term;
    }
    *out = sum;
    return BISHOP_OK;
}

BishopError bishop_acos(double x, double* out) {
    if (!out) return BISHOP_ERR_INVALID_ARG;
    double asin_val;
    BishopError e = bishop_asin(x, &asin_val);
    if (e != BISHOP_OK) return e;
    *out = BISHOP_PI_2 - asin_val;
    return BISHOP_OK;
}

BishopError bishop_atan(double x, double* out) {
    if (!out) return BISHOP_ERR_INVALID_ARG;
    if (!bishop_is_valid_p(&x)) return BISHOP_ERR_INVALID_ARG;
    bool negate = false;
    bool reciprocal = false;
    if (x < 0.0) { negate = true; x = -x; }
    if (x > 1.0) { reciprocal = true; x = 1.0 / x; }
    double x2 = x * x;
    double term = x;
    double sum = x;
    for (int n = 1; n <= 20; n++) {
        term *= -x2;
        sum += term / (double)(2 * n + 1);
    }
    if (reciprocal) sum = BISHOP_PI_2 - sum;
    if (negate) sum = -sum;
    *out = sum;
    return BISHOP_OK;
}

BishopError bishop_atan2(double y, double x, double* out) {
    if (!out) return BISHOP_ERR_INVALID_ARG;
    if (!bishop_is_valid_p(&y) || !bishop_is_valid_p(&x)) return BISHOP_ERR_INVALID_ARG;
    if (x > 0.0) return bishop_atan(y / x, out);
    if (x < 0.0 && y >= 0.0) { BishopError e = bishop_atan(y / x, out); if (e != BISHOP_OK) return e; *out += BISHOP_PI; return BISHOP_OK; }
    if (x < 0.0 && y < 0.0) { BishopError e = bishop_atan(y / x, out); if (e != BISHOP_OK) return e; *out -= BISHOP_PI; return BISHOP_OK; }
    if (x == 0.0 && y > 0.0) { *out = BISHOP_PI_2; return BISHOP_OK; }
    if (x == 0.0 && y < 0.0) { *out = -BISHOP_PI_2; return BISHOP_OK; }
    return BISHOP_ERR_DOMAIN;
}

BishopError bishop_exp(double x, double* out) {
    if (!out) return BISHOP_ERR_INVALID_ARG;
    if (!bishop_is_valid_p(&x)) return BISHOP_ERR_INVALID_ARG;
    if (x > 700.0) { bishop_set_error(BISHOP_ERR_OVERFLOW); return BISHOP_ERR_OVERFLOW; }
    if (x < -700.0) { *out = 0.0; return BISHOP_OK; }
    double term = 1.0;
    double sum = 1.0;
    for (int n = 1; n <= 30; n++) {
        term *= x / (double)n;
        sum += term;
        if (term < BISHOP_EPSILON && term > -BISHOP_EPSILON) break;
    }
    *out = sum;
    return BISHOP_OK;
}

BishopError bishop_log(double x, double* out) {
    if (!out) return BISHOP_ERR_INVALID_ARG;
    if (!bishop_is_valid_p(&x)) return BISHOP_ERR_INVALID_ARG;
    if (x <= 0.0) { bishop_set_error(BISHOP_ERR_DOMAIN); return BISHOP_ERR_DOMAIN; }
    if (x == 1.0) { *out = 0.0; return BISHOP_OK; }
    int k = 0;
    double m = x;
    while (m > 2.0) { m /= 2.0; k++; }
    while (m < 0.5) { m *= 2.0; k--; }
    double t = (m - 1.0) / (m + 1.0);
    double t2 = t * t;
    double sum = t;
    double term = t;
    for (int n = 1; n <= 20; n++) {
        term *= t2;
        sum += term / (double)(2 * n + 1);
    }
    sum *= 2.0;
    *out = sum + (double)k * BISHOP_LN2;
    return BISHOP_OK;
}

BishopError bishop_log10(double x, double* out) {
    if (!out) return BISHOP_ERR_INVALID_ARG;
    double ln_val;
    BishopError e = bishop_log(x, &ln_val);
    if (e != BISHOP_OK) return e;
    *out = ln_val / BISHOP_LN10;
    return BISHOP_OK;
}
