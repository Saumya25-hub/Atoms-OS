#include "../include/bishop_bigint.h"
#include "../internal/bishop_internal.h"

// =============================================
// BISHOP X ENGINE - Big Integer (X-2)
// =============================================

BishopBigInt bishop_bigint_zero(void) {
    BishopBigInt n;
    n.length = 1;
    n.negative = false;
    for (int i = 0; i < BISHOP_BIGINT_MAX_DIGITS; i++) n.digits[i] = 0;
    return n;
}

BishopBigInt bishop_bigint_from_int(int64_t value) {
    BishopBigInt n = bishop_bigint_zero();
    
    if (value < 0) {
        n.negative = true;
        value = -value;
    }
    
    if (value == 0) {
        n.length = 1;
        return n;
    }
    
    int i = 0;
    while (value > 0 && i < BISHOP_BIGINT_MAX_DIGITS) {
        n.digits[i++] = (uint8_t)(value % 10);
        value /= 10;
    }
    n.length = i;
    return n;
}

// Internal: add absolute values
static BishopError bigint_add_abs(const BishopBigInt* a, const BishopBigInt* b, BishopBigInt* result) {
    *result = bishop_bigint_zero();
    
    int max_len = a->length > b->length ? a->length : b->length;
    uint8_t carry = 0;
    
    for (int i = 0; i < max_len || carry; i++) {
        if (i >= BISHOP_BIGINT_MAX_DIGITS) {
            return BISHOP_ERR_OVERFLOW;
        }
        
        uint8_t sum = carry;
        if (i < a->length) sum += a->digits[i];
        if (i < b->length) sum += b->digits[i];
        
        result->digits[i] = sum % 10;
        carry = sum / 10;
        result->length = i + 1;
    }
    
    return BISHOP_OK;
}

// Internal: compare absolute values
static int bigint_compare_abs(const BishopBigInt* a, const BishopBigInt* b) {
    if (a->length != b->length) return a->length > b->length ? 1 : -1;
    for (int i = a->length - 1; i >= 0; i--) {
        if (a->digits[i] != b->digits[i])
            return a->digits[i] > b->digits[i] ? 1 : -1;
    }
    return 0;
}

// Internal: subtract absolute values (assumes |a| >= |b|)
static BishopError bigint_sub_abs(const BishopBigInt* a, const BishopBigInt* b, BishopBigInt* result) {
    *result = bishop_bigint_zero();
    
    int8_t borrow = 0;
    for (int i = 0; i < a->length; i++) {
        int8_t diff = (int8_t)a->digits[i] - borrow;
        if (i < b->length) diff -= (int8_t)b->digits[i];
        
        if (diff < 0) {
            diff += 10;
            borrow = 1;
        } else {
            borrow = 0;
        }
        result->digits[i] = (uint8_t)diff;
    }
    
    // Find actual length
    result->length = a->length;
    while (result->length > 1 && result->digits[result->length - 1] == 0)
        result->length--;
    
    return BISHOP_OK;
}

BishopError bishop_bigint_add(const BishopBigInt* a, const BishopBigInt* b, BishopBigInt* result) {
    if (!a || !b || !result) return BISHOP_ERR_INVALID_ARG;
    
    // Same sign: add absolute values
    if (a->negative == b->negative) {
        BishopError err = bigint_add_abs(a, b, result);
        result->negative = a->negative;
        return err;
    }
    
    // Different signs: subtract
    int cmp = bigint_compare_abs(a, b);
    if (cmp == 0) {
        *result = bishop_bigint_zero();
        return BISHOP_OK;
    }
    
    if (cmp > 0) {
        BishopError err = bigint_sub_abs(a, b, result);
        result->negative = a->negative;
        return err;
    } else {
        BishopError err = bigint_sub_abs(b, a, result);
        result->negative = b->negative;
        return err;
    }
}

BishopError bishop_bigint_mul(const BishopBigInt* a, const BishopBigInt* b, BishopBigInt* result) {
    if (!a || !b || !result) return BISHOP_ERR_INVALID_ARG;
    
    *result = bishop_bigint_zero();
    
    if ((a->length + b->length) > BISHOP_BIGINT_MAX_DIGITS) {
        return BISHOP_ERR_OVERFLOW;
    }
    
    for (int i = 0; i < a->length; i++) {
        uint8_t carry = 0;
        for (int j = 0; j < b->length || carry; j++) {
            int pos = i + j;
            if (pos >= BISHOP_BIGINT_MAX_DIGITS) return BISHOP_ERR_OVERFLOW;
            
            uint32_t prod = (uint32_t)result->digits[pos] + carry;
            if (j < b->length) prod += (uint32_t)a->digits[i] * (uint32_t)b->digits[j];
            
            result->digits[pos] = (uint8_t)(prod % 10);
            carry = (uint8_t)(prod / 10);
            
            if (pos >= result->length) result->length = pos + 1;
        }
    }
    
    result->negative = (a->negative != b->negative) && (result->length > 1 || result->digits[0] != 0);
    
    // Trim leading zeros
    while (result->length > 1 && result->digits[result->length - 1] == 0)
        result->length--;
    
    return BISHOP_OK;
}

int bishop_bigint_compare(const BishopBigInt* a, const BishopBigInt* b) {
    if (!a || !b) return 0;
    
    if (a->negative && !b->negative) return -1;
    if (!a->negative && b->negative) return 1;
    
    int cmp = bigint_compare_abs(a, b);
    return a->negative ? -cmp : cmp;
}

void bishop_bigint_print(const BishopBigInt* n) {
    if (!n) { bos_print("(null)"); return; }
    if (n->negative && !(n->length == 1 && n->digits[0] == 0)) bos_print("-");
    
    for (int i = n->length - 1; i >= 0; i--) {
        char c[2] = { '0' + n->digits[i], '\0' };
        bos_print(c);
    }
}
