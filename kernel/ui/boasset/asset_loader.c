#include "asset_loader.h"
#include "kernel/vfs/vfs_legacy/include/vfs.h"
#include "kernel/core/memory/heap/include/heap.h"

// Helper to construct a raw 32-bit BMP buffer in heap and decode via BOIMAGE
static BOImage* synthesize_bmp_icon(uint32_t asset_id, uint32_t w, uint32_t h) {
    uint32_t data_size = sizeof(BMPHeader) + sizeof(BMPInfoHeader) + (w * h * 4);
    uint8_t* bmp_data = (uint8_t*)kmalloc(data_size);
    if (!bmp_data) return NULL;

    BMPHeader* fh = (BMPHeader*)bmp_data;
    fh->type = 0x4D42; // "BM"
    fh->size = data_size;
    fh->reserved = 0;
    fh->offset = sizeof(BMPHeader) + sizeof(BMPInfoHeader);

    BMPInfoHeader* ih = (BMPInfoHeader*)(bmp_data + sizeof(BMPHeader));
    ih->size = sizeof(BMPInfoHeader);
    ih->width = w;
    ih->height = -(int32_t)h; // Top-down
    ih->planes = 1;
    ih->bpp = 32;
    ih->compression = 0;
    ih->sizeImage = w * h * 4;
    ih->xPelsPerMeter = 2835;
    ih->yPelsPerMeter = 2835;
    ih->clrUsed = 0;
    ih->clrImportant = 0;

    uint8_t* px = bmp_data + fh->offset;
    for (uint32_t y = 0; y < h; y++) {
        for (uint32_t x = 0; x < w; x++) {
            uint32_t idx = (y * w + x) * 4;
            uint8_t b = 0, g = 0, red = 0, a = 255;

            if (asset_id == ICON_FOLDER) {
                // Golden yellow folder with top tab
                if (y < 6 && x < w / 2) {
                    b = 40; g = 180; red = 240; // Tab
                } else if (y >= 5) {
                    b = 50; g = 200; red = 255; // Body
                } else {
                    a = 0; // Transparent corner
                }
            } else if (asset_id == ICON_FILE) {
                // Silver document with folded corner
                if (x > w - 8 && y < 8) {
                    b = 200; g = 150; red = 100; // Fold
                } else {
                    b = 245; g = 245; red = 250; // Sheet
                }
            } else if (asset_id == ICON_TERMINAL) {
                // Dark console box with green prompt
                bool border = (x == 0 || x == w - 1 || y == 0 || y == h - 1);
                if (border) { b = 80; g = 80; red = 80; }
                else if (y >= 10 && y <= 16 && x >= 8 && x <= 14) {
                    b = 50; g = 255; red = 50; // Green cursor prompt
                } else {
                    b = 30; g = 25; red = 20; // Navy black console background
                }
            } else if (asset_id == ICON_CLOSE) {
                // Vibrant red button with white X
                bool is_x = (x == y || x == (w - 1 - y)) && (x >= 6 && x <= w - 7);
                if (is_x) { b = 255; g = 255; red = 255; }
                else { b = 50; g = 50; red = 235; }
            } else if (asset_id == ASSET_LOGO) {
                // ATOMS 64x64 glowing emblem
                float fx = (float)x - 31.5f;
                float fy = (float)y - 31.5f;
                float r_sq = fx*fx + fy*fy;
                if (r_sq < 30.0f * 30.0f) { b = 50; g = 25; red = 20; a = 240; }
                if (r_sq >= 28.0f * 28.0f && r_sq <= 30.0f * 30.0f) { b = 255; g = 180; red = 0; a = 255; }
                float eq1 = (fx*fx)/(24.0f*24.0f) + (fy*fy)/(8.0f*8.0f);
                if (eq1 >= 0.75f && eq1 <= 1.25f) { b = 255; g = 255; red = 0; a = 255; }
                float rx2 = fx * 0.5f + fy * 0.866f;
                float ry2 = -fx * 0.866f + fy * 0.5f;
                float eq2 = (rx2*rx2)/(24.0f*24.0f) + (ry2*ry2)/(8.0f*8.0f);
                if (eq2 >= 0.75f && eq2 <= 1.25f) { b = 255; g = 0; red = 255; a = 255; }
                if (r_sq < 7.0f * 7.0f) { b = 0; g = 215; red = 255; a = 255; }
            } else {
                // Generic tech gradient icon fallback
                b = (uint8_t)(x * 7);
                g = (uint8_t)(y * 7);
                red = 180;
            }

            px[idx + 0] = b;
            px[idx + 1] = g;
            px[idx + 2] = red;
            px[idx + 3] = a;
        }
    }

    BOImage* img = BOImage_LoadBMP(bmp_data, data_size);
    kfree(bmp_data);
    return img;
}

int BOAssetLoader_GeneratePlaceholder(BOAssetHandle* handle) {
    if (!handle) return BOASSET_ERROR_NOT_FOUND;

    uint32_t w = 32, h = 32;
    if (handle->id == ASSET_LOGO) { w = 64; h = 64; }
    else if (handle->id == ICON_CLOSE || handle->id == ICON_MINIMIZE || handle->id == ICON_MAXIMIZE) { w = 24; h = 24; }

    BOImage* img = synthesize_bmp_icon(handle->id, w, h);
    if (!img) return BOASSET_ERROR_OUT_OF_MEMORY;

    handle->image_data = img;
    handle->width = img->width;
    handle->height = img->height;
    handle->loaded = true;
    return BOASSET_OK;
}

int BOAssetLoader_LoadFromVFS(BOAssetHandle* handle) {
    if (!handle || !handle->filepath[0]) {
        return BOAssetLoader_GeneratePlaceholder(handle);
    }

    int fd = vfs_open(handle->filepath);
    if (fd < 0) {
        // File not on disk yet; fallback gracefully to placeholder
        return BOAssetLoader_GeneratePlaceholder(handle);
    }

    // Allocate 128KB temp buffer for file read
    uint32_t max_buf = 128 * 1024;
    uint8_t* file_buf = (uint8_t*)kmalloc(max_buf);
    if (!file_buf) {
        vfs_close(fd);
        return BOAssetLoader_GeneratePlaceholder(handle);
    }

    int bytes_read = vfs_read(fd, file_buf, max_buf);
    vfs_close(fd);

    if (bytes_read <= 0) {
        kfree(file_buf);
        return BOAssetLoader_GeneratePlaceholder(handle);
    }

    BOImage* img = BOImage_LoadBMP(file_buf, (uint32_t)bytes_read);
    kfree(file_buf);

    if (!img) {
        // Not a valid BMP or PNG decoder missing; fallback to placeholder
        return BOAssetLoader_GeneratePlaceholder(handle);
    }

    handle->image_data = img;
    handle->width = img->width;
    handle->height = img->height;
    handle->loaded = true;
    return BOASSET_OK;
}
