#ifndef KERNEL_BOASSET_ASSET_LOADER_H
#define KERNEL_BOASSET_ASSET_LOADER_H

#include "asset_types.h"

int BOAssetLoader_LoadFromVFS(BOAssetHandle* handle);
int BOAssetLoader_GeneratePlaceholder(BOAssetHandle* handle);

#endif // KERNEL_BOASSET_ASSET_LOADER_H
