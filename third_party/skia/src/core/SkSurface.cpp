/*
 * Copyright 2012 Google Inc.
 * Copyright 2026 The Skia Authors / ATOMS OS Adaptation
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "third_party/skia/include/core/SkSurface.h"
#include "userspace/runtime/c/include/stdlib.h"

SkSurface::SkSurface(const SkImageInfo& info, void* pixels, size_t rowBytes, bool ownsPixels)
    : fInfo(info)
    , fPixels(pixels)
    , fRowBytes(rowBytes)
    , fOwnsPixels(ownsPixels)
    , fCanvas(nullptr) {
    if (pixels && !info.isEmpty()) {
        fCanvas = new SkCanvas(info.width(), info.height(), pixels, rowBytes);
    }
}

SkSurface::~SkSurface() {
    delete fCanvas;
    if (fOwnsPixels && fPixels) {
        free(fPixels);
    }
}

SkCanvas* SkSurface::getCanvas() {
    return fCanvas;
}

void SkSurface::flush() {
    if (fCanvas) {
        fCanvas->flush();
    }
}

SkSurface* SkSurface::MakeRasterDirect(const SkImageInfo& info, void* pixels, size_t rowBytes) {
    if (info.isEmpty() || !pixels || rowBytes < info.minRowBytes()) {
        return nullptr;
    }
    return new SkSurface(info, pixels, rowBytes, false);
}

SkSurface* SkSurface::MakeRaster(const SkImageInfo& info) {
    if (info.isEmpty()) return nullptr;
    size_t rowBytes = info.minRowBytes();
    size_t totalBytes = info.computeByteSize(rowBytes);
    void* pixels = malloc(totalBytes);
    if (!pixels) return nullptr;
    return new SkSurface(info, pixels, rowBytes, true);
}

SkSurface* SkSurface::MakeRasterN32Premul(int32_t width, int32_t height) {
    return MakeRaster(SkImageInfo::MakeN32Premul(width, height));
}
