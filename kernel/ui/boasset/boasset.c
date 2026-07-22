#include "boasset.h"
#include "asset_cache.h"
#include "asset_loader.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/gui/surface/surface.h"

static BOAtlas* s_master_atlas = NULL;

static int boimage_png_decode_wrapper(const uint8_t* in_data, uint32_t in_size, uint8_t** out_pixels, uint32_t* out_width, uint32_t* out_height) {
    extern struct BOSSurface* png_decode(const uint8_t* buffer, uint32_t size);
    struct BOSSurface* surface = png_decode(in_data, in_size);
    if (!surface || !surface->framebuffer) return -1;

    *out_pixels = (uint8_t*)surface->framebuffer;
    *out_width = (uint32_t)surface->width;
    *out_height = (uint32_t)surface->height;
    kfree(surface); // Free surface container struct; pixel buffer is retained by BOImage
    return 0;
}

void BOAsset_Initialize(void) {
    BOAssetCache_Initialize();

    // Bind BOIMAGE PNG decoder hook to native png_decode()
    BOImage_SetPNGDecoderHook(boimage_png_decode_wrapper);

    // Register predefined system assets into cache table matching FAT32 VFS filenames
    BOAssetCache_Insert(ICON_FOLDER,      "folder.png",   ASSET_TYPE_ICON);
    BOAssetCache_Insert(ICON_FILE,        "file.png",     ASSET_TYPE_ICON);
    BOAssetCache_Insert(ICON_TERMINAL,    "TERMINAL.PNG", ASSET_TYPE_ICON);
    BOAssetCache_Insert(ICON_EXPLORER,    "EXPLORER.PNG", ASSET_TYPE_ICON);
    BOAssetCache_Insert(ICON_SETTINGS,    "SETTINGS.PNG", ASSET_TYPE_ICON);
    BOAssetCache_Insert(ICON_CALCULATOR,  "CALCULAT.PNG", ASSET_TYPE_ICON);
    BOAssetCache_Insert(ICON_STRESS_TEST, "STRESST.PNG",  ASSET_TYPE_ICON);
    BOAssetCache_Insert(ICON_MUSIC,       "MUSIC.PNG",    ASSET_TYPE_ICON);
    BOAssetCache_Insert(ICON_DOOM,        "DOOM.PNG",     ASSET_TYPE_ICON);
    BOAssetCache_Insert(ICON_INPUT_LAB,   "INPUTLAB.PNG", ASSET_TYPE_ICON);
    BOAssetCache_Insert(ICON_CLOSE,       "close.png",    ASSET_TYPE_ICON);
    BOAssetCache_Insert(ICON_MINIMIZE,    "minimize.png", ASSET_TYPE_ICON);
    BOAssetCache_Insert(ICON_MAXIMIZE,    "maximize.png", ASSET_TYPE_ICON);
    BOAssetCache_Insert(CURSOR_ARROW,     "arrow.png",    ASSET_TYPE_CURSOR);
    BOAssetCache_Insert(ASSET_LOGO,       "logo.png",     ASSET_TYPE_IMAGE);
    BOAssetCache_Insert(ASSET_WALLPAPER,  "W1.PNG",       ASSET_TYPE_WALLPAPER);
}

void BOAsset_Shutdown(void) {
    BOAssetCache_Clear();
}

BOAssetHandle* BOAsset_Load(uint32_t asset_id, const char* filepath, BOAssetType type) {
    BOAssetHandle* handle = BOAssetCache_Insert(asset_id, filepath, type);
    if (!handle) return NULL;

    if (handle->loaded) {
        handle->ref_count++;
        return handle;
    }

    int status = BOAssetLoader_LoadFromVFS(handle);
    if (status != BOASSET_OK || !handle->image_data) {
        return NULL;
    }

    handle->ref_count = 1;

    // Ensure master atlas exists (8x8 cells = 512x512 canvas for all icons)
    if (!s_master_atlas) {
        s_master_atlas = BOImage_CreateAtlas(64, 8, 8);
    }

    if (s_master_atlas) {
        BOImage_AtlasInsert(s_master_atlas, handle->image_data, 
                            &handle->u1, &handle->v1, &handle->u2, &handle->v2);
    }

    return handle;
}

BOAssetHandle* BOAsset_Get(uint32_t asset_id) {
    BOAssetHandle* handle = BOAssetCache_Lookup(asset_id);
    if (!handle) return NULL;
    if (!handle->loaded) {
        return BOAsset_Load(handle->id, handle->filepath, handle->type);
    }
    handle->ref_count++;
    return handle;
}

void BOAsset_Release(BOAssetHandle* handle) {
    if (!handle || handle->ref_count == 0) return;
    handle->ref_count--;
}

void BOAsset_Unload(BOAssetHandle* handle) {
    if (!handle) return;
    BOAssetCache_Remove(handle);
}

BOAssetHandle* BOAsset_Reload(BOAssetHandle* handle) {
    if (!handle) return NULL;
    BOAsset_Unload(handle);
    return BOAsset_Load(handle->id, handle->filepath, handle->type);
}

void BOAsset_PreloadCritical(void) {
    // Preload all critical desktop app icons into master texture atlas
    BOAsset_Get(ASSET_LOGO);
    BOAsset_Get(ICON_FOLDER);
    BOAsset_Get(ICON_FILE);
    BOAsset_Get(ICON_EXPLORER);
    BOAsset_Get(ICON_TERMINAL);
    BOAsset_Get(ICON_SETTINGS);
    BOAsset_Get(ICON_CALCULATOR);
    BOAsset_Get(ICON_STRESS_TEST);
    BOAsset_Get(ICON_MUSIC);
    BOAsset_Get(ICON_DOOM);
    BOAsset_Get(ICON_INPUT_LAB);
    BOAsset_Get(ICON_CLOSE);
}

bool BOAsset_DrawAsset(uint32_t asset_id, int32_t x, int32_t y, int32_t w, int32_t h) {
    BOAssetHandle* handle = BOAsset_Get(asset_id);
    if (!handle || !handle->loaded || !s_master_atlas || !s_master_atlas->atlas_texture) {
        return false;
    }

    BOImage_BatchDrawSprite(s_master_atlas->atlas_texture, x, y, w, h, 
                            handle->u1, handle->v1, handle->u2, handle->v2);
    return true;
}
