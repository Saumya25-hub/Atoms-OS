/*
 * Copyright 2013 Google Inc.
 * Copyright 2026 The Skia Authors / ATOMS OS Adaptation
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef SkImageInfo_DEFINED
#define SkImageInfo_DEFINED

#include "SkRect.h"
#include "SkColor.h"

enum SkColorType {
    kUnknown_SkColorType,
    kAlpha_8_SkColorType,
    kRGB_565_SkColorType,
    kARGB_4444_SkColorType,
    kRGBA_8888_SkColorType,
    kRGB_888x_SkColorType,
    kBGRA_8888_SkColorType,
    kRGBA_1010102_SkColorType,
    kBGRA_1010102_SkColorType,
    kRGB_101010x_SkColorType,
    kBGR_101010x_SkColorType,
    kGray_8_SkColorType,
    kRGBA_F16Norm_SkColorType,
    kRGBA_F16_SkColorType,
    kRGBA_F32_SkColorType,

    kN32_SkColorType = kBGRA_8888_SkColorType,
};

enum SkAlphaType {
    kUnknown_SkAlphaType,
    kOpaque_SkAlphaType,
    kPremul_SkAlphaType,
    kUnpremul_SkAlphaType,
};

struct SkImageInfo {
    int32_t     fWidth;
    int32_t     fHeight;
    SkColorType fColorType;
    SkAlphaType fAlphaType;

    SkImageInfo() : fWidth(0), fHeight(0), fColorType(kUnknown_SkColorType), fAlphaType(kUnknown_SkAlphaType) {}
    SkImageInfo(int32_t w, int32_t h, SkColorType ct, SkAlphaType at)
        : fWidth(w), fHeight(h), fColorType(ct), fAlphaType(at) {}

    static SkImageInfo Make(int32_t width, int32_t height, SkColorType ct, SkAlphaType at) {
        return SkImageInfo(width, height, ct, at);
    }

    static SkImageInfo MakeN32Premul(int32_t width, int32_t height) {
        return SkImageInfo(width, height, kN32_SkColorType, kPremul_SkAlphaType);
    }

    static SkImageInfo MakeN32(int32_t width, int32_t height, SkAlphaType at) {
        return SkImageInfo(width, height, kN32_SkColorType, at);
    }

    int32_t width() const { return fWidth; }
    int32_t height() const { return fHeight; }
    SkColorType colorType() const { return fColorType; }
    SkAlphaType alphaType() const { return fAlphaType; }
    bool isEmpty() const { return fWidth <= 0 || fHeight <= 0; }

    size_t bytesPerPixel() const {
        switch (fColorType) {
            case kRGBA_8888_SkColorType:
            case kBGRA_8888_SkColorType:
            case kRGB_888x_SkColorType:
                return 4;
            case kRGB_565_SkColorType:
            case kARGB_4444_SkColorType:
                return 2;
            case kAlpha_8_SkColorType:
            case kGray_8_SkColorType:
                return 1;
            default:
                return 4;
        }
    }

    size_t minRowBytes() const { return (size_t)fWidth * bytesPerPixel(); }
    size_t computeByteSize(size_t rowBytes) const { return (size_t)fHeight * rowBytes; }
};

#endif // SkImageInfo_DEFINED
