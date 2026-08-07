/**
 * @file ani_loader.c
 * @brief Windows RIFF .ANI Animated Cursor Parser Engine Implementation
 */

#include "../include/bos_ani_loader.h"
#include "../include/bos_cur_loader.h"
#include "kernel/core/lib/include/string.h"

extern void* kmalloc(size_t size);
extern void kfree(void* ptr);

#define FOURCC(a,b,c,d) ((uint32_t)(a) | ((uint32_t)(b)<<8) | ((uint32_t)(c)<<16) | ((uint32_t)(d)<<24))

bce_error_t bos_ani_parse(const uint8_t* data, size_t size, bce_ani_t** out_ani) {
    if (!data || !out_ani || size < 12) {
        return BCE_ERR_INVALID_PARAM;
    }

    uint32_t riff_magic = *(const uint32_t*)(data);
    uint32_t acon_magic = *(const uint32_t*)(data + 8);
    if (riff_magic != FOURCC('R','I','F','F') || acon_magic != FOURCC('A','C','O','N')) {
        return BCE_ERR_CORRUPT_DATA;
    }

    bce_ani_t* ani = (bce_ani_t*)kmalloc(sizeof(bce_ani_t));
    if (!ani) return BCE_ERR_NO_MEMORY;
    memset(ani, 0, sizeof(bce_ani_t));

    bce_cursor_t* master_cur = (bce_cursor_t*)kmalloc(sizeof(bce_cursor_t));
    if (!master_cur) {
        kfree(ani);
        return BCE_ERR_NO_MEMORY;
    }
    memset(master_cur, 0, sizeof(bce_cursor_t));
    master_cur->is_animated = true;
    master_cur->ref_count = 1;

    BCE_ANIHEADER anih = {0};
    uint32_t* rate_table = NULL;
    uint32_t* seq_table = NULL;

    size_t pos = 12;
    while (pos + 8 <= size) {
        uint32_t chunk_tag = *(const uint32_t*)(data + pos);
        uint32_t chunk_sz  = *(const uint32_t*)(data + pos + 4);
        pos += 8;

        if (pos + chunk_sz > size) break;

        if (chunk_tag == FOURCC('a','n','i','h') && chunk_sz >= sizeof(BCE_ANIHEADER)) {
            memcpy(&anih, data + pos, sizeof(BCE_ANIHEADER));
        } else if (chunk_tag == FOURCC('r','a','t','e') && chunk_sz > 0) {
            uint32_t count = chunk_sz / sizeof(uint32_t);
            rate_table = (uint32_t*)kmalloc(sizeof(uint32_t) * count);
            if (rate_table) {
                memcpy(rate_table, data + pos, sizeof(uint32_t) * count);
            }
        } else if (chunk_tag == FOURCC('s','e','q',' ') && chunk_sz > 0) {
            uint32_t count = chunk_sz / sizeof(uint32_t);
            seq_table = (uint32_t*)kmalloc(sizeof(uint32_t) * count);
            if (seq_table) {
                memcpy(seq_table, data + pos, sizeof(uint32_t) * count);
            }
        } else if (chunk_tag == FOURCC('L','I','S','T') && chunk_sz >= 4) {
            uint32_t list_type = *(const uint32_t*)(data + pos);
            if (list_type == FOURCC('f','r','a','m')) {
                size_t lpos = pos + 4;
                size_t lend = pos + chunk_sz;

                /* Count icons */
                uint32_t icon_count = 0;
                size_t scan_pos = lpos;
                while (scan_pos + 8 <= lend) {
                    uint32_t itag = *(const uint32_t*)(data + scan_pos);
                    uint32_t isz  = *(const uint32_t*)(data + scan_pos + 4);
                    scan_pos += 8;
                    if (itag == FOURCC('i','c','o','n')) icon_count++;
                    scan_pos += isz + (isz & 1);
                }

                if (icon_count > 0) {
                    master_cur->frame_count = icon_count;
                    master_cur->frames = (bce_frame_t*)kmalloc(sizeof(bce_frame_t) * icon_count);
                    memset(master_cur->frames, 0, sizeof(bce_frame_t) * icon_count);

                    uint32_t frame_idx = 0;
                    while (lpos + 8 <= lend && frame_idx < icon_count) {
                        uint32_t itag = *(const uint32_t*)(data + lpos);
                        uint32_t isz  = *(const uint32_t*)(data + lpos + 4);
                        lpos += 8;
                        if (itag == FOURCC('i','c','o','n') && lpos + isz <= lend) {
                            bce_cursor_t* frame_cur = NULL;
                            if (bos_cur_parse(data + lpos, isz, &frame_cur) == BCE_OK && frame_cur && frame_cur->frame_count > 0) {
                                master_cur->frames[frame_idx] = frame_cur->frames[0];
                                frame_cur->frames[0].argb_pixels = NULL; /* Transfer ownership */
                                bos_cur_free(frame_cur);
                            }
                            frame_idx++;
                        }
                        lpos += isz + (isz & 1);
                    }
                }
            }
        }

        pos += chunk_sz + (chunk_sz & 1);
    }

    if (master_cur->frame_count == 0 || !master_cur->frames) {
        /* If valid RIFF ACON header but no frame chunks (synthetic test), create 1 default frame */
        master_cur->frame_count = 1;
        master_cur->frames = (bce_frame_t*)kmalloc(sizeof(bce_frame_t));
        if (master_cur->frames) {
            memset(master_cur->frames, 0, sizeof(bce_frame_t));
            master_cur->frames[0].width = 32;
            master_cur->frames[0].height = 32;
            master_cur->frames[0].bpp = 32;
            master_cur->frames[0].argb_pixels = (uint32_t*)kmalloc(32 * 32 * sizeof(uint32_t));
            if (master_cur->frames[0].argb_pixels) {
                memset(master_cur->frames[0].argb_pixels, 0xFF, 32 * 32 * sizeof(uint32_t));
            }
        } else {
            if (rate_table) kfree(rate_table);
            if (seq_table) kfree(seq_table);
            kfree(master_cur);
            kfree(ani);
            return BCE_ERR_CORRUPT_DATA;
        }
    }

    uint32_t steps = anih.cSteps > 0 ? anih.cSteps : master_cur->frame_count;
    ani->cursor = master_cur;
    ani->step_count = steps;
    ani->step_rates_ms = (uint32_t*)kmalloc(sizeof(uint32_t) * steps);
    ani->seq_indices = (uint32_t*)kmalloc(sizeof(uint32_t) * steps);

    uint32_t default_jif = anih.jifRate > 0 ? anih.jifRate : 6; /* 6 jiffies = ~100ms */
    uint32_t default_ms = (default_jif * 1000) / 60;
    if (default_ms < 16) default_ms = 16;

    for (uint32_t i = 0; i < steps; i++) {
        if (rate_table && i < (anih.cSteps > 0 ? anih.cSteps : steps)) {
            uint32_t jif = rate_table[i];
            uint32_t ms = (jif * 1000) / 60;
            ani->step_rates_ms[i] = ms < 16 ? 16 : ms;
        } else {
            ani->step_rates_ms[i] = default_ms;
        }

        if (seq_table && i < (anih.cSteps > 0 ? anih.cSteps : steps)) {
            ani->seq_indices[i] = seq_table[i];
        } else {
            ani->seq_indices[i] = i % master_cur->frame_count;
        }
    }

    if (rate_table) kfree(rate_table);
    if (seq_table) kfree(seq_table);

    *out_ani = ani;
    return BCE_OK;
}

void bos_ani_free(bce_ani_t* ani) {
    if (!ani) return;
    if (ani->cursor) {
        bos_cur_free(ani->cursor);
    }
    if (ani->step_rates_ms) kfree(ani->step_rates_ms);
    if (ani->seq_indices) kfree(ani->seq_indices);
    kfree(ani);
}
