#ifndef BISHOP_BIGINT_H
#define BISHOP_BIGINT_H

#include "bishop_error.h"
#include <stdint.h>

// =============================================
// BISHOP X ENGINE - Big Integer (X-2)
// Stack-allocated, no heap required
// =============================================

#define BISHOP_BIGINT_MAX_DIGITS 256

typedef struct {
    uint8_t digits[BISHOP_BIGINT_MAX_DIGITS]; // digits[0] = least significant
    int     length;                            // number of active digits
    bool    negative;
} BishopBigInt;

// Creation
BishopBigInt  bishop_bigint_zero(void);
BishopBigInt  bishop_bigint_from_int(int64_t value);

// Arithmetic
BishopError   bishop_bigint_add(const BishopBigInt* a, const BishopBigInt* b, BishopBigInt* result);
BishopError   bishop_bigint_mul(const BishopBigInt* a, const BishopBigInt* b, BishopBigInt* result);

// Comparison: returns -1, 0, or 1
int           bishop_bigint_compare(const BishopBigInt* a, const BishopBigInt* b);

// Debug
void          bishop_bigint_print(const BishopBigInt* n);

#endif // BISHOP_BIGINT_H
