#ifndef BOS_GPU_CAPS_H
#define BOS_GPU_CAPS_H

#include <stdint.h>

/* BOS GPU Subsystem Capability Flags (64-bit Bitmask) */
#define BOS_GPU_CAP_NONE           (0ULL)
#define BOS_GPU_CAP_FRAMEBUFFER    (1ULL << 0)   /* Linear raw framebuffer access */
#define BOS_GPU_CAP_VRAM           (1ULL << 1)   /* Dedicated On-card VRAM allocation */
#define BOS_GPU_CAP_DMA            (1ULL << 2)   /* Direct Memory Access (System <-> VRAM) */
#define BOS_GPU_CAP_BLITTER        (1ULL << 3)   /* Hardware 2D Blitter Engine (BitBLT) */
#define BOS_GPU_CAP_SCALING        (1ULL << 4)   /* Hardware Image Scaling / Bilinear Filtering */
#define BOS_GPU_CAP_OVERLAY        (1ULL << 5)   /* Hardware Video / Planes Overlay */
#define BOS_GPU_CAP_HW_CURSOR      (1ULL << 6)   /* Hardware Mouse Cursor Plane */
#define BOS_GPU_CAP_PAGE_FLIP      (1ULL << 7)   /* Hardware Page Flipping */
#define BOS_GPU_CAP_DOUBLE_BUFFER  (1ULL << 8)   /* Double Buffering Support */
#define BOS_GPU_CAP_TRIPLE_BUFFER  (1ULL << 9)   /* Triple Buffering Support */
#define BOS_GPU_CAP_VSYNC          (1ULL << 10)  /* Vertical Synchronization / CRTC Sync */
#define BOS_GPU_CAP_FIFO           (1ULL << 11)  /* Hardware FIFO Command Buffer */

/* Utility macro to check capability */
#define BOS_GPU_HAS_CAP(caps, flag) (((caps) & (flag)) == (flag))

#endif /* BOS_GPU_CAPS_H */
