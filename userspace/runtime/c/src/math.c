/*
 * ATOMS OS — Userspace Freestanding IEEE 754 Math Library
 * Copyright © 2026 ATOMS OS Project / Saumya Chaudhari
 */

#include "../include/math.h"
#include <stdint.h>
#include <stddef.h>

typedef union {
    double d;
    uint64_t u;
} double_bits_t;

typedef union {
    float f;
    uint32_t u;
} float_bits_t;

int isnan(double x) {
    double_bits_t b;
    b.d = x;
    return ((b.u & 0x7FF0000000000000ULL) == 0x7FF0000000000000ULL) &&
           ((b.u & 0x000FFFFFFFFFFFFFULL) != 0);
}

int isinf(double x) {
    double_bits_t b;
    b.d = x;
    return ((b.u & 0x7FFFFFFFFFFFFFFFULL) == 0x7FF0000000000000ULL);
}

int isfinite(double x) {
    double_bits_t b;
    b.d = x;
    return ((b.u & 0x7FF0000000000000ULL) != 0x7FF0000000000000ULL);
}

double fabs(double x) {
    double_bits_t b;
    b.d = x;
    b.u &= 0x7FFFFFFFFFFFFFFFULL;
    return b.d;
}

float fabsf(float x) {
    float_bits_t b;
    b.f = x;
    b.u &= 0x7FFFFFFFU;
    return b.f;
}

double trunc(double x) {
    if (isnan(x) || isinf(x)) return x;
    if (fabs(x) >= 4503599627370496.0) return x; // 2^52 has no fractional bits
    int64_t i = (int64_t)x;
    return (double)i;
}

float truncf(float x) {
    return (float)trunc((double)x);
}

double floor(double x) {
    if (isnan(x) || isinf(x)) return x;
    if (fabs(x) >= 4503599627370496.0) return x;
    int64_t i = (int64_t)x;
    double d = (double)i;
    if (x < d) return d - 1.0;
    return d;
}

float floorf(float x) {
    return (float)floor((double)x);
}

double ceil(double x) {
    if (isnan(x) || isinf(x)) return x;
    if (fabs(x) >= 4503599627370496.0) return x;
    int64_t i = (int64_t)x;
    double d = (double)i;
    if (x > d) return d + 1.0;
    return d;
}

float ceilf(float x) {
    return (float)ceil((double)x);
}

double round(double x) {
    if (isnan(x) || isinf(x)) return x;
    return (x >= 0.0) ? floor(x + 0.5) : ceil(x - 0.5);
}

float roundf(float x) {
    return (float)round((double)x);
}

double fmod(double x, double y) {
    if (isnan(x) || isnan(y) || isinf(x) || y == 0.0) return NAN;
    if (isinf(y)) return x;
    if (x == 0.0) return 0.0;

    double d = trunc(x / y);
    return x - (d * y);
}

float fmodf(float x, float y) {
    return (float)fmod((double)x, (double)y);
}

double sqrt(double x) {
    if (isnan(x) || x < 0.0) return NAN;
    if (x == 0.0 || isinf(x)) return x;

    /* Fast initial approximation using IEEE bit representation */
    double_bits_t b;
    b.d = x;
    b.u = (1ULL << 61) + (b.u >> 1) - (1ULL << 51);
    double guess = b.d;

    /* 6 Newton-Raphson iterations achieve full 53-bit IEEE 754 precision */
    for (int i = 0; i < 6; i++) {
        guess = 0.5 * (guess + (x / guess));
    }

    return guess;
}

float sqrtf(float x) {
    return (float)sqrt((double)x);
}
