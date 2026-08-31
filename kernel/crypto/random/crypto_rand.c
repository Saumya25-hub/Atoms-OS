#include "crypto_rand.h"
#include "kernel/crypto/sha256/sha256.h"
#include "kernel/drivers/rtc/rtc.h"
#include "arch/x86_64/io/port_io.h"
#include "kernel/core/lib/include/string.h"

static uint8_t g_entropy_pool[32];
static uint64_t g_drbg_counter = 0;
static bool g_crypto_rand_initialized = false;
static bool g_has_rdrand = false;

static inline uint64_t rdtsc(void) {
    uint32_t lo, hi;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}

static inline bool check_rdrand_support(void) {
    uint32_t eax = 1, ebx = 0, ecx = 0, edx = 0;
    __asm__ volatile (
        "cpuid"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "a"(eax), "c"(0)
    );
    return (ecx & (1 << 30)) != 0; // ECX bit 30: RDRAND
}

static inline bool rdrand64_retry(uint64_t* rand_val) {
    for (int retry = 0; retry < 10; retry++) {
        unsigned char ok;
        __asm__ volatile (
            "rdrand %0; setc %1"
            : "=r"(*rand_val), "=qm"(ok)
        );
        if (ok) return true;
    }
    return false;
}

void crypto_random_init(void) {
    g_has_rdrand = check_rdrand_support();

    SHA256_CTX ctx;
    sha256_init(&ctx);

    // 1. Hardware RDRAND Entropy (if supported by CPU)
    if (g_has_rdrand) {
        for (int i = 0; i < 8; i++) {
            uint64_t hw_rand = 0;
            if (rdrand64_retry(&hw_rand)) {
                sha256_update(&ctx, (const uint8_t*)&hw_rand, sizeof(hw_rand));
            }
        }
    }

    // 2. Hardware Timer & RTC Entropy
    uint64_t tsc = rdtsc();
    uint64_t rtc_time = rtc_get_utc_timestamp();

    sha256_update(&ctx, (const uint8_t*)&tsc, sizeof(tsc));
    sha256_update(&ctx, (const uint8_t*)&rtc_time, sizeof(rtc_time));

    // 3. PIT Timer Jitter
    for (int i = 0; i < 16; i++) {
        uint8_t pit_val = io_in8(0x40);
        sha256_update(&ctx, &pit_val, 1);
    }

    sha256_final(&ctx, g_entropy_pool);
    g_drbg_counter = 1;
    g_crypto_rand_initialized = true;
}

void crypto_random_bytes(uint8_t* buf, size_t len) {
    if (!g_crypto_rand_initialized) {
        crypto_random_init();
    }
    crypto_random_bytes_secure(buf, len);
}

bool crypto_random_bytes_secure(uint8_t* buf, size_t len) {
    if (!buf || len == 0) return false;
    if (!g_crypto_rand_initialized) {
        crypto_random_init();
    }

    size_t offset = 0;
    while (offset < len) {
        SHA256_CTX ctx;
        sha256_init(&ctx);

        uint64_t tsc = rdtsc();
        g_drbg_counter++;

        // Add hardware RDRAND value if available
        if (g_has_rdrand) {
            uint64_t hw_rand = 0;
            if (rdrand64_retry(&hw_rand)) {
                sha256_update(&ctx, (const uint8_t*)&hw_rand, sizeof(hw_rand));
            }
        }

        sha256_update(&ctx, g_entropy_pool, 32);
        sha256_update(&ctx, (const uint8_t*)&g_drbg_counter, sizeof(g_drbg_counter));
        sha256_update(&ctx, (const uint8_t*)&tsc, sizeof(tsc));

        uint8_t out_block[32];
        sha256_final(&ctx, out_block);

        size_t copy_bytes = (len - offset < 32) ? (len - offset) : 32;
        memcpy(buf + offset, out_block, copy_bytes);
        offset += copy_bytes;

        // Mix new block back into entropy pool (backtracking resistance)
        for (int i = 0; i < 32; i++) {
            g_entropy_pool[i] ^= out_block[i];
        }
    }

    return true;
}
