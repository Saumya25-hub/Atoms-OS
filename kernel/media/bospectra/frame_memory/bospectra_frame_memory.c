#include "include/bospectra_frame_memory.h"
#include "frame_pool/frame_pool.h"
#include "packet_pool/packet_pool.h"
#include "diag/frame_memory_diag.h"
#include "tests/frame_memory_tests.h"
#include "../debug/bospectra_debug.h"
#include "../include/bospectra_errors.h"

static bool g_frame_memory_engine_initialized = false;

bospectra_error_t BOSPECTRA_FrameMemory_Init(void) {
    if (g_frame_memory_engine_initialized) {
        return BOSPECTRA_ERR_ALREADY_INITIALIZED;
    }

    bospectra_frame_pool_init();
    bospectra_packet_pool_init();
    bospectra_frame_memory_diag_init();

    g_frame_memory_engine_initialized = true;
    bospectra_log("FRAME_MEMORY", "Frame Memory Subsystem Initialized (Frame & Packet Pools Allocated).");

    // Run self-tests and stress test (disabled for instant boot)
    // bospectra_frame_memory_tests_run_all();

    return BOSPECTRA_SUCCESS;
}

bospectra_error_t BOSPECTRA_FrameMemory_Shutdown(void) {
    if (!g_frame_memory_engine_initialized) {
        return BOSPECTRA_ERR_NOT_INITIALIZED;
    }

    bospectra_frame_memory_diag_shutdown();
    bospectra_packet_pool_shutdown();
    bospectra_frame_pool_shutdown();

    g_frame_memory_engine_initialized = false;
    return BOSPECTRA_SUCCESS;
}

void BOSPECTRA_FrameMemory_GetStats(BOSPECTRA_FrameMemoryStats* out_stats) {
    bospectra_frame_memory_collect_stats(out_stats);
}

void BOSPECTRA_FrameMemory_DumpDiagnostics(void) {
    bospectra_frame_memory_dump_telemetry();
}
