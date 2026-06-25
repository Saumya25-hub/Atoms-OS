#include "../include/bishop_error.h"
#include "../../libbos/include/bos.h"

// =============================================
// BISHOP X ENGINE - Error/Safety Layer (X-1)
// =============================================

static BishopError g_bishop_last_error = BISHOP_OK;

BishopError bishop_get_last_error(void) {
    return g_bishop_last_error;
}

void bishop_clear_error(void) {
    g_bishop_last_error = BISHOP_OK;
}

void bishop_set_error(BishopError err) {
    g_bishop_last_error = err;
}

const char* bishop_error_string(BishopError err) {
    switch (err) {
        case BISHOP_OK:                     return "OK";
        case BISHOP_ERR_DIV_ZERO:           return "Division By Zero";
        case BISHOP_ERR_OVERFLOW:            return "Overflow Detected";
        case BISHOP_ERR_UNDERFLOW:           return "Underflow Detected";
        case BISHOP_ERR_NAN:                 return "NaN Detected";
        case BISHOP_ERR_INFINITY:            return "Infinity Detected";
        case BISHOP_ERR_INVALID_ARG:         return "Invalid Argument";
        case BISHOP_ERR_DIM_MISMATCH:        return "Dimension Mismatch";
        case BISHOP_ERR_OUT_OF_MEMORY:       return "Out Of Memory";
        case BISHOP_ERR_RECURSION_OVERFLOW:  return "Recursion Overflow";
        case BISHOP_ERR_DOMAIN:              return "Domain Error";
        case BISHOP_ERR_SINGULAR_MATRIX:     return "Singular Matrix";
        default:                             return "Unknown Error";
    }
}

bool bishop_is_nan_p(const double* x) {
    volatile double v = *x;
    return v != v;
}

bool bishop_is_inf_p(const double* x) {
    if (bishop_is_nan_p(x)) return false;
    volatile double val = *x;
    volatile double diff = val - val;
    if (val != 0.0 && (diff != diff)) return true;
    return false;
}

bool bishop_is_valid_p(const double* x) {
    return !bishop_is_nan_p(x) && !bishop_is_inf_p(x);
}

void bishop_report_error(const char* function_name, BishopError err) {
    bos_print("[BishopMath Error] ");
    bos_print(function_name);
    bos_print(": ");
    bos_print(bishop_error_string(err));
    bos_print("\nExecution Halted Safely\n");
    bishop_set_error(err);
}
