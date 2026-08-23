#include "asset_loader.h"
#include "sys_icons_data.h"
#include "sys_icons_data_v11.h"
#include "kernel/vfs/vfs_legacy/include/vfs.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/core/lib/include/string.h"

#include "kernel/ui/icon_engine/include/atoms_icon_data.h"

// Helper to construct a raw 32-bit BMP buffer in heap using master HD icon bitmaps
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

    // Resolve Canonical 48x48 Master Icon Bitmap
    const uint32_t* src_bmp = NULL;
    switch (asset_id) {
        case ICON_EXPLORER:       src_bmp = g_atoms_ico_explorer_48; break;
        case ICON_TERMINAL:       src_bmp = g_atoms_ico_terminal_48; break;
        case ICON_SETTINGS:       src_bmp = g_atoms_ico_settings_48; break;
        case ICON_CALCULATOR:     src_bmp = g_atoms_ico_calculator_48; break;
        case ICON_FILE:           src_bmp = g_atoms_ico_notes_48; break;
        case ICON_FOLDER:         src_bmp = g_atoms_ico_folder_48; break;
        case ICON_STRESS_TEST:
        case ICON_TMH:            src_bmp = g_atoms_ico_tmh_48; break;
        case ICON_MUSIC:          src_bmp = g_atoms_ico_music_48; break;
        case ICON_DOOM:           src_bmp = g_atoms_ico_doom_48; break;
        case ICON_INPUT_LAB:      src_bmp = g_atoms_ico_inputlab_48; break;
        case ICON_ATRIX:          src_bmp = g_atoms_ico_atrix_48; break;
        case ICON_GRAPH_3D:       src_bmp = g_atoms_ico_graph3d_48; break;
        case ASSET_LOGO:          src_bmp = g_atoms_ico_atoms_start_48; break;
        default:                  src_bmp = g_atoms_ico_folder_48; break;
    }

    // High-Quality Bilinear Resampler into BMP Pixel Buffer
    uint32_t src_sz = ATOMS_ICON_MASTER_SIZE;
    for (uint32_t y = 0; y < h; y++) {
        uint32_t fy = (y * src_sz * 256) / h;
        uint32_t y0 = fy >> 8;
        uint32_t y_frac = fy & 0xFF;
        uint32_t y1 = (y0 + 1 < src_sz) ? y0 + 1 : y0;

        for (uint32_t x = 0; x < w; x++) {
            uint32_t fx = (x * src_sz * 256) / w;
            uint32_t x0 = fx >> 8;
            uint32_t x_frac = fx & 0xFF;
            uint32_t x1 = (x0 + 1 < src_sz) ? x0 + 1 : x0;

            uint32_t c00 = src_bmp[y0 * src_sz + x0];
            uint32_t c10 = src_bmp[y0 * src_sz + x1];
            uint32_t c01 = src_bmp[y1 * src_sz + x0];
            uint32_t c11 = src_bmp[y1 * src_sz + x1];

            uint32_t a00 = (c00 >> 24) & 0xFF, r00 = (c00 >> 16) & 0xFF, g00 = (c00 >> 8) & 0xFF, b00 = c00 & 0xFF;
            uint32_t a10 = (c10 >> 24) & 0xFF, r10 = (c10 >> 16) & 0xFF, g10 = (c10 >> 8) & 0xFF, b10 = c10 & 0xFF;
            uint32_t a01 = (c01 >> 24) & 0xFF, r01 = (c01 >> 16) & 0xFF, g01 = (c01 >> 8) & 0xFF, b01 = c01 & 0xFF;
            uint32_t a11 = (c11 >> 24) & 0xFF, r11 = (c11 >> 16) & 0xFF, g11 = (c11 >> 8) & 0xFF, b11 = c11 & 0xFF;

            uint32_t top_a = ((a00 * (256 - x_frac)) + (a10 * x_frac)) >> 8;
            uint32_t top_r = ((r00 * (256 - x_frac)) + (r10 * x_frac)) >> 8;
            uint32_t top_g = ((g00 * (256 - x_frac)) + (g10 * x_frac)) >> 8;
            uint32_t top_b = ((b00 * (256 - x_frac)) + (b10 * x_frac)) >> 8;

            uint32_t bot_a = ((a01 * (256 - x_frac)) + (a11 * x_frac)) >> 8;
            uint32_t bot_r = ((r01 * (256 - x_frac)) + (r11 * x_frac)) >> 8;
            uint32_t bot_g = ((g01 * (256 - x_frac)) + (g11 * x_frac)) >> 8;
            uint32_t bot_b = ((b01 * (256 - x_frac)) + (b11 * x_frac)) >> 8;

            uint32_t a = ((top_a * (256 - y_frac)) + (bot_a * y_frac)) >> 8;
            uint32_t r = ((top_r * (256 - y_frac)) + (bot_r * y_frac)) >> 8;
            uint32_t g = ((top_g * (256 - y_frac)) + (bot_g * y_frac)) >> 8;
            uint32_t b = ((top_b * (256 - y_frac)) + (bot_b * y_frac)) >> 8;

            uint32_t idx = (y * w + x) * 4;
            px[idx + 0] = (uint8_t)b;
            px[idx + 1] = (uint8_t)g;
            px[idx + 2] = (uint8_t)r;
            px[idx + 3] = (uint8_t)a;
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
