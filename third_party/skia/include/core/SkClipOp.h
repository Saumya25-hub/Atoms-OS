/*
 * Copyright 2016 Google Inc.
 * Copyright 2026 The Skia Authors / ATOMS OS Adaptation
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef SkClipOp_DEFINED
#define SkClipOp_DEFINED

enum class SkClipOp {
    kDifference,
    kIntersect,
    kMax_Op = kIntersect
};

#endif // SkClipOp_DEFINED
