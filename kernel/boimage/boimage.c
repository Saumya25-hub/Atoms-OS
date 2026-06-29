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

void BOImage_v2_RunDemo(int32_t screen_x, int32_t screen_y) {
    static BOAtlas* s_demo_atlas = NULL;
    static float u1 = 0, v1 = 0, u2 = 0, v2 = 0;
    static bool s_demo_initialized = false;

    if (!s_demo_initialized) {
        s_demo_atlas = BOImage_CreateAtlas(64, 4, 4);
        if (!s_demo_atlas) return;

        uint32_t img_w = 64;
        uint32_t img_h = 64;
        uint32_t data_size = sizeof(BMPHeader) + sizeof(BMPInfoHeader) + (img_w * img_h * 4);
        uint8_t* bmp_data = (uint8_t*)kmalloc(data_size);
        if (!bmp_data) return;

        BMPHeader* fh = (BMPHeader*)bmp_data;
        fh->type = 0x4D42; // "BM"
        fh->size = data_size;
        fh->reserved = 0;
        fh->offset = sizeof(BMPHeader) + sizeof(BMPInfoHeader);

        BMPInfoHeader* ih = (BMPInfoHeader*)(bmp_data + sizeof(BMPHeader));
        ih->size = sizeof(BMPInfoHeader);
        ih->width = img_w;
        ih->height = -(int32_t)img_h; // Top-down
        ih->planes = 1;
        ih->bpp = 32;
        ih->compression = 0;
        ih->sizeImage = img_w * img_h * 4;
        ih->xPelsPerMeter = 2835;
        ih->yPelsPerMeter = 2835;
        ih->clrUsed = 0;
        ih->clrImportant = 0;

        uint8_t* px = bmp_data + fh->offset;
        for (uint32_t y = 0; y < img_h; y++) {
            for (uint32_t x = 0; x < img_w; x++) {
                uint32_t idx = (y * img_w + x) * 4;
                float fx = (float)x - 31.5f;
                float fy = (float)y - 31.5f;
                float r_sq = fx*fx + fy*fy;

                uint8_t b = 0, g = 0, red = 0, a = 0;

                // Tech dark badge circle with neon border
                if (r_sq < 30.0f * 30.0f) {
                    b = 50; g = 25; red = 20; a = 240;
                }
                if (r_sq >= 28.0f * 28.0f && r_sq <= 30.0f * 30.0f) {
                    b = 255; g = 180; red = 0; a = 255; // Neon Cyan boundary ring
                }

                // Orbital ring 1: horizontal ellipse
                float eq1 = (fx*fx)/(24.0f*24.0f) + (fy*fy)/(8.0f*8.0f);
                if (eq1 >= 0.75f && eq1 <= 1.25f) {
                    b = 255; g = 255; red = 0; a = 255; // Neon Cyan orbit
                }

                // Orbital ring 2: rotated 60 deg
                float rx2 = fx * 0.5f + fy * 0.866f;
                float ry2 = -fx * 0.866f + fy * 0.5f;
                float eq2 = (rx2*rx2)/(24.0f*24.0f) + (ry2*ry2)/(8.0f*8.0f);
                if (eq2 >= 0.75f && eq2 <= 1.25f) {
                    b = 255; g = 0; red = 255; a = 255; // Neon Magenta orbit
                }

                // Orbital ring 3: rotated 120 deg
                float rx3 = -fx * 0.5f + fy * 0.866f;
                float ry3 = -fx * 0.866f - fy * 0.5f;
                float eq3 = (rx3*rx3)/(24.0f*24.0f) + (ry3*ry3)/(8.0f*8.0f);
                if (eq3 >= 0.75f && eq3 <= 1.25f) {
                    b = 0; g = 255; red = 50; a = 255; // Neon Green orbit
                }

                // Glowing Nucleus at center (Gold sphere + halo)
                if (r_sq < 7.0f * 7.0f) {
                    b = 0; g = 215; red = 255; a = 255; // Solid Gold core
                } else if (r_sq < 11.0f * 11.0f) {
                    b = 0; g = 140; red = 255; a = 200; // Orange glow halo
                }

                px[idx + 0] = b;
                px[idx + 1] = g;
                px[idx + 2] = red;
                px[idx + 3] = a;
            }
        }

        BOImage* loaded_img = BOImage_LoadBMP(bmp_data, data_size);
        kfree(bmp_data);
        if (loaded_img) {
            BOImage_AtlasInsert(s_demo_atlas, loaded_img, &u1, &v1, &u2, &v2);
            s_demo_initialized = true;
        }
    }

    if (s_demo_initialized && s_demo_atlas) {
        for (int row = 0; row < 2; row++) {
            for (int col = 0; col < 3; col++) {
                BOImage_BatchDrawSprite(s_demo_atlas->atlas_texture, 
                                        screen_x + col * 80, 
                                        screen_y + row * 80, 
                                        64, 64, u1, v1, u2, v2);
            }
        }
    }
}
