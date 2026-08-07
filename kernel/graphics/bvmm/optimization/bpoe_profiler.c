#include "bpoe.h"

bvmm_result_t bpoe_profile_operation(uint64_t latency_ns, uint64_t bytes_transferred) {
    bpoe_diagnostics_t diag;
    if (bpoe_get_diagnostics(&diag) != BVMM_SUCCESS) return BVMM_ERR_NOT_INITIALIZED;

    diag.profile.total_operations_executed++;
    diag.profile.total_memory_traffic_bytes += bytes_transferred;

    if (latency_ns > diag.profile.peak_fastpath_latency_ns) {
        diag.profile.peak_fastpath_latency_ns = latency_ns;
    }
    diag.profile.average_fastpath_latency_ns = (diag.profile.average_fastpath_latency_ns + latency_ns) / 2;

    return BVMM_SUCCESS;
}
