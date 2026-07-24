#ifndef ADF_CORE_H
#define ADF_CORE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================
// ABE Production Debug & Diagnostics Framework (ADF)
// ============================================================

// Debug Severity Levels
typedef enum {
    ABE_DEBUG_NONE = 0,
    ABE_DEBUG_ERROR,
    ABE_DEBUG_WARNING,
    ABE_DEBUG_INFO,
    ABE_DEBUG_VERBOSE,
    ABE_DEBUG_TRACE
} ABE_DebugLevel;

// Subsystem IDs
typedef enum {
    ABE_SUB_HTML = 0,
    ABE_SUB_CSS,
    ABE_SUB_LAYOUT,
    ABE_SUB_PAINT,
    ABE_SUB_GPU,
    ABE_SUB_CANVAS,
    ABE_SUB_SVG,
    ABE_SUB_WEBGL,
    ABE_SUB_WASM,
    ABE_SUB_JAVASCRIPT,
    ABE_SUB_NETWORKING,
    ABE_SUB_STORAGE,
    ABE_SUB_MEMORY,
    ABE_SUB_SCHEDULER,
    ABE_SUB_OBSERVER,
    ABE_SUB_COUNT
} ABE_SubsystemID;

#define ABE_RING_BUFFER_SIZE 8192

typedef struct {
    uint64_t timestamp;
    uint32_t subsystem;
    uint32_t thread_id;
    uint32_t severity;
    char     message[128];
} ABE_LogEntry;

// Public Debug APIs
void ABE_DebugInitialize(void);
void ABE_DebugEnable(void);
void ABE_DebugDisable(void);
void ABE_DebugSetLevel(ABE_DebugLevel level);
void ABE_DebugEnableSubsystem(ABE_SubsystemID sub);
void ABE_DebugDisableSubsystem(ABE_SubsystemID sub);
void ABE_DebugLog(ABE_SubsystemID sub, ABE_DebugLevel severity, const char* msg);
void ABE_DebugDumpLog(void);

#ifdef __cplusplus
}
#endif

#endif // ADF_CORE_H
