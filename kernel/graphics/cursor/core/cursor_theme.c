/**
 * @file cursor_theme.c
 * @brief Cursor Theme Engine supporting 15 standard cursor types with W11 Concept theme
 */

#include "../include/bos_cursor_theme.h"
#include "../include/bos_cur_loader.h"
#include "../include/bos_ani_loader.h"
#include "../include/w11_cursor_assets.h"
#include "kernel/core/lib/include/string.h"

extern void* kmalloc(size_t size);
extern void kfree(void* ptr);

static bce_theme_t g_active_theme = {0};

static bce_cursor_t* load_w11_cursor_type(bce_cursor_type_t type) {
    const uint8_t* raw = NULL;
    size_t len = 0;
    bool is_ani = false;

    switch (type) {
        case BCE_CURSOR_ARROW:       raw = g_w11_arrow_data;       len = g_w11_arrow_data_len;       break;
        case BCE_CURSOR_HAND:        raw = g_w11_hand_data;        len = g_w11_hand_data_len;        break;
        case BCE_CURSOR_IBEAM:       raw = g_w11_ibeam_data;       len = g_w11_ibeam_data_len;       break;
        case BCE_CURSOR_CROSSHAIR:   raw = g_w11_crosshair_data;   len = g_w11_crosshair_data_len;   break;
        case BCE_CURSOR_UNAVAILABLE: raw = g_w11_no_data;          len = g_w11_no_data_len;          break;
        case BCE_CURSOR_HELP:        raw = g_w11_help_data;        len = g_w11_help_data_len;        break;
        case BCE_CURSOR_MOVE:        raw = g_w11_sizeall_data;     len = g_w11_sizeall_data_len;     break;
        case BCE_CURSOR_RESIZE_NS:   raw = g_w11_sizens_data;      len = g_w11_sizens_data_len;      break;
        case BCE_CURSOR_RESIZE_WE:   raw = g_w11_sizewe_data;      len = g_w11_sizewe_data_len;      break;
        case BCE_CURSOR_RESIZE_NWSE: raw = g_w11_sizenwse_data;    len = g_w11_sizenwse_data_len;    break;
        case BCE_CURSOR_RESIZE_NESW: raw = g_w11_sizenesw_data;    len = g_w11_sizenesw_data_len;    break;
        case BCE_CURSOR_APPSTARTING: raw = g_w11_appstarting_data; len = g_w11_appstarting_data_len; is_ani = true; break;
        case BCE_CURSOR_WAIT:        raw = g_w11_wait_data;        len = g_w11_wait_data_len;        is_ani = true; break;
        default: break;
    }

    if (raw && len > 0) {
        if (is_ani) {
            bce_ani_t* ani = NULL;
            if (bos_ani_parse(raw, len, &ani) == BCE_OK && ani && ani->cursor) {
                bce_cursor_t* cur = ani->cursor;
                cur->type = type;
                ani->cursor = NULL;
                bos_ani_free(ani);
                return cur;
            }
        } else {
            bce_cursor_t* cur = NULL;
            if (bos_cur_parse(raw, len, &cur) == BCE_OK && cur != NULL) {
                cur->type = type;
                return cur;
            }
        }
    }

    /* Synthetic fallback if asset missing */
    bce_cursor_t* cur = (bce_cursor_t*)kmalloc(sizeof(bce_cursor_t));
    if (!cur) return NULL;
    memset(cur, 0, sizeof(bce_cursor_t));

    cur->type = type;
    cur->frame_count = 1;
    cur->ref_count = 1;
    cur->frames = (bce_frame_t*)kmalloc(sizeof(bce_frame_t));
    if (!cur->frames) {
        kfree(cur);
        return NULL;
    }

    uint32_t w = 32, h = 32;
    cur->frames[0].width = w;
    cur->frames[0].height = h;
    cur->frames[0].hotspot_x = 0;
    cur->frames[0].hotspot_y = 0;
    cur->frames[0].bpp = 32;
    cur->frames[0].argb_pixels = (uint32_t*)kmalloc(w * h * sizeof(uint32_t));
    if (!cur->frames[0].argb_pixels) {
        kfree(cur->frames);
        kfree(cur);
        return NULL;
    }

    uint32_t* pixels = cur->frames[0].argb_pixels;
    for (uint32_t y = 0; y < h; y++) {
        for (uint32_t x = 0; x < w; x++) {
            if (x == y || x == 0 || y == w / 2) {
                pixels[y * w + x] = 0xFFFFFFFFU;
            } else {
                pixels[y * w + x] = 0x00000000U;
            }
        }
    }
    return cur;
}

bce_error_t bos_cursor_theme_init(void) {
    memset(&g_active_theme, 0, sizeof(bce_theme_t));
    strncpy(g_active_theme.name, "Windows 11 Concept Theme", 63);

    for (int i = 0; i < BCE_CURSOR_TYPE_COUNT; i++) {
        g_active_theme.cursors[i] = load_w11_cursor_type((bce_cursor_type_t)i);
    }
    return BCE_OK;
}

void bos_cursor_theme_shutdown(void) {
    for (int i = 0; i < BCE_CURSOR_TYPE_COUNT; i++) {
        if (g_active_theme.cursors[i]) {
            bos_cur_free(g_active_theme.cursors[i]);
            g_active_theme.cursors[i] = NULL;
        }
    }
    memset(&g_active_theme, 0, sizeof(bce_theme_t));
}

bce_error_t bos_cursor_theme_load(const char* theme_name) {
    if (!theme_name) return BCE_ERR_INVALID_PARAM;
    strncpy(g_active_theme.name, theme_name, 63);

    /* Dynamic live theme switch (no reboot needed) */
    for (int i = 0; i < BCE_CURSOR_TYPE_COUNT; i++) {
        if (g_active_theme.cursors[i]) {
            bos_cur_free(g_active_theme.cursors[i]);
        }
        g_active_theme.cursors[i] = load_w11_cursor_type((bce_cursor_type_t)i);
    }
    return BCE_OK;
}

bce_cursor_t* bos_cursor_theme_get_type(bce_cursor_type_t type) {
    if (type >= BCE_CURSOR_TYPE_COUNT) type = BCE_CURSOR_ARROW;
    return g_active_theme.cursors[type];
}

const char* bos_cursor_theme_get_current_name(void) {
    return g_active_theme.name;
}
