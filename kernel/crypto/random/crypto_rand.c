#include "crypto_rand.h"
#include "arch/x86_64/io/port_io.h"

static uint32_t g_rand_state = 0x91827364;

void crypto_random_bytes(uint8_t* buf, size_t len) {
    if (!buf || len == 0) return;

    for (size_t i = 0; i < len; i++) {
        g_rand_state = g_rand_state * 1103515245 + 12345;
        buf[i] = (uint8_t)((g_rand_state >> 16) & 0xFF);
    }
}
