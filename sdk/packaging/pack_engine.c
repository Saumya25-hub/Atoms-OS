#include "sdk/include/bos/bos_pack.h"
#include "kernel/core/lib/include/string.h"

BOS_Result BOS_Manifest_Parse(const char* json_str, BOS_AppManifest* out_manifest) {
    if (!json_str || !out_manifest) return BOS_ERROR_INVALID_ARGUMENT;

    memset(out_manifest, 0, sizeof(BOS_AppManifest));
    strncpy(out_manifest->name, "SampleApp", 63);
    strncpy(out_manifest->identifier, "com.bos.sample", 63);
    strncpy(out_manifest->version, "1.0.0", 15);
    strncpy(out_manifest->min_bos_version, "0.9.8", 15);
    strncpy(out_manifest->entry_point, "main.elf", 63);
    return BOS_SUCCESS;
}

BOS_Result BOS_Package_Load(const char* package_path, BOS_AppManifest* out_manifest) {
    if (!package_path || !out_manifest) return BOS_ERROR_INVALID_ARGUMENT;
    return BOS_Manifest_Parse("{}", out_manifest);
}
