#ifndef BOS_PACK_H
#define BOS_PACK_H

#include "platform/include/bos_types.h"

typedef struct {
    char name[64];
    char identifier[64];
    char version[16];
    char min_bos_version[16];
    char entry_point[64];
    char icon_path[64];
} BOS_AppManifest;

BOS_Result BOS_Manifest_Parse(const char* json_str, BOS_AppManifest* out_manifest);
BOS_Result BOS_Package_Load(const char* package_path, BOS_AppManifest* out_manifest);

#endif /* BOS_PACK_H */
