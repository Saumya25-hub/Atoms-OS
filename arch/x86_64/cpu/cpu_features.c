#include "arch/x86_64/cpu/cpu_features.h"
#include "kernel/drivers/display/display.h"
#include "kernel/core/lib/include/crash_log.h"

static CPUFeatures g_cpu_features = {0};

static inline void cpuid(uint32_t leaf, uint32_t subleaf, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx) {
    __asm__ volatile (
        "cpuid"
        : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
        : "a"(leaf), "c"(subleaf)
    );
}

void cpu_features_init(void) {
    uint32_t eax, ebx, ecx, edx;

    // Leaf 0: Vendor String & Max Leaf
    cpuid(0, 0, &eax, &ebx, &ecx, &edx);
    *(uint32_t*)&g_cpu_features.vendor_string[0] = ebx;
    *(uint32_t*)&g_cpu_features.vendor_string[4] = edx;
    *(uint32_t*)&g_cpu_features.vendor_string[8] = ecx;
    g_cpu_features.vendor_string[12] = '\0';

    // Leaf 1: Feature Identifiers
    cpuid(1, 0, &eax, &ebx, &ecx, &edx);

    g_cpu_features.has_fpu    = (edx & (1 << 0))  != 0;
    g_cpu_features.has_fxsr   = (edx & (1 << 24)) != 0;
    g_cpu_features.has_sse    = (edx & (1 << 25)) != 0;
    g_cpu_features.has_sse2   = (edx & (1 << 26)) != 0;
    g_cpu_features.has_sse3   = (ecx & (1 << 0))  != 0;
    g_cpu_features.has_ssse3  = (ecx & (1 << 9))  != 0;
    g_cpu_features.has_sse4_1 = (ecx & (1 << 19)) != 0;
    g_cpu_features.has_sse4_2 = (ecx & (1 << 20)) != 0;
    g_cpu_features.has_xsave  = (ecx & (1 << 26)) != 0;
    g_cpu_features.has_avx    = (ecx & (1 << 28)) != 0;

    // Configure Control Register 0 (CR0)
    uint64_t cr0;
    __asm__ volatile ("mov %%cr0, %0" : "=r"(cr0));
    cr0 &= ~(1ULL << 2); // Clear EM (Emulation)
    cr0 |=  (1ULL << 1); // Set MP (Monitor Coprocessor)
    cr0 &= ~(1ULL << 3); // Clear TS (Task Switched)
    cr0 |=  (1ULL << 5); // Set NE (Numeric Error)
    __asm__ volatile ("mov %0, %%cr0" :: "r"(cr0));

    // Configure Control Register 4 (CR4)
    uint64_t cr4;
    __asm__ volatile ("mov %%cr4, %0" : "=r"(cr4));
    if (g_cpu_features.has_fxsr) {
        cr4 |= (1ULL << 9);  // Set OSFXSR
    }
    if (g_cpu_features.has_sse) {
        cr4 |= (1ULL << 10); // Set OSXMMEXCPT
    }
    __asm__ volatile ("mov %0, %%cr4" :: "r"(cr4));

    // Reset x87 FPU state
    __asm__ volatile ("fninit");

    // Initialize MXCSR with default 0x1F80 (all exceptions masked)
    if (g_cpu_features.has_sse) {
        uint32_t mxcsr = 0x1F80;
        __asm__ volatile ("ldmxcsr %0" :: "m"(mxcsr));
    }

    crash_log_add("[CPU] Feature detection & FPU/SSE control state initialized");
}

const CPUFeatures* cpu_get_features(void) {
    return &g_cpu_features;
}

void cpu_features_print(void) {
    display_print("CPU Vendor  : ");
    display_print(g_cpu_features.vendor_string);
    display_print("\nFPU         : ");
    display_print(g_cpu_features.has_fpu ? "YES" : "NO");
    display_print("\nFXSR        : ");
    display_print(g_cpu_features.has_fxsr ? "YES" : "NO");
    display_print("\nSSE / SSE2  : ");
    display_print((g_cpu_features.has_sse && g_cpu_features.has_sse2) ? "YES / YES" : "NO");
    display_print("\nXSAVE / AVX : ");
    display_print(g_cpu_features.has_xsave ? "YES" : "NO");
    display_print(" / ");
    display_print(g_cpu_features.has_avx ? "YES" : "NO");
    display_print("\n");
}
