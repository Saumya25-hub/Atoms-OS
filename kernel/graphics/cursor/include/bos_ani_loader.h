/**
 * @file bos_ani_loader.h
 * @brief Windows RIFF .ANI Animated Cursor Parser Engine
 */

#ifndef BOS_ANI_LOADER_H
#define BOS_ANI_LOADER_H

#include "bos_cursor.h"

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(push, 1)
typedef struct {
    uint32_t cbSize;   /* 36 bytes */
    uint32_t cFrames;  /* Number of unique frames */
    uint32_t cSteps;   /* Number of animation steps */
    uint32_t cx;
    uint32_t cy;
    uint32_t cBitCount;
    uint32_t cPlanes;
    uint32_t jifRate;  /* Default frame duration in jiffies (1 jiffy = 1/60 sec ~16.6ms) */
    uint32_t flags;    /* Bit 0: 1 = Has seq chunk */
} BCE_ANIHEADER;
#pragma pack(pop)

typedef struct {
    bce_cursor_t* cursor;
    uint32_t*     step_rates_ms;
    uint32_t*     seq_indices;
    uint32_t      step_count;
} bce_ani_t;

bce_error_t bos_ani_parse(const uint8_t* data, size_t size, bce_ani_t** out_ani);
void        bos_ani_free(bce_ani_t* ani);

#ifdef __cplusplus
}
#endif

#endif /* BOS_ANI_LOADER_H */
