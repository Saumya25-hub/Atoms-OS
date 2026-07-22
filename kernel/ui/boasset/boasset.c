#include "boasset.h"
#include "asset_cache.h"
#include "asset_loader.h"

static BOAtlas* s_master_atlas = NULL;

void BOAsset_Initialize(void) {
    BOAssetCache_Initialize();

    // Register predefined system assets into cache table
    BOAssetCache_Insert(ICON_FOLDER,      "System/Assets/Icons/folder.png",      ASSET_TYPE_ICON);
    BOAssetCache_Insert(ICON_FILE,        "System/Assets/Icons/file.png",        ASSET_TYPE_ICON);
    BOAssetCache_Insert(ICON_TERMINAL,    "System/Assets/Icons/terminal.png",    ASSET_TYPE_ICON);
    BOAssetCache_Insert(ICON_EXPLORER,    "System/Assets/Icons/explorer.png",    ASSET_TYPE_ICON);
    BOAssetCache_Insert(ICON_SETTINGS,    "System/Assets/Icons/settings.png",    ASSET_TYPE_ICON);
    BOAssetCache_Insert(ICON_CALCULATOR,  "System/Assets/Icons/calculator.png",  ASSET_TYPE_ICON);
    BOAssetCache_Insert(ICON_STRESS_TEST, "System/Assets/Icons/stresstest.png",  ASSET_TYPE_ICON);
    BOAssetCache_Insert(ICON_MUSIC,       "System/Assets/Icons/music.png",       ASSET_TYPE_ICON);
    BOAssetCache_Insert(ICON_DOOM,        "System/Assets/Icons/doom.png",        ASSET_TYPE_ICON);
    BOAssetCache_Insert(ICON_INPUT_LAB,   "System/Assets/Icons/inputlab.png",    ASSET_TYPE_ICON);
    BOAssetCache_Insert(ICON_CLOSE,       "System/Assets/Window/close.png",      ASSET_TYPE_ICON);
    BOAssetCache_Insert(ICON_MINIMIZE,    "System/Assets/Window/minimize.png",   ASSET_TYPE_ICON);
    BOAssetCache_Insert(ICON_MAXIMIZE,    "System/Assets/Window/maximize.png",   ASSET_TYPE_ICON);
    BOAssetCache_Insert(CURSOR_ARROW,     "System/Assets/Cursor/arrow.png",      ASSET_TYPE_CURSOR);
    BOAssetCache_Insert(ASSET_LOGO,       "System/Assets/Branding/logo.png",     ASSET_TYPE_IMAGE);
    BOAssetCache_Insert(ASSET_WALLPAPER,  "System/Assets/Desktop/wallpaper.bmp", ASSET_TYPE_WALLPAPER);
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
