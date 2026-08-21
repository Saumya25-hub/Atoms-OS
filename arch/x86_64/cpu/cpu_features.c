#include "arch/x86_64/cpu/cpu_features.h"
#include "kernel/debug/abde/abde.h"

static CPUFeatures g_cpu_features = {0};

/*
 * cpuid wrapper — uses "=b" output constraint so the compiler knows EBX/RBX
 * is modified. Clang generates push rbx / pop rbx in the prologue/epilogue
 * to preserve it per System V AMD64 ABI (RBX is callee-saved).
 */
static inline void cpuid(uint32_t leaf, uint32_t subleaf, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx) {
    __asm__ volatile (
        "cpuid"
        : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
        : "a"(leaf), "c"(subleaf)
    );
}

void cpu_features_init(void) {
    uint32_t eax, ebx, ecx, edx;

    diag_set_step("ENTER");

    // =========================================================================
    // 1. CPUID Leaf 0: Vendor String & Max Leaf
    // =========================================================================
    diag_set_step("BEFORE CPUID0");
    cpuid(0, 0, &eax, &ebx, &ecx, &edx);
    *(uint32_t*)&g_cpu_features.vendor_string[0] = ebx;
    *(uint32_t*)&g_cpu_features.vendor_string[4] = edx;
    *(uint32_t*)&g_cpu_features.vendor_string[8] = ecx;
    g_cpu_features.vendor_string[12] = '\0';
    diag_set_step("AFTER CPUID0");

    // =========================================================================
    // 2. CPUID Leaf 1: Feature Identifiers
    // =========================================================================
    diag_set_step("BEFORE CPUID1");
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
    diag_set_step("AFTER CPUID1");

    // =========================================================================
    // 2b. CPUID Leaf 7 & Leaf 0xD: Extended Features & XSAVE Details
    // =========================================================================
    cpuid(7, 0, &eax, &ebx, &ecx, &edx);
    g_cpu_features.has_avx2 = (ebx & (1 << 5)) != 0;

    if (g_cpu_features.has_xsave) {
        cpuid(0xD, 0, &eax, &ebx, &ecx, &edx);
        g_cpu_features.xfeature_supported_mask = ((uint64_t)edx << 32) | eax;
        g_cpu_features.xsave_size_bytes = ebx >= 576 ? ebx : 576;
        g_cpu_features.xsave_max_size_bytes = ecx;

        cpuid(0xD, 1, &eax, &ebx, &ecx, &edx);
        g_cpu_features.has_xsaveopt = (eax & (1 << 0)) != 0;
    } else {
        g_cpu_features.xsave_size_bytes = 512;
        g_cpu_features.xsave_max_size_bytes = 512;
        g_cpu_features.xfeature_supported_mask = 0;
        g_cpu_features.has_xsaveopt = false;
    }

    // =========================================================================
    // 3. Configure Control Register 0 (CR0)
    // =========================================================================
    diag_set_step("BEFORE CR0");
    uint64_t cr0;
    __asm__ volatile ("mov %%cr0, %0" : "=r"(cr0));
    cr0 &= ~(1ULL << 2); // Clear EM (Emulation)
    cr0 |=  (1ULL << 1); // Set MP (Monitor Coprocessor)
    cr0 &= ~(1ULL << 3); // Clear TS (Task Switched)
    cr0 |=  (1ULL << 5); // Set NE (Numeric Error)
    __asm__ volatile ("mov %0, %%cr0" :: "r"(cr0));
    diag_set_step("AFTER CR0");

    // =========================================================================
    // 4. Configure Control Register 4 (CR4) & XCR0
    // =========================================================================
    diag_set_step("BEFORE CR4");
    uint64_t cr4;
    __asm__ volatile ("mov %%cr4, %0" : "=r"(cr4));
    if (g_cpu_features.has_fxsr) {
        cr4 |= (1ULL << 9);  // Set OSFXSR
    }
    if (g_cpu_features.has_sse) {
        cr4 |= (1ULL << 10); // Set OSXMMEXCPT
    }
    if (g_cpu_features.has_xsave) {
        cr4 |= (1ULL << 18); // Set OSXSAVE
    }
    __asm__ volatile ("mov %0, %%cr4" :: "r"(cr4));
    diag_set_step("AFTER CR4");

    if (g_cpu_features.has_xsave) {
        uint64_t xcr0 = (1ULL << 0) | (1ULL << 1); // Enable x87 and SSE
        if (g_cpu_features.has_avx && (g_cpu_features.xfeature_supported_mask & (1ULL << 2))) {
            xcr0 |= (1ULL << 2); // Enable AVX
        }
        uint32_t low = (uint32_t)(xcr0 & 0xFFFFFFFF);
        uint32_t high = (uint32_t)(xcr0 >> 32);
        __asm__ volatile ("xsetbv" :: "a"(low), "d"(high), "c"(0));

        // Re-read exact enabled size from CPUID leaf 0xD subleaf 0
        cpuid(0xD, 0, &eax, &ebx, &ecx, &edx);
        if (ebx >= 576) {
            g_cpu_features.xsave_size_bytes = ebx;
        }
    }

    // =========================================================================
    // 5. Reset x87 FPU state
    // =========================================================================
    diag_set_step("BEFORE FNINIT");
    __asm__ volatile ("fninit");
    diag_set_step("AFTER FNINIT");

    // =========================================================================
    // 6. Initialize MXCSR with default 0x1F80 (all exceptions masked)
    // =========================================================================
    diag_set_step("BEFORE LDMXCSR");
    if (g_cpu_features.has_sse) {
        uint32_t mxcsr = 0x1F80;
        __asm__ volatile ("ldmxcsr %0" :: "m"(mxcsr));
    }
    diag_set_step("AFTER LDMXCSR");

    diag_set_step("CPU FEATURES CERTIFIED");

    // NOTE: crash_log_add() removed from early boot path.
    // crash_log depends on display subsystem and uses unverified .bss static
    // arrays during pre-IDT execution. It will be re-enabled after IDT/heap
    // initialization when the full kernel infrastructure is available.
}

const CPUFeatures* cpu_get_features(void) {
    return &g_cpu_features;
}

void cpu_features_print(void) {
    // Stubbed out during early forensic bring-up.
    // Will be re-enabled when display subsystem is fully initialized.
}
