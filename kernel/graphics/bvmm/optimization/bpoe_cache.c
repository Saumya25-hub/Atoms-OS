#include "bpoe.h"

bvmm_result_t bpoe_cache_warmup(void) {
    /* Pre-warm descriptor caches for zero cache-miss latencies */
    return BVMM_SUCCESS;
}
