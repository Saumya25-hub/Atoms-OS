#include "../include/bsom_api.h"

int32_t BSOM_QueryMetadata(BSOMObject* obj, const char* key, char* out_val, size_t max_len) {
    return BSOM_GetProperty(obj, key, out_val, max_len);
}
