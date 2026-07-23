#include "asset_loader.h"
#include "sys_icons_data.h"
#include "sys_icons_data_v11.h"
#include "kernel/vfs/vfs_legacy/include/vfs.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/core/lib/include/string.h"

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

    // Fast-path system status PNG asset synthesis (V1.1 Set)
    const uint8_t* src_rgba = NULL;
    if (asset_id == ICON_SYS_WIFI_CONN) src_rgba = g_icon_system_wifi_connected_rgba;
    else if (asset_id == ICON_SYS_WIFI_WEAK) src_rgba = g_icon_system_wifi_weak_rgba;
    else if (asset_id == ICON_SYS_WIFI_DISC) src_rgba = g_icon_system_wifi_disconnected_rgba;
    else if (asset_id == ICON_SYS_VOL_NORM) src_rgba = g_icon_system_volume_normal_rgba;
    else if (asset_id == ICON_SYS_VOL_LOW) src_rgba = g_icon_system_volume_low_rgba;
    else if (asset_id == ICON_SYS_VOL_MUTE) src_rgba = g_icon_system_volume_muted_rgba;
    else if (asset_id == ICON_SYS_BAT_NORM) src_rgba = g_icon_system_battery_normal_rgba;
    else if (asset_id == ICON_SYS_BAT_CHG) src_rgba = g_icon_system_battery_charging_rgba;
    else if (asset_id == ICON_SYS_BAT_LOW) src_rgba = g_icon_system_battery_low_rgba;
    else if (asset_id == ICON_SYS_BELL_NORM) src_rgba = g_icon_system_notification_normal_rgba;
    else if (asset_id == ICON_SYS_BELL_UNREAD) src_rgba = g_icon_system_notification_unread_rgba;

    if (src_rgba && w == 16 && h == 16) {
        memcpy(px, src_rgba, 16 * 16 * 4);
        BOImage* img = BOImage_LoadBMP(bmp_data, data_size);
        kfree(bmp_data);
        return img;
    }
    for (uint32_t y = 0; y < h; y++) {
        for (uint32_t x = 0; x < w; x++) {
            uint32_t idx = (y * w + x) * 4;
            uint8_t b = 0, g = 0, red = 0, a = 255;

            if (asset_id == ICON_FOLDER || asset_id == ICON_EXPLORER) {
                // Golden yellow folder / Explorer
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
            } else if (asset_id == ICON_SETTINGS) {
                // Sleek metallic gear / slate tile
                bool border = (x == 0 || x == w - 1 || y == 0 || y == h - 1);
                if (border) { b = 100; g = 100; red = 100; }
                else if ((x >= 12 && x <= 20 && y >= 8 && y <= 24) || (x >= 8 && x <= 24 && y >= 12 && y <= 20)) {
                    b = 220; g = 210; red = 200;
                } else {
                    b = 100; g = 80; red = 60;
                }
            } else if (asset_id == ICON_CALCULATOR) {
                // Slate calculator tile with screen
                if (y >= 4 && y <= 10 && x >= 6 && x <= w - 7) {
                    b = 180; g = 220; red = 180; // Screen
                } else {
                    b = 100; g = 70; red = 70;
                }
            } else if (asset_id == ICON_STRESS_TEST) {
                // Microchip tile
                if (x >= 6 && x <= w - 7 && y >= 6 && y <= h - 7) {
                    b = 180; g = 100; red = 40; // CPU chip core
                } else {
                    b = 60; g = 40; red = 20;
                }
            } else if (asset_id == ICON_MUSIC) {
                // Musical note blue tile
                if (x >= 12 && x <= 20 && y >= 6 && y <= 24) {
                    b = 255; g = 255; red = 255; // Note
                } else {
                    b = 230; g = 100; red = 40; // Royal blue
                }
            } else if (asset_id == ICON_DOOM) {
                // Red DOOM gaming tile
                if (x >= 4 && x <= w - 5 && y >= 4 && y <= h - 5) {
                    b = 30; g = 30; red = 220; // Red tile
                } else {
                    b = 20; g = 20; red = 80;
                }
            } else if (asset_id == ICON_INPUT_LAB) {
                // Indigo input cursor tile
                if (x >= 8 && x <= 24 && y >= 8 && y <= 24) {
                    b = 240; g = 180; red = 100;
                } else {
                    b = 180; g = 60; red = 60;
                }
            } else if (asset_id == ICON_GRAPH_3D) {
                // Futuristic 3D Floating Gem / Cube Icon Synthesis
                int32_t cx = (int32_t)w / 2;
                int32_t cy = (int32_t)h / 2 - 2;
                int32_t dx = (int32_t)x - cx;
                int32_t dy = (int32_t)y - cy;
                
                // Outer Squircle Tile Background (#0F172A)
                a = 230; red = 15; g = 23; b = 42;
                
                // Drop shadow underneath (floating effect)
                if (y >= h - 6 && x >= 6 && x <= w - 7) {
                    a = 120; red = 0; g = 0; b = 0;
                }
                // 3D Cube Top Face (Bright Cyan #38BDF8)
                else if (dy < 0 && (dy + (dx > 0 ? dx : -dx) / 2) >= -8 && dy >= -10) {
                    red = 56; g = 189; b = 248; a = 255;
                }
                // 3D Cube Left Face (Deep Cyan #0284C7)
                else if (dx <= 0 && dy >= 0 && dy <= 10 && dx >= -10 && (dy - dx / 2) <= 12) {
                    red = 2; g = 132; b = 199; a = 255;
                }
                // 3D Cube Right Face (Violet/Indigo #6366F1)
                else if (dx > 0 && dy >= 0 && dy <= 10 && dx <= 10 && (dy + dx / 2) <= 12) {
                    red = 99; g = 102; b = 241; a = 255;
                }
                // Cyan Glow Border
                else if (x == 2 || x == w - 3 || y == 2 || y == h - 3) {
                    red = 56; g = 189; b = 248; a = 180;
                }
            } else if (asset_id == ICON_CLOSE) {
                // Vibrant red button with white X
                bool is_x = (x == y || x == (w - 1 - y)) && (x >= 6 && x <= w - 7);
                if (is_x) { b = 255; g = 255; red = 255; }
                else { b = 50; g = 50; red = 235; }
            } else if (asset_id == ASSET_LOGO) {
                // ATOMS 64x64 Minimalist Ultra-Premium Start Emblem
                float fx = (float)x - 31.5f;
                float fy = (float)y - 31.5f;
                float r_sq = fx*fx + fy*fy;
                
                // 100% Fully Transparent Background
                a = 0; b = 0; g = 0; red = 0;

                // 1. Sleek Outer Cyan Halo (Radius 22 to 24.5)
                if (r_sq >= 21.5f * 21.5f && r_sq <= 24.5f * 24.5f) {
                    red = 56; g = 189; b = 248; a = 230; // Bright Electric Cyan
                }
                
                // 2. Central Premium Gradient Sphere (Royal Indigo to Cyan)
                if (r_sq < 21.0f * 21.0f) {
                    float t = r_sq / (21.0f * 21.0f);
                    float light = (fx * -0.5f + fy * -0.5f) / 30.0f;
                    if (light < 0.0f) light = 0.0f;
                    if (light > 0.4f) light = 0.4f;

                    red = (uint8_t)((37  + (uint8_t)(light * 180.0f)) * (1.0f - t) + 15 * t);
                    g   = (uint8_t)((99  + (uint8_t)(light * 180.0f)) * (1.0f - t) + 23 * t);
                    b   = (uint8_t)((235 + (uint8_t)(light * 40.0f))  * (1.0f - t) + 42 * t);
                    a   = 240;
                }

                // 3. Single Minimalist Diagonal Orbit Ring (Crisp White Accent)
                float rx = fx * 0.819f + fy * 0.573f;  // 35-degree rotation
                float ry = -fx * 0.573f + fy * 0.819f;
                float eq = (rx*rx)/(17.0f*17.0f) + (ry*ry)/(5.5f*5.5f);
                if (eq >= 0.80f && eq <= 1.20f && r_sq < 23.5f * 23.5f) {
                    red = 255; g = 255; b = 255; a = 255; // Pure Crisp White
                }

                // 4. Central Quantum Nucleus Node (Pure White Core Spark)
                if (r_sq < 4.5f * 4.5f) {
                    red = 255; g = 255; b = 255; a = 255;
                }
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
    else if (handle->id >= ICON_SYS_WIFI_CONN && handle->id <= ICON_SYS_BELL_UNREAD) { w = 16; h = 16; }

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

    BOImage* img = NULL;
    uint32_t fmt = BOImage_FormatDetector(file_buf, (uint32_t)bytes_read);
    if (fmt == BOIMAGE_FORMAT_PNG) {
        img = BOImage_LoadPNG(file_buf, (uint32_t)bytes_read);
    } else {
        img = BOImage_LoadBMP(file_buf, (uint32_t)bytes_read);
    }
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
