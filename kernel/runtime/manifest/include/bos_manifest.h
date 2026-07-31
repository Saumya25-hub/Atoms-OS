#ifndef BOS_MANIFEST_H
#define BOS_MANIFEST_H

#include <stdint.h>
#include <stdbool.h>

#define BOS_MANIFEST_MAX_STR 128

typedef struct {
    char name[BOS_MANIFEST_MAX_STR];
    char identifier[BOS_MANIFEST_MAX_STR];
    char version[32];
    char author[BOS_MANIFEST_MAX_STR];
    char entry_point[BOS_MANIFEST_MAX_STR];
    char target_platform[64];
    uint32_t window_width;
    uint32_t window_height;
    bool is_parsed;
} BOS_Manifest;

BOS_Manifest* bos_manifest_parse(const char* json_content);
void bos_manifest_free(BOS_Manifest* manifest);

#endif /* BOS_MANIFEST_H */
