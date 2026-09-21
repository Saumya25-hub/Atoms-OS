/*
 * ATOMS OS — Userspace C Runtime math.h
 * Freestanding IEEE 754 Floating Point Math Functions
 * Copyright © 2026 ATOMS OS Project / Saumya Chaudhari
 */

#ifndef ATOMS_USER_MATH_H
#define ATOMS_USER_MATH_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define HUGE_VAL (__builtin_huge_val())
#define NAN      (__builtin_nan(""))
#define INFINITY (__builtin_inff())

double fabs(double x);
float  fabsf(float x);
double sqrt(double x);
float  sqrtf(float x);
double floor(double x);
float  floorf(float x);
double ceil(double x);
float  ceilf(float x);
double fmod(double x, double y);
float  fmodf(float x, float y);
double round(double x);
float  roundf(float x);
double trunc(double x);
float  truncf(float x);

int isnan(double x);
int isinf(double x);
int isfinite(double x);

#ifdef __cplusplus
}
#endif

#endif /* ATOMS_USER_MATH_H */
