#ifndef BISHOP_ERROR_H
#define BISHOP_ERROR_H

#include <stdint.h>
#include <stdbool.h>

// =============================================
// BISHOP X ENGINE - Error/Safety Layer (X-1)
// =============================================
// NOTE: BishopResult uses int64_t fixed-point instead of double
// to avoid SSE register issues in -mno-sse mode.
// We pass doubles via pointers, never by struct return.

typedef enum {
    BISHOP_OK                   = 0,
    BISHOP_ERR_DIV_ZERO         = 1,
    BISHOP_ERR_OVERFLOW         = 2,
    BISHOP_ERR_UNDERFLOW        = 3,
    BISHOP_ERR_NAN              = 4,
    BISHOP_ERR_INFINITY         = 5,
    BISHOP_ERR_INVALID_ARG      = 6,
    BISHOP_ERR_DIM_MISMATCH     = 7,
    BISHOP_ERR_OUT_OF_MEMORY    = 8,
    BISHOP_ERR_RECURSION_OVERFLOW = 9,
    BISHOP_ERR_DOMAIN           = 10,
    BISHOP_ERR_SINGULAR_MATRIX  = 11
} BishopError;

// Error state management
BishopError  bishop_get_last_error(void);
void         bishop_clear_error(void);
void         bishop_set_error(BishopError err);
const char*  bishop_error_string(BishopError err);

// Safety checking helpers (take pointer to avoid SSE)
bool bishop_is_nan_p(const double* x);
bool bishop_is_inf_p(const double* x);
bool bishop_is_valid_p(const double* x);

// Error reporting (prints to console, does NOT crash)
void bishop_report_error(const char* function_name, BishopError err);

#endif // BISHOP_ERROR_H
