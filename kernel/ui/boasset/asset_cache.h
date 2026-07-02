#ifndef KERNEL_BOASSET_ASSET_CACHE_H
#define KERNEL_BOASSET_ASSET_CACHE_H

#include "asset_types.h"

void BOAssetCache_Initialize(void);
void BOAssetCache_Clear(void);
BOAssetHandle* BOAssetCache_Lookup(uint32_t asset_id);
BOAssetHandle* BOAssetCache_Insert(uint32_t asset_id, const char* filepath, BOAssetType type);
void BOAssetCache_Remove(BOAssetHandle* handle);

#endif // KERNEL_BOASSET_ASSET_CACHE_H
