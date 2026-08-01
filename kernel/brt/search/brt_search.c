#include "../include/brt_api.h"
#include "kernel/shell_runtime/include/bsr_api.h"

int32_t BRT_Search(const char* query_pattern, BDeDirEntry** out_results, uint32_t* out_count) {
    return BSR_Search(query_pattern, out_results, out_count);
}
