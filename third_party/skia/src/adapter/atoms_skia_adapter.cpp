/*
 * ATOMS OS — Skia ↔ BWE Graphics Adapter Implementation
 * Copyright © 2026 ATOMS OS Project / Saumya Chaudhari
 */

#include "third_party/skia/include/adapter/atoms_skia_adapter.h"

AtomsSkiaSurface::AtomsSkiaSurface(uint32_t* pixelBuffer, int width, int height, uint32_t windowId)
    : fPixelBuffer(pixelBuffer)
    , fWidth(width)
    , fHeight(height)
    , fWindowId(windowId)
    , fSurface(nullptr) {
    if (pixelBuffer && width > 0 && height > 0) {
        SkImageInfo info = SkImageInfo::MakeN32Premul(width, height);
        fSurface = SkSurface::MakeRasterDirect(info, pixelBuffer, width * sizeof(uint32_t));
    }
}

AtomsSkiaSurface::~AtomsSkiaSurface() {
    delete fSurface;
}

SkCanvas* AtomsSkiaSurface::GetCanvas() {
    return fSurface ? fSurface->getCanvas() : nullptr;
}

bool AtomsSkiaSurface::Resize(int newWidth, int newHeight, uint32_t* newBuffer) {
    if (newWidth <= 0 || newHeight <= 0 || !newBuffer) return false;
    delete fSurface;
    fWidth = newWidth;
    fHeight = newHeight;
    fPixelBuffer = newBuffer;
    SkImageInfo info = SkImageInfo::MakeN32Premul(newWidth, newHeight);
    fSurface = SkSurface::MakeRasterDirect(info, newBuffer, newWidth * sizeof(uint32_t));
    return fSurface != nullptr;
}

void AtomsSkiaSurface::Flush() {
    if (fSurface) {
        fSurface->flush();
    }
}

// C-linkage export implementation
extern "C" {

AtomsSkiaSurfaceHandle AtomsSkia_CreateSurface(uint32_t* pixels, int width, int height, uint32_t windowId) {
    return (AtomsSkiaSurfaceHandle)new AtomsSkiaSurface(pixels, width, height, windowId);
}

void AtomsSkia_DestroySurface(AtomsSkiaSurfaceHandle handle) {
    if (handle) {
        delete (AtomsSkiaSurface*)handle;
    }
}

void AtomsSkia_RenderDemoScene(AtomsSkiaSurfaceHandle handle) {
    if (!handle) return;
    AtomsSkiaSurface* surface = (AtomsSkiaSurface*)handle;
    SkCanvas* canvas = surface->GetCanvas();
    if (!canvas) return;

    // 1. Clear background to dark navy (#1E1E2E)
    canvas->clear(0xFF1E1E2E);

    // 2. Draw Header Banner
    SkPaint paint;
    paint.setColor(0xFF313244);
    canvas->drawRect(SkRect::MakeXYWH(20, 20, (SkScalar)surface->GetWidth() - 40, 60), paint);

    // 3. Draw Rounded Rectangle Card
    paint.setColor(0xFF89B4FA);
    SkRRect card = SkRRect::MakeRectXY(SkRect::MakeXYWH(30, 100, 200, 120), 12, 12);
    canvas->drawRRect(card, paint);

    // 4. Draw Inner Card Content
    paint.setColor(0xFF181825);
    canvas->drawCircle(80, 160, 30, paint);

    // 5. Draw Translucent Alpha-Blended Overlays
    paint.setColor(0x80F38BA8); // 50% Translucent Coral Red
    canvas->drawCircle(180, 160, 40, paint);

    paint.setColor(0x80A6E3A1); // 50% Translucent Mint Green
    canvas->drawCircle(220, 160, 40, paint);

    // 6. Draw Transformed Diamond / Polygon Path
    SkPath path;
    path.moveTo(350, 100);
    path.lineTo(420, 160);
    path.lineTo(350, 220);
    path.lineTo(280, 160);
    path.close();

    paint.setColor(0xFFFAB387); // Warm Peach Orange
    canvas->drawPath(path, paint);

    // 7. Draw Stroked Outline
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setStrokeWidth(3.0f);
    paint.setColor(0xFFCDD6F4);
    canvas->drawPath(path, paint);

    surface->Flush();
}

void AtomsSkia_Flush(AtomsSkiaSurfaceHandle handle) {
    if (handle) {
        ((AtomsSkiaSurface*)handle)->Flush();
    }
}

} // extern "C"
