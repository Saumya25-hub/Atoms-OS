#include "../include/bsr_api.h"
#include "kernel/core/lib/include/string.h"

static BSR_FileAssociation s_associations[BSR_MAX_ASSOCIATIONS];
static uint32_t            s_assoc_count = 0;

int32_t BSR_RegisterAssociation(const char* ext, const char* app_name, const char* app_path) {
    if (!ext || !app_name || !app_path) return -1;

    for (uint32_t i = 0; i < s_assoc_count; i++) {
        if (strcmp(s_associations[i].ext, ext) == 0) {
            strcpy(s_associations[i].app_name, app_name);
            strcpy(s_associations[i].app_path, app_path);
            return 0;
        }
    }

    if (s_assoc_count < BSR_MAX_ASSOCIATIONS) {
        strcpy(s_associations[s_assoc_count].ext, ext);
        strcpy(s_associations[s_assoc_count].app_name, app_name);
        strcpy(s_associations[s_assoc_count].app_path, app_path);
        s_assoc_count++;
        return 0;
    }
    return -1;
}

const char* BSR_GetAssociation(const char* ext) {
    if (!ext) return NULL;
    for (uint32_t i = 0; i < s_assoc_count; i++) {
        if (strcmp(s_associations[i].ext, ext) == 0) {
            return s_associations[i].app_path;
        }
    }
    return NULL;
}
