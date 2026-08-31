/*
 * Copyright 2006 The Android Open Source Project
 * Copyright 2026 The Skia Authors / ATOMS OS Adaptation
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef SkTypes_DEFINED
#define SkTypes_DEFINED

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef float SkScalar;

#define SK_Scalar1              1.0f
#define SK_ScalarHalf           0.5f
#define SK_ScalarPI             3.14159265f

#define SkIntToScalar(x)        ((SkScalar)(x))
#define SkScalarFloorToInt(x)   ((int)(x))
#define SkScalarRoundToInt(x)   ((int)((x) + 0.5f))
#define SkScalarCeilToInt(x)    ((int)((x) + 0.9999f))
#define SkScalarAbs(x)          ((x) < 0.0f ? -(x) : (x))

static inline SkScalar SkScalarMin(SkScalar a, SkScalar b) { return a < b ? a : b; }
static inline SkScalar SkScalarMax(SkScalar a, SkScalar b) { return a > b ? a : b; }

template <typename T>
static inline void SkTSwap(T& a, T& b) {
    T tmp = a;
    a = b;
    b = tmp;
}

#endif // SkTypes_DEFINED
