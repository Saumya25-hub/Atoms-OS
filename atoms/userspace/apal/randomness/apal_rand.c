/*
 * ATOMS Platform Adaptation Layer (APAL)
 * Randomness & Entropy Implementation
 */

#include "apal_rand.h"
#include "../time/apal_time.h"
#include <string.h>

static uint64_t s_rand_state[2] = {0x123456789ABCDEF0ULL, 0xFEDCBA9876543210ULL};
static bool s_rand_initialized = false;

static bool try_hardware_rdrand(uint64_t *val) {
    uint8_t success;
    uint64_t res;
    __asm__ volatile (
        "rdrand %0; setc %1"
        : "=r"(res), "=qm"(success)
    );
    if (success) {
        *val = res;
        return true;
    }
    return false;
}

static uint64_t xorshift128plus(void) {
    uint64_t x = s_rand_state[0];
    uint64_t const y = s_rand_state[1];
    s_rand_state[0] = y;
    x ^= x << 23;
    s_rand_state[1] = x ^ y ^ (x >> 17) ^ (y >> 26);
    return s_rand_state[1] + y;
}

static void ensure_rand_init(void) {
    if (!s_rand_initialized) {
        uint64_t hw_val = 0;
        if (try_hardware_rdrand(&hw_val)) {
            s_rand_state[0] ^= hw_val;
        }
        s_rand_state[1] ^= apal_time_now_monotonic_ns();
        s_rand_initialized = true;
    }
}

uint64_t apal_rand_uint64(void) {
    uint64_t val = 0;
    if (try_hardware_rdrand(&val)) {
        return val;
    }
    ensure_rand_init();
    return xorshift128plus();
}

apal_status_t apal_rand_bytes(void *output, size_t count) {
    if (!output || count == 0) return APAL_ERR_INVALID_PARAM;
    uint8_t *ptr = (uint8_t *)output;

    while (count >= sizeof(uint64_t)) {
        uint64_t r = apal_rand_uint64();
        memcpy(ptr, &r, sizeof(uint64_t));
        ptr += sizeof(uint64_t);
        count -= sizeof(uint64_t);
    }

    if (count > 0) {
        uint64_t r = apal_rand_uint64();
        memcpy(ptr, &r, count);
    }
    return APAL_OK;
}
