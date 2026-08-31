/*
 * Copyright 2012 Google Inc.
 * Copyright 2026 The Skia Authors / ATOMS OS Adaptation
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef SkSurface_DEFINED
#define SkSurface_DEFINED

#include "SkTypes.h"
#include "SkImageInfo.h"
#include "SkCanvas.h"

class SkSurface {
public:
    virtual ~SkSurface();

    int width() const { return fInfo.width(); }
    int height() const { return fInfo.height(); }
    SkImageInfo imageInfo() const { return fInfo; }

    SkCanvas* getCanvas();
    void* getPixels() const { return fPixels; }
    size_t rowBytes() const { return fRowBytes; }

    void flush();

    // Factory methods
    static SkSurface* MakeRasterDirect(const SkImageInfo& info, void* pixels, size_t rowBytes);
    static SkSurface* MakeRaster(const SkImageInfo& info);
    static SkSurface* MakeRasterN32Premul(int32_t width, int32_t height);

protected:
    SkSurface(const SkImageInfo& info, void* pixels, size_t rowBytes, bool ownsPixels);

private:
    SkImageInfo fInfo;
    void*       fPixels;
    size_t      fRowBytes;
    bool        fOwnsPixels;
    SkCanvas*   fCanvas;
};

#endif // SkSurface_DEFINED
