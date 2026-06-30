#include "asset_cache.h"
#include "kernel/memory/heap/include/heap.h"

#define ASSET_CACHE_MAX_ENTRIES 256

static BOAssetHandle s_cache_table[ASSET_CACHE_MAX_ENTRIES];

static void copy_string(char* dest, const char* src, uint32_t max_len) {
    uint32_t i = 0;
    while (src && src[i] != '\0' && i < max_len - 1) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

void BOAssetCache_Initialize(void) {
    for (int i = 0; i < ASSET_CACHE_MAX_ENTRIES; i++) {
        s_cache_table[i].id = BOASSET_ID_NONE;
        s_cache_table[i].loaded = false;
        s_cache_table[i].ref_count = 0;
        s_cache_table[i].image_data = NULL;
    }
}

void BOAssetCache_Clear(void) {
    for (int i = 0; i < ASSET_CACHE_MAX_ENTRIES; i++) {
        if (s_cache_table[i].id != BOASSET_ID_NONE && s_cache_table[i].image_data) {
            BOImage_FreeImage(s_cache_table[i].image_data);
            s_cache_table[i].image_data = NULL;
        }
        s_cache_table[i].id = BOASSET_ID_NONE;
        s_cache_table[i].loaded = false;
        s_cache_table[i].ref_count = 0;
    }
}

BOAssetHandle* BOAssetCache_Lookup(uint32_t asset_id) {
    if (asset_id == BOASSET_ID_NONE) return NULL;
    for (int i = 0; i < ASSET_CACHE_MAX_ENTRIES; i++) {
        if (s_cache_table[i].id == asset_id) {
            return &s_cache_table[i];
        }
    }
    return NULL;
}

BOAssetHandle* BOAssetCache_Insert(uint32_t asset_id, const char* filepath, BOAssetType type) {
    // Check if already present
    BOAssetHandle* existing = BOAssetCache_Lookup(asset_id);
    if (existing) {
        return existing;
    }

    // Find free slot
    for (int i = 0; i < ASSET_CACHE_MAX_ENTRIES; i++) {
        if (s_cache_table[i].id == BOASSET_ID_NONE) {
            s_cache_table[i].id = asset_id;
            s_cache_table[i].type = type;
            copy_string(s_cache_table[i].filepath, filepath ? filepath : "", sizeof(s_cache_table[i].filepath));
            s_cache_table[i].width = 0;
            s_cache_table[i].height = 0;
            s_cache_table[i].loaded = false;
            s_cache_table[i].ref_count = 0;
            s_cache_table[i].image_data = NULL;
            s_cache_table[i].u1 = 0.0f;
            s_cache_table[i].v1 = 0.0f;
            s_cache_table[i].u2 = 0.0f;
            s_cache_table[i].v2 = 0.0f;
            return &s_cache_table[i];
        }
    }
    return NULL;
}

void BOAssetCache_Remove(BOAssetHandle* handle) {
    if (!handle) return;
    if (handle->image_data) {
        BOImage_FreeImage(handle->image_data);
        handle->image_data = NULL;
    }
    handle->id = BOASSET_ID_NONE;
    handle->loaded = false;
    handle->ref_count = 0;
}
