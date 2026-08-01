// Engine 8: Texture Manager
#include "../include/agp_api.h"
#include "kernel/core/lib/include/string.h"

extern void* kmalloc(size_t size);
extern void kfree(void* ptr);

typedef struct {
    uint32_t   width;
    uint32_t   height;
    AGPFormat  format;
    uint32_t*  pixels;
    bool       in_use;
} AGPTexture;

static AGPTexture s_texture_pool[AGP_MAX_TEXTURES];

AGPTextureID AGP_CreateTexture(uint32_t w, uint32_t h, AGPFormat fmt, const void* pixels) {
    for (uint32_t i = 0; i < AGP_MAX_TEXTURES; i++) {
        if (!s_texture_pool[i].in_use) {
            AGPTexture* tex = &s_texture_pool[i];
            tex->in_use = true;
            tex->width = w;
            tex->height = h;
            tex->format = fmt;

            size_t bytes = (size_t)w * h * sizeof(uint32_t);
            tex->pixels = (uint32_t*)kmalloc(bytes);
            if (tex->pixels) {
                if (pixels) {
                    memcpy(tex->pixels, pixels, bytes);
                } else {
                    memset(tex->pixels, 0, bytes);
                }
            }
            return i + 1;
        }
    }
    return 0;
}

void AGP_BindTexture(AGPTextureID tex_id) {
    AGPContext* ctx = AGP_GetCurrentContext();
    if (ctx) ctx->active_texture = tex_id;
}

void AGP_DeleteTexture(AGPTextureID tex_id) {
    if (tex_id == 0 || tex_id > AGP_MAX_TEXTURES) return;
    uint32_t idx = tex_id - 1;
    if (s_texture_pool[idx].in_use) {
        if (s_texture_pool[idx].pixels) {
            kfree(s_texture_pool[idx].pixels);
            s_texture_pool[idx].pixels = NULL;
        }
        s_texture_pool[idx].in_use = false;
    }
}
