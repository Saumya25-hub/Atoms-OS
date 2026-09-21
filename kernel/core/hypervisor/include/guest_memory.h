/*
 * ATOMS OS — Unified Guest Physical Memory Virtualization Interface
 * Copyright © 2026 ATOMS OS Project / Saumya Chaudhari
 *
 * Phase 2: Second-Level Address Translation (EPT/NPT) Management Core
 */

#ifndef ATOMS_GUEST_MEMORY_H
#define ATOMS_GUEST_MEMORY_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "kernel/core/hypervisor/include/hypervisor.h"
#include "kernel/core/hypervisor/include/ept.h"
#include "kernel/core/hypervisor/include/npt.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Guest Memory Access Permission Flags */
typedef enum {
    GUEST_PERM_NONE  = 0,
    GUEST_PERM_READ  = (1U << 0),
    GUEST_PERM_WRITE = (1U << 1),
    GUEST_PERM_EXEC  = (1U << 2),
    GUEST_PERM_RW    = (GUEST_PERM_READ | GUEST_PERM_WRITE),
    GUEST_PERM_RX    = (GUEST_PERM_READ | GUEST_PERM_EXEC),
    GUEST_PERM_RWX   = (GUEST_PERM_READ | GUEST_PERM_WRITE | GUEST_PERM_EXEC)
} GuestMemoryPerm;

/* Information Record for a Query on a GPA */
typedef struct {
    uint64_t gpa;
    uint64_t hpa;
    uint32_t perms;
    bool     is_mapped;
    bool     is_2mb;
} GuestPageInfo;

/* Guest Memory Object Structure */
typedef struct guest_memory {
    uint32_t            vm_id;
    HypervisorBackend   backend;
    uint64_t            gpa_base;
    uint64_t            gpa_size;
    uint64_t            page_count;

    /* Host Backing Virtual & Physical Pointers */
    void               *hva_backing;
    uint64_t            hpa_backing;

    /* Second-Level Paging Root Table (PML4) */
    void               *ept_pml4_virt;
    uint64_t            ept_pml4_phys;
    uint64_t            eptp;            /* Configured EPTP for Intel VMCS */

    void               *npt_pml4_virt;
    uint64_t            npt_pml4_phys;
    uint64_t            n_cr3;           /* Configured n_cr3 for AMD VMCB */

    bool                is_initialized;
} GuestMemory;

/* Guest Memory Management APIs */
GuestMemory *guest_memory_create(uint32_t vm_id, HypervisorBackend backend, uint64_t gpa_base, uint64_t size);
void guest_memory_destroy(GuestMemory *mem);

/* Page Mapping & Unmapping (4KB Baseline and 2MB Large Pages) */
bool guest_memory_map_page(GuestMemory *mem, uint64_t gpa, uint64_t hpa, uint32_t perms);
bool guest_memory_map_2mb_page(GuestMemory *mem, uint64_t gpa, uint64_t hpa, uint32_t perms);
bool guest_memory_unmap_page(GuestMemory *mem, uint64_t gpa);

/* Permission & Translation Inspection */
bool guest_memory_set_permissions(GuestMemory *mem, uint64_t gpa, uint32_t perms);
bool guest_memory_translate_gpa(GuestMemory *mem, uint64_t gpa, uint64_t *out_hpa);
bool guest_memory_query_page(GuestMemory *mem, uint64_t gpa, GuestPageInfo *out_info);
bool guest_memory_validate_gpa_range(GuestMemory *mem, uint64_t gpa, uint64_t size, uint32_t req_perms);

/* TLB & Second-Level Page Table Invalidation */
void guest_memory_invalidate_tlb(GuestMemory *mem, uint64_t gpa, bool all_contexts);

/* Violation & Fault Intercept Dispatchers */
bool atoms_hypervisor_handle_ept_violation(vCPU *vcpu, const VMExitContext *ctx);
bool atoms_hypervisor_handle_npt_fault(vCPU *vcpu, const VMExitContext *ctx);

#ifdef __cplusplus
}
#endif

#endif /* ATOMS_GUEST_MEMORY_H */
