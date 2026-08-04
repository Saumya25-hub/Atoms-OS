/*
 * BOSPECTRA V3 — Error Manager Subsystem
 * kernel/media/bospectra/runtime/error_manager.h
 *
 * Structured error object definitions, categories, and severity levels.
 */

#ifndef BOSPECTRA_V3_ERROR_MANAGER_H
#define BOSPECTRA_V3_ERROR_MANAGER_H

#include "../include/bospectra_types.h"
#include "../include/bospectra_errors.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef enum {
    ERR_SEVERITY_INFO = 0,
    ERR_SEVERITY_WARNING,
    ERR_SEVERITY_ERROR,
    ERR_SEVERITY_CRITICAL,
    ERR_SEVERITY_FATAL
} BOSPECTRA_ErrorSeverity;

typedef enum {
    ERR_CAT_MEMORY = 0,
    ERR_CAT_CONTAINER,
    ERR_CAT_VFS,
    ERR_CAT_PACKET,
    ERR_CAT_DECODER,
    ERR_CAT_COLOR,
    ERR_CAT_RENDERER,
    ERR_CAT_SCHEDULER,
    ERR_CAT_DISPLAY,
    ERR_CAT_PLAYBACK,
    ERR_CAT_SYNC,
    ERR_CAT_RESOURCE,
    ERR_CAT_OWNERSHIP,
    ERR_CAT_REFCOUNT,
    ERR_CAT_PIPELINE,
    ERR_CAT_INTERNAL
} BOSPECTRA_ErrorCategory;

typedef struct {
    bospectra_error_t       error_code;
    BOSPECTRA_ErrorSeverity severity;
    BOSPECTRA_ErrorCategory category;
    char                    subsystem[24];
    char                    message[64];
    char                    source_file[32];
    char                    function[32];
    uint32_t                line;
    uint32_t                session_id;
    uint64_t                timestamp_us;
    bool                    recoverable;
    char                    suggested_action[64];
} BOSPECTRA_Error;

void bospectra_error_manager_init(void);
void bospectra_error_manager_shutdown(void);

BOSPECTRA_Error bospectra_error_create(
    bospectra_error_t code,
    BOSPECTRA_ErrorSeverity severity,
    BOSPECTRA_ErrorCategory category,
    const char* subsystem,
    const char* message,
    const char* source_file,
    const char* function,
    uint32_t line,
    uint32_t session_id,
    bool recoverable,
    const char* suggested_action
);

#endif /* BOSPECTRA_V3_ERROR_MANAGER_H */
