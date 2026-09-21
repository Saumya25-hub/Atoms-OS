#include "kernel/core/cpu/cpu_state.h"
#include "arch/x86_64/cpu/cpu_features.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/core/lib/include/string.h"

typedef struct {
    bool initialized;
    CpuExtBackend backend;
    uint32_t state_size;
    uint32_t state_alignment;
    uint64_t xfeature_mask;
    uint8_t *clean_template;
    Task *current_owner;
} CpuExtendedStateEngine;

static CpuExtendedStateEngine g_cpu_ext_engine = {0};

static inline void ensure_sse_control_registers(void) {
    uint64_t cr0, cr4;

    /* CR0: Clear EM (bit 2), Set MP (bit 1), Clear TS (bit 3), Set NE (bit 5) */
    __asm__ volatile ("mov %%cr0, %0" : "=r"(cr0));
    cr0 &= ~(1ULL << 2);
    cr0 |=  (1ULL << 1);
    cr0 &= ~(1ULL << 3);
    cr0 |=  (1ULL << 5);
    __asm__ volatile ("mov %0, %%cr0" :: "r"(cr0));

    /* CR4: Set OSFXSR (bit 9), OSXMMEXCPT (bit 10), and OSXSAVE (bit 18 if supported) */
    __asm__ volatile ("mov %%cr4, %0" : "=r"(cr4));
    cr4 |= (1ULL << 9);
    cr4 |= (1ULL << 10);
    if (g_cpu_ext_engine.backend == CPU_EXT_BACKEND_XSAVE) {
        cr4 |= (1ULL << 18);
    }
    /* Enable CR4.FSGSBASE (bit 16) if supported by CPU (CPUID 7 EBX bit 0) */
    uint32_t eax7 = 0, ebx7 = 0, ecx7 = 0, edx7 = 0;
    __asm__ volatile ("cpuid" : "=a"(eax7), "=b"(ebx7), "=c"(ecx7), "=d"(edx7) : "a"(7), "c"(0));
    if (ebx7 & 1) {
        cr4 |= (1ULL << 16);
    }
    __asm__ volatile ("mov %0, %%cr4" :: "r"(cr4));
}

static inline void fxsave_raw(void *buf) {
    __asm__ volatile ("fxsave (%0)" :: "r"(buf) : "memory");
}

static inline void fxrstor_raw(const void *buf) {
    __asm__ volatile ("fxrstor (%0)" :: "r"(buf) : "memory");
}

static inline void xsave_raw(void *buf, uint64_t mask) {
    uint32_t low = (uint32_t)(mask & 0xFFFFFFFF);
    uint32_t high = (uint32_t)(mask >> 32);
    __asm__ volatile ("xsave (%0)" :: "r"(buf), "a"(low), "d"(high) : "memory");
}

static inline void xrstor_raw(const void *buf, uint64_t mask) {
    uint32_t low = (uint32_t)(mask & 0xFFFFFFFF);
    uint32_t high = (uint32_t)(mask >> 32);
    __asm__ volatile ("xrstor (%0)" :: "r"(buf), "a"(low), "d"(high) : "memory");
}

void cpu_extended_state_engine_init(void) {
    if (g_cpu_ext_engine.initialized) return;

    const CPUFeatures *features = cpu_get_features();

    if (features && features->has_xsave) {
        g_cpu_ext_engine.backend = CPU_EXT_BACKEND_XSAVE;
        g_cpu_ext_engine.state_size = (features->xsave_size_bytes >= 576) ? features->xsave_size_bytes : 576;
        g_cpu_ext_engine.state_alignment = 64;
        g_cpu_ext_engine.xfeature_mask = (1ULL << 0) | (1ULL << 1); // x87 | SSE
        if (features->has_avx && (features->xfeature_supported_mask & (1ULL << 2))) {
            g_cpu_ext_engine.xfeature_mask |= (1ULL << 2); // AVX
        }
    } else if (features && features->has_fxsr) {
        g_cpu_ext_engine.backend = CPU_EXT_BACKEND_FXSAVE;
        g_cpu_ext_engine.state_size = 512;
        g_cpu_ext_engine.state_alignment = 64; // 64-byte alignment also satisfies 16-byte FXSAVE
        g_cpu_ext_engine.xfeature_mask = 0;
    } else {
        g_cpu_ext_engine.backend = CPU_EXT_BACKEND_NONE;
        g_cpu_ext_engine.state_size = 0;
        g_cpu_ext_engine.state_alignment = 8;
        g_cpu_ext_engine.xfeature_mask = 0;
        g_cpu_ext_engine.initialized = true;
        return;
    }

    ensure_sse_control_registers();

    // Allocate known-good clean template state
    g_cpu_ext_engine.clean_template = (uint8_t *)kmalloc_aligned(g_cpu_ext_engine.state_size, g_cpu_ext_engine.state_alignment);
    if (g_cpu_ext_engine.clean_template) {
        memset(g_cpu_ext_engine.clean_template, 0, g_cpu_ext_engine.state_size);

        __asm__ volatile ("fninit");
        uint32_t mxcsr = 0x1F80;
        __asm__ volatile ("ldmxcsr %0" :: "m"(mxcsr));

        if (g_cpu_ext_engine.backend == CPU_EXT_BACKEND_XSAVE) {
            xsave_raw(g_cpu_ext_engine.clean_template, g_cpu_ext_engine.xfeature_mask);
        } else if (g_cpu_ext_engine.backend == CPU_EXT_BACKEND_FXSAVE) {
            fxsave_raw(g_cpu_ext_engine.clean_template);
        }
    }

    g_cpu_ext_engine.initialized = true;
}

CpuExtBackend cpu_extended_state_get_backend(void) {
    return g_cpu_ext_engine.backend;
}

uint32_t cpu_extended_state_get_size(void) {
    return g_cpu_ext_engine.state_size;
}

uint32_t cpu_extended_state_get_alignment(void) {
    return g_cpu_ext_engine.state_alignment;
}

bool cpu_extended_state_init_task(Task *task) {
    if (!task) return false;

    if (!g_cpu_ext_engine.initialized) {
        cpu_extended_state_engine_init();
    }

    if (g_cpu_ext_engine.backend == CPU_EXT_BACKEND_NONE) {
        task->extended_state = NULL;
        return true;
    }

    if (task->extended_state) {
        cpu_extended_state_free_task(task);
    }

    TaskExtendedContext *ctx = (TaskExtendedContext *)kmalloc(sizeof(TaskExtendedContext));
    if (!ctx) return false;

    ctx->buffer = (uint8_t *)kmalloc_aligned(g_cpu_ext_engine.state_size, g_cpu_ext_engine.state_alignment);
    if (!ctx->buffer) {
        kfree(ctx);
        return false;
    }

    if (g_cpu_ext_engine.clean_template) {
        memcpy(ctx->buffer, g_cpu_ext_engine.clean_template, g_cpu_ext_engine.state_size);
    } else {
        memset(ctx->buffer, 0, g_cpu_ext_engine.state_size);
    }

    ctx->magic = CPU_EXT_MAGIC;
    ctx->backend = g_cpu_ext_engine.backend;
    ctx->size = g_cpu_ext_engine.state_size;
    ctx->alignment = g_cpu_ext_engine.state_alignment;
    ctx->is_initialized = true;

    task->extended_state = ctx;
    return true;
}

static inline bool is_valid_kernel_pointer(const void *ptr) {
    uintptr_t addr = (uintptr_t)ptr;
    return (addr >= 0x100000ULL && (addr & 0x7) == 0);
}

void cpu_extended_state_free_task(Task *task) {
    if (!task || !task->extended_state || !is_valid_kernel_pointer(task->extended_state)) {
        if (task) task->extended_state = NULL;
        return;
    }

    TaskExtendedContext *ctx = (TaskExtendedContext *)task->extended_state;
    if (ctx->magic == CPU_EXT_MAGIC) {
        if (g_cpu_ext_engine.current_owner == task) {
            g_cpu_ext_engine.current_owner = NULL;
        }
        if (ctx->buffer && is_valid_kernel_pointer(ctx->buffer)) {
            kfree_aligned(ctx->buffer);
            ctx->buffer = NULL;
        }
        ctx->magic = 0;
        kfree(ctx);
    }
    task->extended_state = NULL;
}

void cpu_extended_state_save(Task *task) {
    if (!task || !task->extended_state || !is_valid_kernel_pointer(task->extended_state)) return;

    TaskExtendedContext *ctx = (TaskExtendedContext *)task->extended_state;
    if (ctx->magic != CPU_EXT_MAGIC || !ctx->buffer || !is_valid_kernel_pointer(ctx->buffer)) return;

    // Strict alignment verification
    if (((uintptr_t)ctx->buffer & (ctx->alignment - 1)) != 0) return;

    ensure_sse_control_registers();

    if (ctx->backend == CPU_EXT_BACKEND_XSAVE) {
        xsave_raw(ctx->buffer, g_cpu_ext_engine.xfeature_mask);
    } else if (ctx->backend == CPU_EXT_BACKEND_FXSAVE) {
        fxsave_raw(ctx->buffer);
    }

    ctx->is_initialized = true;
}

void cpu_extended_state_restore(Task *task) {
    if (!task || !task->extended_state || !is_valid_kernel_pointer(task->extended_state)) return;

    TaskExtendedContext *ctx = (TaskExtendedContext *)task->extended_state;
    if (ctx->magic != CPU_EXT_MAGIC || !ctx->buffer || !is_valid_kernel_pointer(ctx->buffer)) return;

    // Strict alignment verification
    if (((uintptr_t)ctx->buffer & (ctx->alignment - 1)) != 0) return;

    if (!ctx->is_initialized) {
        if (g_cpu_ext_engine.clean_template) {
            memcpy(ctx->buffer, g_cpu_ext_engine.clean_template, g_cpu_ext_engine.state_size);
        }
        ctx->is_initialized = true;
    }

    ensure_sse_control_registers();

    if (ctx->backend == CPU_EXT_BACKEND_XSAVE) {
        xrstor_raw(ctx->buffer, g_cpu_ext_engine.xfeature_mask);
    } else if (ctx->backend == CPU_EXT_BACKEND_FXSAVE) {
        fxrstor_raw(ctx->buffer);
    }

    g_cpu_ext_engine.current_owner = task;
}

void cpu_extended_state_context_switch(Task *old_task, Task *new_task) {
    if (old_task) {
        cpu_extended_state_save(old_task);
    }
    if (new_task) {
        cpu_extended_state_restore(new_task);
    }
}
