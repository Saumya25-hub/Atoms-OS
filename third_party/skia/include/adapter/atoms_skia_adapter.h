/*
 * ATOMS OS — Skia ↔ BWE Graphics Adapter Layer
 * Copyright © 2026 ATOMS OS Project / Saumya Chaudhari
 */

#ifndef ATOMS_SKIA_ADAPTER_H
#define ATOMS_SKIA_ADAPTER_H

#include "third_party/skia/include/core/SkTypes.h"
#include "third_party/skia/include/core/SkSurface.h"
#include "third_party/skia/include/core/SkCanvas.h"

#ifdef __cplusplus
class AtomsSkiaSurface {
public:
    AtomsSkiaSurface(uint32_t* pixelBuffer, int width, int height, uint32_t windowId = 0);
    ~AtomsSkiaSurface();

    SkCanvas* GetCanvas();
    SkSurface* GetSurface() const { return fSurface; }

    int GetWidth() const { return fWidth; }
    int GetHeight() const { return fHeight; }
    uint32_t* GetPixelBuffer() const { return fPixelBuffer; }
    uint32_t  GetWindowId() const { return fWindowId; }

    bool Resize(int newWidth, int newHeight, uint32_t* newBuffer);
    void Flush();

private:
    uint32_t*  fPixelBuffer;
    int        fWidth;
    int        fHeight;
    uint32_t   fWindowId;
    SkSurface* fSurface;
};

extern "C" {
#endif

// C-linkage helper for BWE / ATRIX integration
typedef void* AtomsSkiaSurfaceHandle;

AtomsSkiaSurfaceHandle AtomsSkia_CreateSurface(uint32_t* pixels, int width, int height, uint32_t windowId);
void                   AtomsSkia_DestroySurface(AtomsSkiaSurfaceHandle handle);
void                   AtomsSkia_RenderDemoScene(AtomsSkiaSurfaceHandle handle);
void                   AtomsSkia_Flush(AtomsSkiaSurfaceHandle handle);

#ifdef __cplusplus
}
#endif

#endif // ATOMS_SKIA_ADAPTER_H
