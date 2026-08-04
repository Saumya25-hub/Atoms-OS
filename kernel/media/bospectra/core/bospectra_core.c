#include "bospectra_core.h"
#include "bospectra_state.h"
#include "../include/bospectra.h"
#include "../include/bospectra_errors.h"
#include "../memory/bospectra_memory.h"
#include "../debug/bospectra_debug.h"
#include "../debug/bospectra_version.h"
#include "../packet/bospectra_packet.h"
#include "../stream/bospectra_stream.h"
#include "../file/bospectra_file.h"
#include "../buffer/bospectra_buffer.h"
#include "../tests/bospectra_tests.h"
#include "kernel/core/lib/include/string.h"

#include "../manager/media_manager.h"
#include "../registry/bospectra_registry.h"

static bool g_bospectra_core_initialized = false;

bospectra_error_t bospectra_core_bootstrap_subsystems(void) {
    bospectra_memory_init();
    bospectra_debug_init();
    bospectra_packet_subsystem_init();
    bospectra_stream_subsystem_init();
    bospectra_file_subsystem_init();
    bospectra_buffer_subsystem_init();
    bospectra_v3_registry_init();
    bospectra_media_manager_init();
    return BOSPECTRA_SUCCESS;
}

bospectra_error_t bospectra_core_teardown_subsystems(void) {
    bospectra_buffer_subsystem_shutdown();
    bospectra_file_subsystem_shutdown();
    bospectra_stream_subsystem_shutdown();
    bospectra_packet_subsystem_shutdown();
    bospectra_debug_shutdown();
    bospectra_memory_shutdown();
    return BOSPECTRA_SUCCESS;
}

bool bospectra_core_is_ready(void) {
    return g_bospectra_core_initialized && (bospectra_state_get() == BOSPECTRA_STATE_READY);
}

const char* bospectra_error_to_string(bospectra_error_t err) {
    switch (err) {
        case BOSPECTRA_SUCCESS:                 return "BOSPECTRA_SUCCESS";
        case BOSPECTRA_ERR_NOT_INITIALIZED:     return "BOSPECTRA_ERR_NOT_INITIALIZED";
        case BOSPECTRA_ERR_ALREADY_INITIALIZED: return "BOSPECTRA_ERR_ALREADY_INITIALIZED";
        case BOSPECTRA_ERR_INVALID_ARGUMENT:    return "BOSPECTRA_ERR_INVALID_ARGUMENT";
        case BOSPECTRA_ERR_OUT_OF_MEMORY:       return "BOSPECTRA_ERR_OUT_OF_MEMORY";
        case BOSPECTRA_ERR_STATE_INVALID:       return "BOSPECTRA_ERR_STATE_INVALID";
        case BOSPECTRA_ERR_FILE_NOT_FOUND:      return "BOSPECTRA_ERR_FILE_NOT_FOUND";
        case BOSPECTRA_ERR_FILE_READ_FAILED:    return "BOSPECTRA_ERR_FILE_READ_FAILED";
        case BOSPECTRA_ERR_BUFFER_OVERFLOW:     return "BOSPECTRA_ERR_BUFFER_OVERFLOW";
        case BOSPECTRA_ERR_BUFFER_UNDERFLOW:    return "BOSPECTRA_ERR_BUFFER_UNDERFLOW";
        case BOSPECTRA_ERR_HANDLE_INVALID:      return "BOSPECTRA_ERR_HANDLE_INVALID";
        case BOSPECTRA_ERR_STREAM_EXISTS:       return "BOSPECTRA_ERR_STREAM_EXISTS";
        case BOSPECTRA_ERR_STREAM_NOT_FOUND:    return "BOSPECTRA_ERR_STREAM_NOT_FOUND";
        case BOSPECTRA_ERR_SUBSYSTEM_FAILED:   return "BOSPECTRA_ERR_SUBSYSTEM_FAILED";
        case BOSPECTRA_ERR_SELF_TEST_FAILED:    return "BOSPECTRA_ERR_SELF_TEST_FAILED";
        default:                                return "BOSPECTRA_ERR_UNKNOWN";
    }
}

bospectra_error_t BOSPECTRA_Init(void) {
    if (g_bospectra_core_initialized) {
        return BOSPECTRA_ERR_ALREADY_INITIALIZED;
    }

    bospectra_state_init();
    if (bospectra_state_transition(BOSPECTRA_STATE_UNINITIALIZED, BOSPECTRA_STATE_INITIALIZING) != BOSPECTRA_SUCCESS) {
        return BOSPECTRA_ERR_STATE_INVALID;
    }

    bospectra_error_t err = bospectra_core_bootstrap_subsystems();
    if (err != BOSPECTRA_SUCCESS) {
        bospectra_state_transition(BOSPECTRA_STATE_INITIALIZING, BOSPECTRA_STATE_ERROR);
        return err;
    }

    bospectra_log("CORE", "BOSPECTRA Engine Core Subsystems Bootstrapped.");

    if (bospectra_state_transition(BOSPECTRA_STATE_INITIALIZING, BOSPECTRA_STATE_READY) != BOSPECTRA_SUCCESS) {
        return BOSPECTRA_ERR_STATE_INVALID;
    }

    g_bospectra_core_initialized = true;
    bospectra_log("CORE", "BOSPECTRA Engine Initialized (State: READY).");

    return BOSPECTRA_SUCCESS;
}

bospectra_error_t BOSPECTRA_Shutdown(void) {
    if (!g_bospectra_core_initialized) {
        return BOSPECTRA_ERR_NOT_INITIALIZED;
    }

    bospectra_state_t cur_state = bospectra_state_get();
    if (bospectra_state_transition(cur_state, BOSPECTRA_STATE_STOPPING) != BOSPECTRA_SUCCESS) {
        return BOSPECTRA_ERR_STATE_INVALID;
    }

    bospectra_log("CORE", "Shutting down BOSPECTRA Engine Subsystems...");
    bospectra_core_teardown_subsystems();

    bospectra_state_transition(BOSPECTRA_STATE_STOPPING, BOSPECTRA_STATE_SHUTDOWN);
    g_bospectra_core_initialized = false;

    return BOSPECTRA_SUCCESS;
}

bospectra_state_t BOSPECTRA_GetState(void) {
    return bospectra_state_get();
}

void BOSPECTRA_GetVersion(BOSPECTRA_Version* out_version) {
    if (!out_version) return;
    out_version->major = BOSPECTRA_VERSION_MAJOR;
    out_version->minor = BOSPECTRA_VERSION_MINOR;
    out_version->patch = BOSPECTRA_VERSION_PATCH;
    out_version->build = BOSPECTRA_VERSION_BUILD;
    strncpy(out_version->build_tag, BOSPECTRA_VERSION_TAG, sizeof(out_version->build_tag) - 1);
}

const char* BOSPECTRA_GetVersionString(void) {
    return BOSPECTRA_VERSION_STR;
}

bospectra_error_t BOSPECTRA_RunSelfTests(void) {
    return bospectra_tests_run_all();
}

void BOSPECTRA_DumpDiagnostics(void) {
    bospectra_debug_dump();
}
