#ifndef BOSPECTRA_DEBUG_H
#define BOSPECTRA_DEBUG_H

#include "../include/bospectra_types.h"
#include "../memory/bospectra_memory.h"

typedef struct {
    const char* subsystem_name;
    bool        is_initialized;
    bool        is_healthy;
    uint32_t    last_error;
    uint64_t    active_handles;
    size_t      allocated_bytes;
    uint64_t    total_operations;
} BOSPECTRA_SubsystemDiag;

typedef struct {
    bospectra_state_t       engine_state;
    BOSPECTRA_Version       version;
    uint64_t                uptime_ticks;
    BOSPECTRA_MemoryStats   memory_stats;
    uint32_t                active_streams;
    uint32_t                active_files;
    BOSPECTRA_SubsystemDiag core_diag;
    BOSPECTRA_SubsystemDiag memory_diag;
    BOSPECTRA_SubsystemDiag stream_diag;
    BOSPECTRA_SubsystemDiag file_diag;
    BOSPECTRA_SubsystemDiag buffer_diag;
} BOSPECTRA_Diagnostics;

void              bospectra_debug_init(void);
void              bospectra_debug_shutdown(void);
bospectra_error_t bospectra_debug_get_diagnostics(BOSPECTRA_Diagnostics* out_diag);
void              bospectra_debug_dump(void);
void              bospectra_log(const char* level, const char* message);

/* Forensic Diagnostic Logging Helpers */
void bospectra_trace_str(const char* label, const char* value);
void bospectra_trace_u32(const char* label, uint32_t value);
void bospectra_trace_hex(const char* label, uint64_t value);
void bospectra_trace_error(const char* func, bospectra_error_t err, const char* file, int line, const char* reason);

#endif // BOSPECTRA_DEBUG_H
