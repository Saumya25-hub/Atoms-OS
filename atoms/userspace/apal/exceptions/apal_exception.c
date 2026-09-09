/*
 * ATOMS Platform Adaptation Layer (APAL)
 * Exception & Fault Boundary Implementation
 */

#include "apal_exception.h"
#include "atoms/userspace/runtime/include/atoms_syscall.h"
#include <string.h>

#define MAX_GUARD_RANGES 64

typedef struct {
    uint64_t start_addr;
    uint64_t end_addr;
    bool in_use;
} apal_guard_range_t;

static apal_guard_range_t g_guard_ranges[MAX_GUARD_RANGES];
static apal_fault_handler_t g_user_handler = NULL;
static bool g_exception_init = false;

static void ensure_exception_init(void) {
    if (!g_exception_init) {
        memset(g_guard_ranges, 0, sizeof(g_guard_ranges));
        g_exception_init = true;
    }
}

apal_status_t apal_exception_register_guard(void *guard_addr, size_t guard_size) {
    if (!guard_addr || guard_size == 0) return APAL_ERR_INVALID_PARAM;
    ensure_exception_init();

    uint64_t start = (uint64_t)guard_addr;
    uint64_t end = start + guard_size;

    for (int i = 0; i < MAX_GUARD_RANGES; i++) {
        if (!g_guard_ranges[i].in_use) {
            g_guard_ranges[i].in_use = true;
            g_guard_ranges[i].start_addr = start;
            g_guard_ranges[i].end_addr = end;
            return APAL_OK;
        }
    }
    return APAL_ERR_NO_MEMORY;
}

bool apal_exception_is_guard_address(uint64_t fault_address) {
    ensure_exception_init();
    for (int i = 0; i < MAX_GUARD_RANGES; i++) {
        if (g_guard_ranges[i].in_use) {
            if (fault_address >= g_guard_ranges[i].start_addr &&
                fault_address < g_guard_ranges[i].end_addr) {
                return true;
            }
        }
    }
    return false;
}

apal_status_t apal_exception_set_handler(apal_fault_handler_t handler) {
    g_user_handler = handler;
    return APAL_OK;
}
