#ifndef ATRIX_JS_RUNTIME_H
#define ATRIX_JS_RUNTIME_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================
// ATRIX JavaScript Stack VM Runtime Engine (Phase 12)
// ============================================================

typedef struct {
    int64_t  stack[64];
    uint32_t stack_ptr;
    uint32_t total_instructions_executed;
    bool     has_error;
} ATRIX_JS_VM;

void ATRIX_JSRuntime_Init(void);
bool ATRIX_JS_ExecuteScript(const char* script_code);

#ifdef __cplusplus
}
#endif

#endif // ATRIX_JS_RUNTIME_H
