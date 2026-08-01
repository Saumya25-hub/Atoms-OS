#include "../include/bsom_api.h"

int32_t BSOM_Search(const char* pattern, BSOMObject*** out_results, uint32_t* out_count) {
    if (!pattern || !out_results || !out_count) return -1;
    *out_results = NULL;
    *out_count = 0;
    return 0;
}
