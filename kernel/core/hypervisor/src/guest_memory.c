/*
 * ATOMS OS — Unified Guest Physical Memory Virtualization Core
 * Copyright © 2026 ATOMS OS Project / Saumya Chaudhari
 *
 * Phase 2: Second-Level Address Translation & Guest Physical Memory Manager
 */

#include "kernel/core/hypervisor/include/guest_memory.h"
#include "kernel/core/hypervisor/include/ept.h"
#include "kernel/core/hypervisor/include/npt.h"
#include "kernel/core/memory/pmm/include/pmm.h"
#include "kernel/core/memory/vmm/include/vmm.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/core/lib/include/string.h"

extern void com1_puts(const char *s);

/* Declarations from ept.c and npt.c */
extern bool ept_init_pml4(GuestMemory *mem);
extern void ept_destroy_pml4(GuestMemory *mem);
extern bool ept_map_page(GuestMemory *mem, uint64_t gpa, uint64_t hpa, uint32_t perms);
extern bool ept_map_2mb_page(GuestMemory *mem, uint64_t gpa, uint64_t hpa, uint32_t perms);
extern bool ept_unmap_page(GuestMemory *mem, uint64_t gpa);
extern bool ept_set_permissions(GuestMemory *mem, uint64_t gpa, uint32_t perms);
extern bool ept_translate_gpa(GuestMemory *mem, uint64_t gpa, uint64_t *out_hpa);
extern bool ept_query_page(GuestMemory *mem, uint64_t gpa, GuestPageInfo *out_info);
extern void ept_invalidate_tlb(GuestMemory *mem, bool all_contexts);

extern bool npt_init_pml4(GuestMemory *mem);
extern void npt_destroy_pml4(GuestMemory *mem);
extern bool npt_map_page(GuestMemory *mem, uint64_t gpa, uint64_t hpa, uint32_t perms);
extern bool npt_map_2mb_page(GuestMemory *mem, uint64_t gpa, uint64_t hpa, uint32_t perms);
extern bool npt_unmap_page(GuestMemory *mem, uint64_t gpa);
extern bool npt_set_permissions(GuestMemory *mem, uint64_t gpa, uint32_t perms);
extern bool npt_translate_gpa(GuestMemory *mem, uint64_t gpa, uint64_t *out_hpa);
extern bool npt_query_page(GuestMemory *mem, uint64_t gpa, GuestPageInfo *out_info);

/* Create and populate an isolated guest physical memory container */
GuestMemory *guest_memory_create(uint32_t vm_id, HypervisorBackend backend, uint64_t gpa_base, uint64_t size) {
    if (size == 0) return NULL;

    size_t aligned_size = (size + 4095) & ~4095ULL;
    GuestMemory *mem = (GuestMemory *)kmalloc(sizeof(GuestMemory));
    if (!mem) return NULL;
    memset(mem, 0, sizeof(GuestMemory));

    mem->vm_id = vm_id;
    mem->backend = backend;
    mem->gpa_base = gpa_base;
    mem->gpa_size = aligned_size;
    mem->page_count = aligned_size / 4096;

    /* Allocate contiguous page-aligned backing RAM */
    if (aligned_size <= (512 * 1024)) {
        mem->hva_backing = kmalloc_aligned(aligned_size, 4096);
        if (mem->hva_backing) {
            mem->hpa_backing = (uint64_t)vmm_get_physical_address(vmm_get_kernel_pml4(), (uint64_t)mem->hva_backing);
        }
    }

    if (!mem->hva_backing) {
        /* Allocate physical frames directly from Physical Memory Manager (PMM) */
        void *phys = pmm_alloc_pages_nopanic(mem->page_count);
        if (!phys) {
            com1_puts("[GUEST MEMORY ERROR] Failed to allocate guest RAM physical frames!\n");
            kfree(mem);
            return NULL;
        }
        mem->hpa_backing = (uint64_t)phys;
        mem->hva_backing = phys; /* In BOS kernel lower physical RAM is identity-mapped */
    }

    memset(mem->hva_backing, 0, aligned_size);

    /* Initialize Second-Level Address Translation root tables */
    if (backend == HYPERVISOR_BACKEND_INTEL_VMX) {
        if (!ept_init_pml4(mem)) {
            if (mem->hpa_backing == (uint64_t)mem->hva_backing) {
                pmm_free_pages((void *)mem->hpa_backing, mem->page_count);
            } else {
                kfree_aligned(mem->hva_backing);
            }
            kfree(mem);
            return NULL;
        }

        /* Identity-map guest RAM range GPA -> HPA with default RWX permissions */
        if ((aligned_size % 0x200000ULL) == 0 && (gpa_base % 0x200000ULL) == 0 && (mem->hpa_backing % 0x200000ULL) == 0) {
            for (uint64_t offset = 0; offset < aligned_size; offset += 0x200000ULL) {
                uint64_t curr_gpa = gpa_base + offset;
                uint64_t curr_hpa = mem->hpa_backing + offset;
                ept_map_2mb_page(mem, curr_gpa, curr_hpa, GUEST_PERM_RWX);
            }
        } else {
            for (uint64_t offset = 0; offset < aligned_size; offset += 4096) {
                uint64_t curr_gpa = gpa_base + offset;
                uint64_t curr_hpa = mem->hpa_backing + offset;
                ept_map_page(mem, curr_gpa, curr_hpa, GUEST_PERM_RWX);
            }
        }
    } else if (backend == HYPERVISOR_BACKEND_AMD_SVM) {
        if (!npt_init_pml4(mem)) {
            if (mem->hpa_backing == (uint64_t)mem->hva_backing) {
                pmm_free_pages((void *)mem->hpa_backing, mem->page_count);
            } else {
                kfree_aligned(mem->hva_backing);
            }
            kfree(mem);
            return NULL;
        }

        /* Identity-map guest RAM range GPA -> HPA in NPT */
        if ((aligned_size % 0x200000ULL) == 0 && (gpa_base % 0x200000ULL) == 0 && (mem->hpa_backing % 0x200000ULL) == 0) {
            for (uint64_t offset = 0; offset < aligned_size; offset += 0x200000ULL) {
                uint64_t curr_gpa = gpa_base + offset;
                uint64_t curr_hpa = mem->hpa_backing + offset;
                npt_map_2mb_page(mem, curr_gpa, curr_hpa, GUEST_PERM_RWX);
            }
        } else {
            for (uint64_t offset = 0; offset < aligned_size; offset += 4096) {
                uint64_t curr_gpa = gpa_base + offset;
                uint64_t curr_hpa = mem->hpa_backing + offset;
                npt_map_page(mem, curr_gpa, curr_hpa, GUEST_PERM_RWX);
            }
        }
    }

    mem->is_initialized = true;
    return mem;
}

/* Destroy guest memory and reclaim all physical and second-level paging frames */
void guest_memory_destroy(GuestMemory *mem) {
    if (!mem) return;

    if (mem->backend == HYPERVISOR_BACKEND_INTEL_VMX) {
        ept_destroy_pml4(mem);
    } else if (mem->backend == HYPERVISOR_BACKEND_AMD_SVM) {
        npt_destroy_pml4(mem);
    }

    if (mem->hva_backing) {
        if (mem->hpa_backing == (uint64_t)mem->hva_backing) {
            pmm_free_pages((void *)mem->hpa_backing, mem->page_count);
        } else {
            kfree_aligned(mem->hva_backing);
        }
        mem->hva_backing = NULL;
    }

    mem->is_initialized = false;
    kfree(mem);
}

/* Map single 4KB guest page with strict bounds validation */
bool guest_memory_map_page(GuestMemory *mem, uint64_t gpa, uint64_t hpa, uint32_t perms) {
    if (!mem || !mem->is_initialized) return false;

    /* Bounds check: Prevent guest mapping outside its assigned GPA window */
    if (gpa < mem->gpa_base || (gpa + 4096) > (mem->gpa_base + mem->gpa_size)) {
        return false;
    }

    if (mem->backend == HYPERVISOR_BACKEND_INTEL_VMX) {
        return ept_map_page(mem, gpa, hpa, perms);
    } else if (mem->backend == HYPERVISOR_BACKEND_AMD_SVM) {
        return npt_map_page(mem, gpa, hpa, perms);
    }
    return false;
}

/* Map 2MB large guest page */
bool guest_memory_map_2mb_page(GuestMemory *mem, uint64_t gpa, uint64_t hpa, uint32_t perms) {
    if (!mem || !mem->is_initialized) return false;

    if (gpa < mem->gpa_base || (gpa + 0x200000ULL) > (mem->gpa_base + mem->gpa_size)) {
        return false;
    }

    if (mem->backend == HYPERVISOR_BACKEND_INTEL_VMX) {
        return ept_map_2mb_page(mem, gpa, hpa, perms);
    } else if (mem->backend == HYPERVISOR_BACKEND_AMD_SVM) {
        return npt_map_2mb_page(mem, gpa, hpa, perms);
    }
    return false;
}

/* Unmap guest physical page */
bool guest_memory_unmap_page(GuestMemory *mem, uint64_t gpa) {
    if (!mem || !mem->is_initialized) return false;

    if (gpa < mem->gpa_base || gpa >= (mem->gpa_base + mem->gpa_size)) {
        return false;
    }

    if (mem->backend == HYPERVISOR_BACKEND_INTEL_VMX) {
        return ept_unmap_page(mem, gpa);
    } else if (mem->backend == HYPERVISOR_BACKEND_AMD_SVM) {
        return npt_unmap_page(mem, gpa);
    }
    return false;
}

/* Update access permissions */
bool guest_memory_set_permissions(GuestMemory *mem, uint64_t gpa, uint32_t perms) {
    if (!mem || !mem->is_initialized) return false;

    if (gpa < mem->gpa_base || gpa >= (mem->gpa_base + mem->gpa_size)) {
        return false;
    }

    if (mem->backend == HYPERVISOR_BACKEND_INTEL_VMX) {
        return ept_set_permissions(mem, gpa, perms);
    } else if (mem->backend == HYPERVISOR_BACKEND_AMD_SVM) {
        return npt_set_permissions(mem, gpa, perms);
    }
    return false;
}

/* Translate GPA -> HPA */
bool guest_memory_translate_gpa(GuestMemory *mem, uint64_t gpa, uint64_t *out_hpa) {
    if (!mem || !mem->is_initialized || !out_hpa) return false;

    if (gpa < mem->gpa_base || gpa >= (mem->gpa_base + mem->gpa_size)) {
        return false;
    }

    if (mem->backend == HYPERVISOR_BACKEND_INTEL_VMX) {
        return ept_translate_gpa(mem, gpa, out_hpa);
    } else if (mem->backend == HYPERVISOR_BACKEND_AMD_SVM) {
        return npt_translate_gpa(mem, gpa, out_hpa);
    }
    return false;
}

/* Query page translation info */
bool guest_memory_query_page(GuestMemory *mem, uint64_t gpa, GuestPageInfo *out_info) {
    if (!mem || !mem->is_initialized || !out_info) return false;

    if (gpa < mem->gpa_base || gpa >= (mem->gpa_base + mem->gpa_size)) {
        out_info->gpa = gpa;
        out_info->hpa = 0;
        out_info->perms = GUEST_PERM_NONE;
        out_info->is_mapped = false;
        out_info->is_2mb = false;
        return false;
    }

    if (mem->backend == HYPERVISOR_BACKEND_INTEL_VMX) {
        return ept_query_page(mem, gpa, out_info);
    } else if (mem->backend == HYPERVISOR_BACKEND_AMD_SVM) {
        return npt_query_page(mem, gpa, out_info);
    }
    return false;
}

/* Validate that an entire GPA range is mapped and possesses required permissions */
bool guest_memory_validate_gpa_range(GuestMemory *mem, uint64_t gpa, uint64_t size, uint32_t req_perms) {
    if (!mem || !mem->is_initialized || size == 0) return false;

    if (gpa < mem->gpa_base || (gpa + size) > (mem->gpa_base + mem->gpa_size)) {
        return false;
    }

    GuestPageInfo info;
    for (uint64_t offset = 0; offset < size; offset += 4096) {
        if (!guest_memory_query_page(mem, gpa + offset, &info)) {
            return false;
        }
        if (!info.is_mapped || (info.perms & req_perms) != req_perms) {
            return false;
        }
    }
    return true;
}

/* Invalidate second-level TLB */
void guest_memory_invalidate_tlb(GuestMemory *mem, uint64_t gpa, bool all_contexts) {
    (void)gpa;
    if (!mem || !mem->is_initialized) return;

    if (mem->backend == HYPERVISOR_BACKEND_INTEL_VMX) {
        ept_invalidate_tlb(mem, all_contexts);
    }
}
