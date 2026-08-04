#include "texture_pool.h"
#include "../../memory/bospectra_memory.h"
#include "kernel/core/lib/include/string.h"

typedef struct {
    BOTexture texture;
    uint32_t  pool_idx;
    bool      in_use;
} TexturePoolSlot;

static TexturePoolSlot g_texture_pool[BOSPECTRA_TEXTURE_POOL_SIZE];
static uint32_t g_texture_pool_active = 0;
static uint32_t g_texture_pool_peak = 0;
static bool     g_texture_pool_initialized = false;

void bospectra_texture_pool_init(void) {
    memset(g_texture_pool, 0, sizeof(g_texture_pool));

    for (uint32_t i = 0; i < BOSPECTRA_TEXTURE_POOL_SIZE; i++) {
        g_texture_pool[i].pool_idx = i;
        g_texture_pool[i].in_use = false;
        g_texture_pool[i].texture.id = i + 1;
        g_texture_pool[i].texture.width = 1920;
        g_texture_pool[i].texture.height = 1080;
        g_texture_pool[i].texture.format = 0; // ARGB32
        g_texture_pool[i].texture.data = (uint8_t*)bospectra_mem_alloc_aligned(1920 * 1080 * 4, BOSPECTRA_DEFAULT_ALIGNMENT, "PreallocatedTextureBuf");
        g_texture_pool[i].texture.owns_object = false;
        g_texture_pool[i].texture.owns_data = false;
    }

    g_texture_pool_active = 0;
    g_texture_pool_peak = 0;
    g_texture_pool_initialized = true;
}

void bospectra_texture_pool_shutdown(void) {
    for (uint32_t i = 0; i < BOSPECTRA_TEXTURE_POOL_SIZE; i++) {
        if (g_texture_pool[i].texture.data) {
            bospectra_mem_free(g_texture_pool[i].texture.data);
            g_texture_pool[i].texture.data = NULL;
        }
    }
    memset(g_texture_pool, 0, sizeof(g_texture_pool));
    g_texture_pool_active = 0;
    g_texture_pool_peak = 0;
    g_texture_pool_initialized = false;
}

bospectra_error_t bospectra_texture_acquire(uint32_t width, uint32_t height, BOTexture** out_texture) {
    if (!g_texture_pool_initialized) return BOSPECTRA_ERR_NOT_INITIALIZED;
    if (!out_texture) return BOSPECTRA_ERR_INVALID_ARGUMENT;

    for (uint32_t i = 0; i < BOSPECTRA_TEXTURE_POOL_SIZE; i++) {
        if (!g_texture_pool[i].in_use) {
            TexturePoolSlot* slot = &g_texture_pool[i];
            slot->in_use = true;
            slot->texture.width = (width > 0) ? width : 1920;
            slot->texture.height = (height > 0) ? height : 1080;

            g_texture_pool_active++;
            if (g_texture_pool_active > g_texture_pool_peak) {
                g_texture_pool_peak = g_texture_pool_active;
            }

            *out_texture = &slot->texture;
            return BOSPECTRA_SUCCESS;
        }
    }

    *out_texture = NULL;
    return BOSPECTRA_ERR_OUT_OF_MEMORY;
}

bospectra_error_t bospectra_texture_release(BOTexture* texture) {
    if (!g_texture_pool_initialized) return BOSPECTRA_ERR_NOT_INITIALIZED;
    if (!texture) return BOSPECTRA_ERR_INVALID_ARGUMENT;

    for (uint32_t i = 0; i < BOSPECTRA_TEXTURE_POOL_SIZE; i++) {
        if (&g_texture_pool[i].texture == texture) {
            if (!g_texture_pool[i].in_use) {
                return BOSPECTRA_ERR_HANDLE_INVALID; // Double free protection
            }
            g_texture_pool[i].in_use = false;
            if (g_texture_pool_active > 0) g_texture_pool_active--;
            return BOSPECTRA_SUCCESS;
        }
    }

    return BOSPECTRA_ERR_HANDLE_INVALID;
}

void bospectra_texture_pool_get_counts(uint32_t* active, uint32_t* peak, uint32_t* capacity) {
    if (active) *active = g_texture_pool_active;
    if (peak) *peak = g_texture_pool_peak;
    if (capacity) *capacity = BOSPECTRA_TEXTURE_POOL_SIZE;
}
