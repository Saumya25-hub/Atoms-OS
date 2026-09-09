/*
 * ATOMS Platform Adaptation Layer (APAL)
 * Exception & Fault Boundary Adapter (Chromium V8 Guard Pages / User-mode #PF)
 */

#ifndef ATOMS_APAL_EXCEPTION_H
#define ATOMS_APAL_EXCEPTION_H

#include "../include/apal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint64_t fault_address;
    uint32_t error_code;
    uint64_t rip;
    uint64_t rsp;
} apal_fault_info_t;

typedef bool (*apal_fault_handler_t)(const apal_fault_info_t *info);

/* Registers a guard page address range (e.g. V8 stack guard or PartitionAlloc guard) */
apal_status_t apal_exception_register_guard(void *guard_addr, size_t guard_size);

/* Checks whether a fault address falls inside a registered guard page boundary */
bool apal_exception_is_guard_address(uint64_t fault_address);

/* Sets the userspace fault handler callback */
apal_status_t apal_exception_set_handler(apal_fault_handler_t handler);

#ifdef __cplusplus
}
#endif

#endif /* ATOMS_APAL_EXCEPTION_H */
