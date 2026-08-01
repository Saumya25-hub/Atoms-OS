#include "../include/bsr_api.h"
#include "kernel/botree/include/botree.h"

int32_t BSR_Search(const char* query_pattern, BDeDirEntry** out_results, uint32_t* out_count) {
    if (!query_pattern || !out_results || !out_count) return -1;
    return BDe_ReadDirectory("/", out_results, out_count);
}
