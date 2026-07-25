/*
 * BOS OS — Phase 3: Production TLS & Security Engine
 * sec_random.c — Cryptographically Secure Random Number Generator (CSPRNG) Implementation
 */

#include "kernel/security/random/sec_random.h"
#include "kernel/security/include/bos_hash.h"
#include "kernel/security/include/bos_crypto.h"
#include "kernel/core/lib/include/string.h"

#define ENTROPY_POOL_SIZE 64

typedef struct {
    uint8_t  state_key[32];
    uint8_t  state_v[32];
    uint64_t reseed_counter;
    bool     initialized;
} csprng_state_t;

static csprng_state_t g_csprng;

static inline uint64_t rdtsc_entropy(void) {
    uint32_t low, high;
    __asm__ volatile("rdtsc" : "=a"(low), "=d"(high));
    return ((uint64_t)high << 32) | low;
}

static void mix_entropy_pool(const void* src, size_t len) {
    bos_sha256_ctx_t ctx;
    uint8_t digest[32];
    
    uint64_t tsc = rdtsc_entropy();
    
    bos_sha256_init(&ctx);
    bos_sha256_update(&ctx, g_csprng.state_key, 32);
    bos_sha256_update(&ctx, g_csprng.state_v, 32);
    bos_sha256_update(&ctx, &tsc, sizeof(tsc));
    if (src && len > 0) {
        bos_sha256_update(&ctx, src, len);
    }
    bos_sha256_final(&ctx, digest);
    
    /* Update state key & V */
    memcpy(g_csprng.state_key, digest, 32);
    
    /* Second iteration for state V */
    bos_sha256_init(&ctx);
    bos_sha256_update(&ctx, digest, 32);
    bos_sha256_update(&ctx, &g_csprng.reseed_counter, sizeof(g_csprng.reseed_counter));
    bos_sha256_final(&ctx, g_csprng.state_v);
    
    g_csprng.reseed_counter++;
}

bos_sec_status_t sec_random_init(void) {
    memset(&g_csprng, 0, sizeof(g_csprng));
    
    /* Gather initial entropy from hardware TSC, kernel memory addresses, stacks */
    uint64_t initial_entropy[8];
    for (int i = 0; i < 8; i++) {
        initial_entropy[i] = rdtsc_entropy() ^ ((uint64_t)(uintptr_t)&initial_entropy << (i * 4));
    }
    
    mix_entropy_pool(initial_entropy, sizeof(initial_entropy));
    g_csprng.initialized = true;
    return BOS_SEC_OK;
}

void bos_random_seed_entropy(const void* data, size_t len) {
    mix_entropy_pool(data, len);
}

bos_sec_status_t bos_random_bytes(void* buf, size_t len) {
    if (!buf || len == 0) return BOS_SEC_ERR_INVALID_PARAM;
    if (!g_csprng.initialized) {
        sec_random_init();
    }
    
    uint8_t* out = (uint8_t*)buf;
    size_t offset = 0;
    
    while (offset < len) {
        uint64_t tsc = rdtsc_entropy();
        bos_sha256_ctx_t ctx;
        uint8_t block[32];
        
        bos_sha256_init(&ctx);
        bos_sha256_update(&ctx, g_csprng.state_key, 32);
        bos_sha256_update(&ctx, g_csprng.state_v, 32);
        bos_sha256_update(&ctx, &tsc, sizeof(tsc));
        bos_sha256_update(&ctx, &offset, sizeof(offset));
        bos_sha256_final(&ctx, block);
        
        size_t chunk = (len - offset < 32) ? (len - offset) : 32;
        memcpy(out + offset, block, chunk);
        offset += chunk;
        
        /* Fast state progression */
        for (size_t i = 0; i < 32; i++) {
            g_csprng.state_v[i] ^= block[i];
        }
        g_csprng.reseed_counter++;
    }
    
    /* Periodic reseed */
    if (g_csprng.reseed_counter % 100 == 0) {
        mix_entropy_pool(NULL, 0);
    }
    
    return BOS_SEC_OK;
}

uint32_t bos_random_u32(void) {
    uint32_t val = 0;
    bos_random_bytes(&val, sizeof(val));
    return val;
}

uint64_t bos_random_u64(void) {
    uint64_t val = 0;
    bos_random_bytes(&val, sizeof(val));
    return val;
}
