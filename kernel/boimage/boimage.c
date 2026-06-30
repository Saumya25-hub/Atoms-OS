#include "boimage.h"
#include "kernel/memory/heap/include/heap.h"
#include "bovisual/Include/graphics.h"

#define BOIMAGE_MAX_CACHE 256

static BOImageHandle s_image_cache[BOIMAGE_MAX_CACHE];
static int s_next_image_id = 1;
static PNGDecoderHook g_png_decoder_hook = NULL;

void BOImage_Init(void) {
    for (int i = 0; i < BOIMAGE_MAX_CACHE; i++) {
        s_image_cache[i].id = 0;
        s_image_cache[i].image = NULL;
        s_image_cache[i].ref_count = 0;
    }
}

void BOImage_SetPNGDecoderHook(PNGDecoderHook hook) {
    g_png_decoder_hook = hook;
}

uint32_t BOImage_FormatDetector(const uint8_t* data, uint32_t data_size) {
    if (!data || data_size < 4) return BOIMAGE_FORMAT_UNKNOWN;
    if (data[0] == 0x42 && data[1] == 0x4D) { // "BM"
        return BOIMAGE_FORMAT_BMP;
    }
    if (data[0] == 0x89 && data[1] == 0x50 && data[2] == 0x4E && data[3] == 0x47) { // PNG signature
        return BOIMAGE_FORMAT_PNG;
    }
    return BOIMAGE_FORMAT_UNKNOWN; // Intentionally reject JPEG and others
}

BOImage* BOImage_LoadBMP(const uint8_t* data, uint32_t data_size) {
    if (!data || data_size < sizeof(BMPHeader) + sizeof(BMPInfoHeader)) {
        return NULL;
    }

    const BMPHeader* header = (const BMPHeader*)data;
    if (header->type != 0x4D42) return NULL; // "BM"

    const BMPInfoHeader* info = (const BMPInfoHeader*)(data + sizeof(BMPHeader));
    if (info->compression != 0) return NULL; // Only support uncompressed BMP in v1
    if (info->bpp != 24 && info->bpp != 32) return NULL;

    int32_t width = info->width;
    int32_t height = info->height;
    bool bottom_up = true;
    if (height < 0) {
        height = -height;
        bottom_up = false;
    }

    if (width <= 0 || height <= 0 || width > 4096 || height > 4096) return NULL;

    BOImage* img = (BOImage*)kmalloc(sizeof(BOImage));
    if (!img) return NULL;

    img->width = (uint32_t)width;
    img->height = (uint32_t)height;
    img->format = BOIMAGE_FORMAT_BMP;
    img->pixels = (uint8_t*)kmalloc(width * height * 4);
    if (!img->pixels) {
        kfree(img);
        return NULL;
    }

    const uint8_t* pixel_src = data + header->offset;
    uint32_t bytes_per_pixel = info->bpp / 8;
    uint32_t row_padded = (width * bytes_per_pixel + 3) & (~3);
    uint32_t* dest_pixels = (uint32_t*)img->pixels;

    for (int32_t row = 0; row < height; row++) {
        int32_t src_row = bottom_up ? (height - 1 - row) : row;
        const uint8_t* row_data = pixel_src + (src_row * row_padded);

        for (int32_t col = 0; col < width; col++) {
            uint32_t offset = col * bytes_per_pixel;
            uint8_t b = row_data[offset];
            uint8_t g = row_data[offset + 1];
            uint8_t r = row_data[offset + 2];
            uint8_t a = (bytes_per_pixel == 4) ? row_data[offset + 3] : 0xFF;

            dest_pixels[row * width + col] = (a << 24) | (r << 16) | (g << 8) | b;
        }
    }

    return img;
}

BOImage* BOImage_LoadPNG(const uint8_t* data, uint32_t data_size) {
    if (BOImage_FormatDetector(data, data_size) != BOIMAGE_FORMAT_PNG) {
        return NULL;
    }

    if (!g_png_decoder_hook) {
        return NULL; // External decoder hook required for PNG
    }

    uint8_t* decoded_pixels = NULL;
    uint32_t width = 0, height = 0;
    if (g_png_decoder_hook(data, data_size, &decoded_pixels, &width, &height) != 0 || !decoded_pixels) {
        return NULL;
    }

    BOImage* img = (BOImage*)kmalloc(sizeof(BOImage));
    if (!img) {
        kfree(decoded_pixels);
        return NULL;
    }

    img->width = width;
    img->height = height;
    img->format = BOIMAGE_FORMAT_PNG;
    img->pixels = decoded_pixels;
    return img;
}

int BOImage_CacheStore(BOImage* img) {
    if (!img) return -1;
    for (int i = 0; i < BOIMAGE_MAX_CACHE; i++) {
        if (s_image_cache[i].id == 0 || s_image_cache[i].image == NULL) {
            int new_id = s_next_image_id++;
            s_image_cache[i].id = new_id;
            s_image_cache[i].image = img;
            s_image_cache[i].ref_count = 1;
            return new_id;
        }
    }
    return -1; // Cache full
}

BOImageHandle* BOImage_GetImage(int id) {
    if (id <= 0) return NULL;
    for (int i = 0; i < BOIMAGE_MAX_CACHE; i++) {
        if (s_image_cache[i].id == id && s_image_cache[i].image != NULL) {
            return &s_image_cache[i];
        }
    }
    return NULL;
}

void BOImage_ReleaseImage(int id) {
    BOImageHandle* handle = BOImage_GetImage(id);
    if (handle && handle->ref_count > 0) {
        handle->ref_count--;
        if (handle->ref_count == 0) {
            if (handle->image) {
                if (handle->image->pixels) kfree(handle->image->pixels);
                kfree(handle->image);
            }
            handle->id = 0;
            handle->image = NULL;
        }
    }
}

void BOImage_FreeImage(BOImage* img) {
    if (!img) return;
    if (img->pixels) kfree(img->pixels);
    kfree(img);
}

void BOImage_BlitToFramebuffer(int id, int32_t x, int32_t y) {
    BOImageHandle* handle = BOImage_GetImage(id);
    if (!handle || !handle->image || !handle->image->pixels) return;

    BOImage* img = handle->image;
    const uint32_t* src_pixels = (const uint32_t*)img->pixels;

    for (uint32_t row = 0; row < img->height; row++) {
        for (uint32_t col = 0; col < img->width; col++) {
            uint32_t color = src_pixels[row * img->width + col];
            uint8_t a = (color >> 24) & 0xFF;
            if (a > 0) {
                BOVISUAL_Graphics_PutPixel(x + col, y + row, color);
            }
        }
    }
}

void BOImage_AlphaBlend(int id, int32_t x, int32_t y) {
    BOImageHandle* handle = BOImage_GetImage(id);
    if (!handle || !handle->image || !handle->image->pixels) return;

    BOImage* img = handle->image;
    const uint32_t* src_pixels = (const uint32_t*)img->pixels;

    for (uint32_t row = 0; row < img->height; row++) {
        for (uint32_t col = 0; col < img->width; col++) {
            uint32_t src_color = src_pixels[row * img->width + col];
            uint32_t src_a = (src_color >> 24) & 0xFF;

            if (src_a == 255) {
                BOVISUAL_Graphics_PutPixel(x + col, y + row, src_color);
            } else if (src_a > 0) {
                uint32_t dst_color = BOVISUAL_Graphics_ReadPixel(x + col, y + row);
                uint32_t dst_r = (dst_color >> 16) & 0xFF;
                uint32_t dst_g = (dst_color >> 8) & 0xFF;
                uint32_t dst_b = dst_color & 0xFF;

                uint32_t src_r = (src_color >> 16) & 0xFF;
                uint32_t src_g = (src_color >> 8) & 0xFF;
                uint32_t src_b = src_color & 0xFF;

                uint32_t inv_a = 255 - src_a;
                uint32_t out_r = ((src_r * src_a) + (dst_r * inv_a)) / 255;
                uint32_t out_g = ((src_g * src_a) + (dst_g * inv_a)) / 255;
                uint32_t out_b = ((src_b * src_a) + (dst_b * inv_a)) / 255;

                BOVISUAL_Graphics_PutPixel(x + col, y + row, (0xFF << 24) | (out_r << 16) | (out_g << 8) | out_b);
            }
        }
    }
}

// ============================================================
// BOIMAGE v2 ENGINE IMPLEMENTATION
// ============================================================

static BOBatch s_active_batch;
static BOGPUBackend s_gpu_backend = {NULL, NULL};
static uint32_t s_next_texture_id = 1000;

void BOImage_v2_Init(void) {
    s_active_batch.count = 0;
    s_gpu_backend.upload_texture = NULL;
    s_gpu_backend.draw_batch = NULL;
}

void BOImage_SetGPUBackend(const BOGPUBackend* backend) {
    if (backend) {
        s_gpu_backend = *backend;
    } else {
        s_gpu_backend.upload_texture = NULL;
        s_gpu_backend.draw_batch = NULL;
    }
}

BOTexture* BOImage_CreateTexture(uint32_t width, uint32_t height, uint32_t format) {
    if (width == 0 || height == 0 || width > 4096 || height > 4096) return NULL;
    BOTexture* tex = (BOTexture*)kmalloc(sizeof(BOTexture));
    if (!tex) return NULL;

    tex->id = s_next_texture_id++;
    tex->width = width;
    tex->height = height;
    tex->format = format;
    tex->data = (uint8_t*)kcalloc(width * height, 4);
    if (!tex->data) {
        kfree(tex);
        return NULL;
    }

    if (s_gpu_backend.upload_texture) {
        s_gpu_backend.upload_texture(tex);
    }
    return tex;
}

void BOImage_DestroyTexture(BOTexture* tex) {
    if (!tex) return;
    if (tex->data) kfree(tex->data);
    kfree(tex);
}

BOAtlas* BOImage_CreateAtlas(int32_t cell_size, int32_t width_cells, int32_t height_cells) {
    if (cell_size <= 0 || width_cells <= 0 || height_cells <= 0) return NULL;
    BOAtlas* atlas = (BOAtlas*)kmalloc(sizeof(BOAtlas));
    if (!atlas) return NULL;

    atlas->cell_size = cell_size;
    atlas->width_cells = width_cells;
    atlas->height_cells = height_cells;

    uint32_t total_w = (uint32_t)(cell_size * width_cells);
    uint32_t total_h = (uint32_t)(cell_size * height_cells);

    atlas->atlas_texture = BOImage_CreateTexture(total_w, total_h, 0);
    if (!atlas->atlas_texture) {
        kfree(atlas);
        return NULL;
    }

    atlas->occupancy_map = (uint8_t*)kcalloc((uint32_t)(width_cells * height_cells), 1);
    if (!atlas->occupancy_map) {
        BOImage_DestroyTexture(atlas->atlas_texture);
        kfree(atlas);
        return NULL;
    }

    return atlas;
}

void BOImage_DestroyAtlas(BOAtlas* atlas) {
    if (!atlas) return;
    if (atlas->occupancy_map) kfree(atlas->occupancy_map);
    if (atlas->atlas_texture) BOImage_DestroyTexture(atlas->atlas_texture);
    kfree(atlas);
}

bool BOImage_AtlasInsert(BOAtlas* atlas, BOImage* img, float* out_u1, float* out_v1, float* out_u2, float* out_v2) {
    if (!atlas || !img || !img->pixels) return false;

    int32_t req_w = (int32_t)((img->width + atlas->cell_size - 1) / atlas->cell_size);
    int32_t req_h = (int32_t)((img->height + atlas->cell_size - 1) / atlas->cell_size);

    if (req_w > atlas->width_cells || req_h > atlas->height_cells) return false;

    // Scan occupancy grid for contiguous block
    for (int32_t cy = 0; cy <= atlas->height_cells - req_h; cy++) {
        for (int32_t cx = 0; cx <= atlas->width_cells - req_w; cx++) {
            bool fits = true;
            for (int32_t dy = 0; dy < req_h; dy++) {
                for (int32_t dx = 0; dx < req_w; dx++) {
                    if (atlas->occupancy_map[(cy + dy) * atlas->width_cells + (cx + dx)] != 0) {
                        fits = false;
                        break;
                    }
                }
                if (!fits) break;
            }

            if (fits) {
                // Mark occupied
                for (int32_t dy = 0; dy < req_h; dy++) {
                    for (int32_t dx = 0; dx < req_w; dx++) {
                        atlas->occupancy_map[(cy + dy) * atlas->width_cells + (cx + dx)] = 1;
                    }
                }

                // Copy image pixels into atlas texture buffer
                uint32_t* dest_buf = (uint32_t*)atlas->atlas_texture->data;
                uint32_t* src_buf = (uint32_t*)img->pixels;
                int32_t start_px = cx * atlas->cell_size;
                int32_t start_py = cy * atlas->cell_size;

                for (uint32_t r = 0; r < img->height; r++) {
                    for (uint32_t c = 0; c < img->width; c++) {
                        dest_buf[(start_py + r) * atlas->atlas_texture->width + (start_px + c)] = src_buf[r * img->width + c];
                    }
                }

                if (s_gpu_backend.upload_texture) {
                    s_gpu_backend.upload_texture(atlas->atlas_texture);
                }

                // Compute normalized UV coordinates
                if (out_u1) *out_u1 = (float)start_px / (float)atlas->atlas_texture->width;
                if (out_v1) *out_v1 = (float)start_py / (float)atlas->atlas_texture->height;
                if (out_u2) *out_u2 = (float)(start_px + img->width) / (float)atlas->atlas_texture->width;
                if (out_v2) *out_v2 = (float)(start_py + img->height) / (float)atlas->atlas_texture->height;

                return true;
            }
        }
    }
    return false; // No fitting region found
}

void BOImage_BatchDrawSprite(BOTexture* tex, int32_t x, int32_t y, int32_t w, int32_t h, float u1, float v1, float u2, float v2) {
    if (!tex || s_active_batch.count >= BOIMAGE_MAX_SPRITES) {
        if (s_active_batch.count >= BOIMAGE_MAX_SPRITES) {
            BOImage_FlushBatch(&s_active_batch);
        }
        if (!tex) return;
    }

    BOSprite* s = &s_active_batch.sprites[s_active_batch.count++];
    s->texture = tex;
    s->x = x;
    s->y = y;
    s->width = w;
    s->height = h;
    s->u1 = u1;
    s->v1 = v1;
    s->u2 = u2;
    s->v2 = v2;
}

void BOImage_FlushBatch(BOBatch* batch) {
    if (!batch || batch->count == 0) return;

    if (s_gpu_backend.draw_batch) {
        s_gpu_backend.draw_batch(batch);
        batch->count = 0;
        return;
    }

    // Fast CPU fallback renderer for sprite batch
    for (int32_t i = 0; i < batch->count; i++) {
        BOSprite* s = &batch->sprites[i];
        if (!s->texture || !s->texture->data || s->width <= 0 || s->height <= 0) continue;

        uint32_t* tex_data = (uint32_t*)s->texture->data;
        int32_t tex_w = (int32_t)s->texture->width;
        int32_t tex_h = (int32_t)s->texture->height;

        int32_t src_x1 = (int32_t)(s->u1 * tex_w);
        int32_t src_y1 = (int32_t)(s->v1 * tex_h);
        int32_t src_x2 = (int32_t)(s->u2 * tex_w);
        int32_t src_y2 = (int32_t)(s->v2 * tex_h);

        for (int32_t dy = 0; dy < s->height; dy++) {
            int32_t sy = src_y1 + (dy * (src_y2 - src_y1)) / s->height;
            if (sy < 0 || sy >= tex_h) continue;

            for (int32_t dx = 0; dx < s->width; dx++) {
                int32_t sx = src_x1 + (dx * (src_x2 - src_x1)) / s->width;
                if (sx < 0 || sx >= tex_w) continue;

                uint32_t color = tex_data[sy * tex_w + sx];
                uint8_t a = (color >> 24) & 0xFF;
                if (a > 0) {
                    BOVISUAL_Graphics_PutPixel(s->x + dx, s->y + dy, color);
                }
            }
        }
    }

    batch->count = 0;
}

void BOImage_BOHeartTickFlush(void) {
    BOImage_FlushBatch(&s_active_batch);
}

#include "kernel/boasset/boasset.h"

// ============================================================
// BOIMAGE v2.5 QUALITY ENGINE (Phase 1)
// ============================================================

// Module 1: Sampling Engine (Fixed-point Bilinear & Nearest interpolation)
uint32_t BOImage_SamplePixel(const BOTexture* tex, float u, float v, BOImageScalingFilter filter) {
    if (!tex || !tex->data) return 0;
    if (u < 0.0f) u = 0.0f;
    if (u > 1.0f) u = 1.0f;
    if (v < 0.0f) v = 0.0f;
    if (v > 1.0f) v = 1.0f;

    uint32_t* px = (uint32_t*)tex->data;
    uint32_t w = tex->width;
    uint32_t h = tex->height;
    if (w == 0 || h == 0) return 0;

    if (filter == BO_FILTER_NEAREST) {
        uint32_t tx = (uint32_t)(u * (w - 1) + 0.5f);
        uint32_t ty = (uint32_t)(v * (h - 1) + 0.5f);
        if (tx >= w) tx = w - 1;
        if (ty >= h) ty = h - 1;
        return px[ty * w + tx];
    }

    // Fixed-point 8-bit Bilinear Filtering
    float fx_full = u * (float)(w - 1);
    float fy_full = v * (float)(h - 1);
    uint32_t x0 = (uint32_t)fx_full;
    uint32_t y0 = (uint32_t)fy_full;
    uint32_t x1 = (x0 + 1 < w) ? x0 + 1 : x0;
    uint32_t y1 = (y0 + 1 < h) ? y0 + 1 : y0;

    uint32_t fx = (uint32_t)((fx_full - (float)x0) * 256.0f);
    uint32_t fy = (uint32_t)((fy_full - (float)y0) * 256.0f);
    if (fx > 255) fx = 255;
    if (fy > 255) fy = 255;

    uint32_t c00 = px[y0 * w + x0];
    uint32_t c10 = px[y0 * w + x1];
    uint32_t c01 = px[y1 * w + x0];
    uint32_t c11 = px[y1 * w + x1];

    uint32_t a00 = (c00 >> 24) & 0xFF, r00 = (c00 >> 16) & 0xFF, g00 = (c00 >> 8) & 0xFF, b00 = c00 & 0xFF;
    uint32_t a10 = (c10 >> 24) & 0xFF, r10 = (c10 >> 16) & 0xFF, g10 = (c10 >> 8) & 0xFF, b10 = c10 & 0xFF;
    uint32_t a01 = (c01 >> 24) & 0xFF, r01 = (c01 >> 16) & 0xFF, g01 = (c01 >> 8) & 0xFF, b01 = c01 & 0xFF;
    uint32_t a11 = (c11 >> 24) & 0xFF, r11 = (c11 >> 16) & 0xFF, g11 = (c11 >> 8) & 0xFF, b11 = c11 & 0xFF;

    uint32_t a0 = a00 + (((a10 - (int32_t)a00) * (int32_t)fx) >> 8);
    uint32_t r0 = r00 + (((r10 - (int32_t)r00) * (int32_t)fx) >> 8);
    uint32_t g0 = g00 + (((g10 - (int32_t)g00) * (int32_t)fx) >> 8);
    uint32_t b0 = b00 + (((b10 - (int32_t)b00) * (int32_t)fx) >> 8);

    uint32_t a1 = a01 + (((a11 - (int32_t)a01) * (int32_t)fx) >> 8);
    uint32_t r1 = r01 + (((r11 - (int32_t)r01) * (int32_t)fx) >> 8);
    uint32_t g1 = g01 + (((g11 - (int32_t)g01) * (int32_t)fx) >> 8);
    uint32_t b1 = b01 + (((b11 - (int32_t)b01) * (int32_t)fx) >> 8);

    uint32_t a = a0 + (((a1 - (int32_t)a0) * (int32_t)fy) >> 8);
    uint32_t r = r0 + (((r1 - (int32_t)r0) * (int32_t)fy) >> 8);
    uint32_t g = g0 + (((g1 - (int32_t)g0) * (int32_t)fy) >> 8);
    uint32_t b = b0 + (((b1 - (int32_t)b0) * (int32_t)fy) >> 8);

    return (a << 24) | (r << 16) | (g << 8) | b;
}

// Module 2: Blend Engine (Accurate Alpha Blending & Premultiplied support)
uint32_t BOImage_BlendPixel(uint32_t dst_argb, uint32_t src_argb) {
    uint32_t src_a = (src_argb >> 24) & 0xFF;
    if (src_a == 0) return dst_argb;
    if (src_a == 255) return src_argb;

    uint32_t inv_a = 255 - src_a;
    uint32_t src_r = (src_argb >> 16) & 0xFF;
    uint32_t src_g = (src_argb >> 8) & 0xFF;
    uint32_t src_b = src_argb & 0xFF;

    uint32_t dst_a = (dst_argb >> 24) & 0xFF;
    uint32_t dst_r = (dst_argb >> 16) & 0xFF;
    uint32_t dst_g = (dst_argb >> 8) & 0xFF;
    uint32_t dst_b = dst_argb & 0xFF;

    uint32_t out_a = src_a + ((dst_a * inv_a) >> 8);
    uint32_t out_r = ((src_r * src_a) + (dst_r * inv_a)) >> 8;
    uint32_t out_g = ((src_g * src_a) + (dst_g * inv_a)) >> 8;
    uint32_t out_b = ((src_b * src_a) + (dst_b * inv_a)) >> 8;

    return (out_a << 24) | (out_r << 16) | (out_g << 8) | out_b;
}

// Module 3: Pixel Snapping Engine
void BOImage_SnapBounds(float x, float y, float w, float h, int32_t* out_x, int32_t* out_y, int32_t* out_w, int32_t* out_h) {
    if (out_x) *out_x = (int32_t)(x >= 0.0f ? x + 0.5f : x - 0.5f);
    if (out_y) *out_y = (int32_t)(y >= 0.0f ? y + 0.5f : y - 0.5f);
    if (out_w) *out_w = (int32_t)(w >= 0.0f ? w + 0.5f : w - 0.5f);
    if (out_h) *out_h = (int32_t)(h >= 0.0f ? h + 0.5f : h - 0.5f);
}

// Module 4: Scaling & Raster Engine
void BOImage_AtlasDrawEx(BOTexture* tex, int32_t x, int32_t y, int32_t w, int32_t h, float u1, float v1, float u2, float v2, BOImageScalingFilter filter) {
    if (!tex || !tex->data || w <= 0 || h <= 0) return;

    int32_t sx, sy, sw, sh;
    BOImage_SnapBounds((float)x, (float)y, (float)w, (float)h, &sx, &sy, &sw, &sh);
    if (sw <= 0 || sh <= 0) return;

    for (int32_t dy = 0; dy < sh; dy++) {
        int32_t screen_y = sy + dy;
        float v = v1 + (v2 - v1) * ((float)dy / (float)(sh > 1 ? sh - 1 : 1));

        for (int32_t dx = 0; dx < sw; dx++) {
            int32_t screen_x = sx + dx;
            float u = u1 + (u2 - u1) * ((float)dx / (float)(sw > 1 ? sw - 1 : 1));

            uint32_t src_color = BOImage_SamplePixel(tex, u, v, filter);
            uint32_t src_a = (src_color >> 24) & 0xFF;
            if (src_a == 0) continue;

            if (src_a == 255) {
                BOVISUAL_Graphics_PutPixel(screen_x, screen_y, src_color);
            } else {
                uint32_t dst_color = BOVISUAL_Graphics_ReadPixel(screen_x, screen_y);
                uint32_t blended = BOImage_BlendPixel(dst_color, src_color);
                BOVISUAL_Graphics_PutPixel(screen_x, screen_y, blended);
            }
        }
    }
}

void BOImage_DrawEx(BOImage* image, int32_t x, int32_t y, int32_t width, int32_t height, BOImageScalingFilter filter) {
    if (!image || !image->pixels || width <= 0 || height <= 0) return;

    BOTexture wrapper;
    wrapper.id = 0;
    wrapper.width = image->width;
    wrapper.height = image->height;
    wrapper.format = 0;
    wrapper.data = image->pixels;

    BOImage_AtlasDrawEx(&wrapper, x, y, width, height, 0.0f, 0.0f, 1.0f, 1.0f, filter);
}

void BOImage_v2_RunDemo(int32_t screen_x, int32_t screen_y) {
    // 1. Draw standard 1:1 unscaled icons via BOASSET
    BOAsset_DrawAsset(ICON_FOLDER,   screen_x,       screen_y,      32, 32);
    BOAsset_DrawAsset(ICON_FILE,     screen_x + 50,  screen_y,      32, 32);
    BOAsset_DrawAsset(ICON_TERMINAL, screen_x + 100, screen_y,      32, 32);
    BOAsset_DrawAsset(ICON_CLOSE,    screen_x + 150, screen_y + 4,  24, 24);

    // 2. Showcase BOIMAGE v2.5 Quality Engine: Draw 64x64 ATOMS logo scaled up to 128x128 with Bilinear Filtering!
    BOAssetHandle* logo_handle = BOAsset_Get(ASSET_LOGO);
    if (logo_handle && logo_handle->image_data) {
        BOImage_DrawEx(logo_handle->image_data, screen_x, screen_y + 48, 128, 128, BO_FILTER_BILINEAR);
    }
}
