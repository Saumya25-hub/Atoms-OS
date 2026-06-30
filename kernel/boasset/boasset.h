#ifndef KERNEL_BOASSET_BOASSET_H
#define KERNEL_BOASSET_BOASSET_H

#include "asset_types.h"

// Lifecycle
void BOAsset_Initialize(void);
void BOAsset_Shutdown(void);

// Registration & Loading
BOAssetHandle* BOAsset_Load(uint32_t asset_id, const char* filepath, BOAssetType type);
BOAssetHandle* BOAsset_Get(uint32_t asset_id);
void BOAsset_Release(BOAssetHandle* handle);
void BOAsset_Unload(BOAssetHandle* handle);
BOAssetHandle* BOAsset_Reload(BOAssetHandle* handle);

// Boot Preload
void BOAsset_PreloadCritical(void);

// Direct Rendering Helper (Invokes BOIMAGE v2 Batching Pipeline)
bool BOAsset_DrawAsset(uint32_t asset_id, int32_t x, int32_t y, int32_t w, int32_t h);

#endif // KERNEL_BOASSET_BOASSET_H
