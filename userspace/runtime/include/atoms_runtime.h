/*
 * ATOMS OS — Canonical Unified BOS Ring 3 Runtime Foundation API
 * Copyright © 2026 ATOMS OS Project / Saumya Chaudhari
 *
 * This header serves as the master contract for all Ring 3 runtimes,
 * including future embedded JVMs (Avian), JavaScript engines (V8),
 * rendering engines (Skia/Blink), and native userland services.
 */

#ifndef ATOMS_RUNTIME_H
#define ATOMS_RUNTIME_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/* C Standard Library Subsystems */
#include "../c/include/atoms_syscall.h"
#include "../c/include/errno.h"
#include "../c/include/assert.h"
#include "../c/include/setjmp.h"
#include "../c/include/math.h"
#include "../c/include/stdio.h"
#include "../c/include/stdlib.h"
#include "../c/include/string.h"
#include "../c/include/time.h"
#include "../c/include/unistd.h"
#include "../c/include/sys/mman.h"
#include "../c/include/pthread.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Runtime Version & Capability Metadata
 */
#define ATOMS_RUNTIME_VERSION_MAJOR 1
#define ATOMS_RUNTIME_VERSION_MINOR 0
#define ATOMS_RUNTIME_NAME          "BOS Ring 3 Native Runtime"

typedef struct {
    uint32_t version_major;
    uint32_t version_minor;
    bool     has_tls_msr;
    bool     has_futex;
    bool     has_wx_mprotect;
    bool     has_file_mmap;
    bool     has_setjmp_abi;
    bool     has_ieee754_math;
} atoms_runtime_caps_t;

/*
 * Initialization & Teardown
 */
void __libc_init_array(void);
void __libc_fini_array(void);

static inline void atoms_runtime_get_caps(atoms_runtime_caps_t *caps) {
    if (!caps) return;
    caps->version_major  = ATOMS_RUNTIME_VERSION_MAJOR;
    caps->version_minor  = ATOMS_RUNTIME_VERSION_MINOR;
    caps->has_tls_msr    = true;
    caps->has_futex      = true;
    caps->has_wx_mprotect= true;
    caps->has_file_mmap  = true;
    caps->has_setjmp_abi = true;
    caps->has_ieee754_math = true;
}

static inline void atoms_runtime_banner(void) {
    puts("=================================================");
    puts("  ATOMS OS — BOS Ring 3 Runtime Foundation V1.0  ");
    puts("  Ready for Native Embedded Java Runtime (Avian) ");
    puts("=================================================");
}

#ifdef __cplusplus
}
#endif

#endif /* ATOMS_RUNTIME_H */
