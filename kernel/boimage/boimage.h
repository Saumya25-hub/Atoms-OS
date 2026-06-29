#ifndef KERNEL_BOIMAGE_H
#define KERNEL_BOIMAGE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define BOIMAGE_FORMAT_BMP 0
#define BOIMAGE_FORMAT_PNG 1
#define BOIMAGE_FORMAT_UNKNOWN 0xFFFFFFFF

#pragma pack(push, 1)
typedef struct {
    uint16_t type;      // "BM" (0x4D42)
    uint32_t size;
    uint32_t reserved;
    uint32_t offset;
} BMPHeader;

typedef struct {
    uint32_t size;
    int32_t width;
    int32_t height;
    uint16_t planes;
    uint16_t bpp;
    uint32_t compression;
    uint32_t sizeImage;
    int32_t  xPelsPerMeter;
    int32_t  yPelsPerMeter;
    uint32_t clrUsed;
    uint32_t clrImportant;
} BMPInfoHeader;
#pragma pack(pop)

// Core decoded image buffer in kernel memory
typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t format;   // 0 = BMP, 1 = PNG_RGBA
    uint8_t* pixels;   // raw framebuffer-ready buffer (32-bit ARGB/RGBA)
} BOImage;

// Handle returned to UI / OS subsystems
typedef struct {
    int id;
    BOImage* image;
    uint32_t ref_count;
} BOImageHandle;

// External PNG decoder hook signature
typedef int (*PNGDecoderHook)(const uint8_t* in_data, uint32_t in_size, uint8_t** out_pixels, uint32_t* out_width, uint32_t* out_height);

// Initialization & Registration
void BOImage_Init(void);
void BOImage_SetPNGDecoderHook(PNGDecoderHook hook);

// Loader / Decoder APIs
BOImage* BOImage_LoadBMP(const uint8_t* data, uint32_t data_size);
BOImage* BOImage_LoadPNG(const uint8_t* data, uint32_t data_size);
uint32_t BOImage_FormatDetector(const uint8_t* data, uint32_t data_size);

// Cache System APIs
int BOImage_CacheStore(BOImage* img);
BOImageHandle* BOImage_GetImage(int id);
void BOImage_ReleaseImage(int id);

// High-speed Blitter & BOHEART Integration
void BOImage_BlitToFramebuffer(int id, int32_t x, int32_t y);
void BOImage_AlphaBlend(int id, int32_t x, int32_t y);

// ============================================================
// BOIMAGE v2 ENGINE UPGRADE (GPU-Ready, Atlas, Batching)
// ============================================================

#define BOIMAGE_MAX_SPRITES 2048

// Texture Abstraction (GPU-ready base)
typedef struct {
    uint32_t id;
    uint32_t width;
    uint32_t height;
    uint32_t format;   // 0 = ARGB / 1 = RGBA
    uint8_t* data;     // CPU fallback OR GPU staging buffer
} BOTexture;

// Sprite (Batch Unit)
typedef struct {
    BOTexture* texture;
    int32_t x, y;
    int32_t width, height;
    float u1, v1;
    float u2, v2;
} BOSprite;

// Batch Queue
typedef struct {
    BOSprite sprites[BOIMAGE_MAX_SPRITES];
    int32_t count;
} BOBatch;

// Atlas Structure (Cell-based rectangle packing)
typedef struct {
    BOTexture* atlas_texture;
    int32_t cell_size;
    int32_t width_cells;
    int32_t height_cells;
    uint8_t* occupancy_map; // 1 byte per cell (0 = free, 1 = occupied)
} BOAtlas;

// GPU Backend Hook Abstraction
typedef struct {
    void (*upload_texture)(BOTexture* tex);
    void (*draw_batch)(BOBatch* batch);
} BOGPUBackend;

// BOIMAGE v2 APIs
void BOImage_v2_Init(void);
void BOImage_SetGPUBackend(const BOGPUBackend* backend);

// Texture APIs
BOTexture* BOImage_CreateTexture(uint32_t width, uint32_t height, uint32_t format);
void BOImage_DestroyTexture(BOTexture* tex);

// Atlas APIs
BOAtlas* BOImage_CreateAtlas(int32_t cell_size, int32_t width_cells, int32_t height_cells);
void BOImage_DestroyAtlas(BOAtlas* atlas);
bool BOImage_AtlasInsert(BOAtlas* atlas, BOImage* img, float* out_u1, float* out_v1, float* out_u2, float* out_v2);

// Batch & BOHEART Integration APIs
void BOImage_BatchDrawSprite(BOTexture* tex, int32_t x, int32_t y, int32_t w, int32_t h, float u1, float v1, float u2, float v2);
void BOImage_FlushBatch(BOBatch* batch);
void BOImage_BOHeartTickFlush(void);
void BOImage_v2_RunDemo(int32_t screen_x, int32_t screen_y);

#endif // KERNEL_BOIMAGE_H
