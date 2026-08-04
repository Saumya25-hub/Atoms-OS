#ifndef BOSPECTRA_FRAME_H
#define BOSPECTRA_FRAME_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "../../include/bospectra_types.h"

typedef enum {
    BOSPECTRA_PIXEL_FORMAT_UNKNOWN = 0,
    BOSPECTRA_PIXEL_FORMAT_ARGB32,      // 32-bit ARGB (8:8:8:8)
    BOSPECTRA_PIXEL_FORMAT_RGBA32,      // 32-bit RGBA (8:8:8:8)
    BOSPECTRA_PIXEL_FORMAT_RGB24,       // 24-bit RGB (8:8:8)
    BOSPECTRA_PIXEL_FORMAT_YUV420P,     // Planar YUV 4:2:0
    BOSPECTRA_PIXEL_FORMAT_NV12,        // Bi-planar Y/UV 4:2:0
    BOSPECTRA_PIXEL_FORMAT_YUY2         // Packed YUYV 4:2:2
} bospectra_pixel_format_t;

#define BOSPECTRA_FRAME_FLAG_KEYFRAME   0x0001U
#define BOSPECTRA_FRAME_FLAG_INTERLACED 0x0002U
#define BOSPECTRA_FRAME_FLAG_CORRUPT    0x0004U

typedef struct BOSFrame {
    uint32_t                 frame_id;
    uint32_t                 width;
    uint32_t                 height;
    uint32_t                 linesize[4];       // Row stride per plane in bytes
    bospectra_pixel_format_t format;
    uint8_t*                 data[4];           // Pointers to plane buffers (Y, U, V, A)
    size_t                   buffer_size;       // Total allocated payload size
    uint64_t                 pts;               // Presentation Timestamp (microseconds)
    uint64_t                 dts;               // Decode Timestamp (microseconds)
    uint64_t                 duration_us;       // Frame duration
    uint32_t                 flags;             // Keyframe, interlaced, corrupt flags
    uint32_t                 ref_count;         // Shared ownership reference counter
    uint32_t                 pool_index;        // Index in pre-allocated pool array
    bool                     is_allocated;
} BOSFrame;

// Frame Lifecycle & Reference Counting APIs
void              bospectra_frame_ref(BOSFrame* frame);
bospectra_error_t bospectra_frame_unref(BOSFrame* frame);
bool              bospectra_frame_is_valid(const BOSFrame* frame);

#endif // BOSPECTRA_FRAME_H
