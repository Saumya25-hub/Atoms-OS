/*
 * ATOMS OS — Intel Extended Page Tables (EPT) Management Core
 * Copyright © 2026 ATOMS OS Project / Saumya Chaudhari
 *
 * Phase 2: Second-Level Address Translation for Intel VT-x
 */

#include "kernel/core/hypervisor/include/ept.h"
#include "kernel/core/hypervisor/include/guest_memory.h"
#include "kernel/core/memory/pmm/include/pmm.h"
#include "kernel/core/memory/vmm/include/vmm.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/core/lib/include/string.h"

extern void com1_puts(const char *s);

/* Helper to format numbers for COM1 output */
static void ept_print_hex64(uint64_t val) {
    char hex_chars[] = "0123456789ABCDEF";
    char buf[19];
    buf[0] = '0'; buf[1] = 'x';
    for (int i = 0; i < 16; i++) {
        buf[2 + i] = hex_chars[(val >> (60 - i * 4)) & 0xF];
    }
    buf[18] = '\0';
    com1_puts(buf);
}

static inline uint64_t ept_rdmsr(uint32_t msr) {
    uint32_t low, high;
    __asm__ volatile ("rdmsr" : "=a"(low), "=d"(high) : "c"(msr));
    return ((uint64_t)high << 32) | low;
}

/* Allocate and initialize an EPT PML4 table */
bool ept_init_pml4(GuestMemory *mem) {
    if (!mem) return false;

    void *pml4 = pmm_alloc_page();
    if (!pml4) {
        com1_puts("[EPT ERROR] Failed to allocate EPT PML4 root table!\n");
        return false;
    }
    memset(pml4, 0, 4096);

    uint64_t pml4_phys = (uint64_t)pml4;
    mem->ept_pml4_virt = pml4;
    mem->ept_pml4_phys = pml4_phys;

    /* Build 64-bit EPTP value: WB memory type (0x06), 4-level walk (0x18) = 0x1E */
    uint64_t eptp = (pml4_phys & EPTP_ADDR_MASK) | EPTP_MEM_TYPE_WB | EPTP_PAGE_WALK_LENGTH_4;
    uint64_t ept_cap = ept_rdmsr(IA32_VMX_EPT_VPID_CAP_MSR);
    if (ept_cap & (1ULL << 21)) {
        /* Processor supports accessed and dirty flags for EPT */
        eptp |= EPTP_ENABLE_ACCESS_DIRTY;
    }
    mem->eptp = eptp;
    return true;
}

/* Destroy all intermediate EPT page tables (recursive cleanup) */
void ept_destroy_pml4(GuestMemory *mem) {
    if (!mem || !mem->ept_pml4_virt) return;

    uint64_t *pml4 = (uint64_t *)mem->ept_pml4_virt;

    for (int pml4_idx = 0; pml4_idx < 512; pml4_idx++) {
        if ((pml4[pml4_idx] & 0x7) == 0) continue; // Not present / No permissions

        uint64_t pdpt_phys = pml4[pml4_idx] & EPT_ADDR_MASK;
        uint64_t *pdpt = (uint64_t *)pdpt_phys; // Direct mapped in kernel space

        for (int pdpt_idx = 0; pdpt_idx < 512; pdpt_idx++) {
            if ((pdpt[pdpt_idx] & 0x7) == 0) continue;

            uint64_t pd_phys = pdpt[pdpt_idx] & EPT_ADDR_MASK;
            uint64_t *pd = (uint64_t *)pd_phys;

            for (int pd_idx = 0; pd_idx < 512; pd_idx++) {
                if ((pd[pd_idx] & 0x7) == 0) continue;

                if (!(pd[pd_idx] & EPT_FLAG_PAGE_SIZE_2MB)) {
                    uint64_t pt_phys = pd[pd_idx] & EPT_ADDR_MASK;
                    pmm_free_page((void *)pt_phys);
                }
            }
            pmm_free_page((void *)pd_phys);
        }
        pmm_free_page((void *)pdpt_phys);
    }

    pmm_free_page(mem->ept_pml4_virt);
    mem->ept_pml4_virt = NULL;
    mem->ept_pml4_phys = 0;
    mem->eptp = 0;
}

/* Walk EPT page tables and locate entry for GPA, allocating intermediate levels if requested */
static uint64_t *ept_walk_entry(uint64_t *pml4, uint64_t gpa, bool create_if_missing, bool *out_is_2mb) {
    if (!pml4) return NULL;
    if (out_is_2mb) *out_is_2mb = false;

    uint64_t pml4_idx = (gpa >> 39) & 0x1FF;
    uint64_t pdpt_idx = (gpa >> 30) & 0x1FF;
    uint64_t pd_idx   = (gpa >> 21) & 0x1FF;
    uint64_t pt_idx   = (gpa >> 12) & 0x1FF;

    uint64_t intermediate_flags = EPT_FLAG_READ | EPT_FLAG_WRITE | EPT_FLAG_EXEC;

    /* Level 4: PML4 -> Level 3: PDPT */
    if ((pml4[pml4_idx] & 0x7) == 0) {
        if (!create_if_missing) return NULL;
        void *new_pdpt = pmm_alloc_page();
        if (!new_pdpt) return NULL;
        memset(new_pdpt, 0, 4096);
        uint64_t pdpt_phys = (uint64_t)new_pdpt;
        pml4[pml4_idx] = (pdpt_phys & EPT_ADDR_MASK) | intermediate_flags;
    }

    uint64_t *pdpt = (uint64_t *)(pml4[pml4_idx] & EPT_ADDR_MASK);

    /* Level 3: PDPT -> Level 2: PD */
    if ((pdpt[pdpt_idx] & 0x7) == 0) {
        if (!create_if_missing) return NULL;
        void *new_pd = pmm_alloc_page();
        if (!new_pd) return NULL;
        memset(new_pd, 0, 4096);
        uint64_t pd_phys = (uint64_t)new_pd;
        pdpt[pdpt_idx] = (pd_phys & EPT_ADDR_MASK) | intermediate_flags;
    }

    uint64_t *pd = (uint64_t *)(pdpt[pdpt_idx] & EPT_ADDR_MASK);

    /* Check for 2MB page at PD level */
    if (pd[pd_idx] & EPT_FLAG_PAGE_SIZE_2MB) {
        if (out_is_2mb) *out_is_2mb = true;
        return &pd[pd_idx];
    }

    /* Level 2: PD -> Level 1: PT */
    if ((pd[pd_idx] & 0x7) == 0) {
        if (!create_if_missing) return NULL;
        void *new_pt = pmm_alloc_page();
        if (!new_pt) return NULL;
        memset(new_pt, 0, 4096);
        uint64_t pt_phys = (uint64_t)new_pt;
        pd[pd_idx] = (pt_phys & EPT_ADDR_MASK) | intermediate_flags;
    }

    uint64_t *pt = (uint64_t *)(pd[pd_idx] & EPT_ADDR_MASK);
    return &pt[pt_idx];
}

/* Map a 4KB Guest Physical Page in EPT */
bool ept_map_page(GuestMemory *mem, uint64_t gpa, uint64_t hpa, uint32_t perms) {
    if (!mem || !mem->ept_pml4_virt) return false;

    bool is_2mb = false;
    uint64_t *pte = ept_walk_entry((uint64_t *)mem->ept_pml4_virt, gpa, true, &is_2mb);
    if (!pte || is_2mb) return false;

    uint64_t flags = (perms & 0x7ULL) | EPT_FLAG_MEM_TYPE_WB | EPT_FLAG_IGNORE_PAT;
    *pte = (hpa & EPT_ADDR_MASK) | flags;
    return true;
}

/* Map a 2MB Large Guest Physical Page in EPT */
bool ept_map_2mb_page(GuestMemory *mem, uint64_t gpa, uint64_t hpa, uint32_t perms) {
    if (!mem || !mem->ept_pml4_virt) return false;
    if ((gpa & 0x1FFFFFULL) != 0 || (hpa & 0x1FFFFFULL) != 0) return false;

    uint64_t pml4_idx = (gpa >> 39) & 0x1FF;
    uint64_t pdpt_idx = (gpa >> 30) & 0x1FF;
    uint64_t pd_idx   = (gpa >> 21) & 0x1FF;

    uint64_t *pml4 = (uint64_t *)mem->ept_pml4_virt;
    uint64_t intermediate_flags = EPT_FLAG_READ | EPT_FLAG_WRITE | EPT_FLAG_EXEC;

    if ((pml4[pml4_idx] & 0x7) == 0) {
        void *new_pdpt = pmm_alloc_page();
        if (!new_pdpt) return false;
        memset(new_pdpt, 0, 4096);
        uint64_t pdpt_phys = (uint64_t)new_pdpt;
        pml4[pml4_idx] = (pdpt_phys & EPT_ADDR_MASK) | intermediate_flags;
    }

    uint64_t *pdpt = (uint64_t *)(pml4[pml4_idx] & EPT_ADDR_MASK);
    if ((pdpt[pdpt_idx] & 0x7) == 0) {
        void *new_pd = pmm_alloc_page();
        if (!new_pd) return false;
        memset(new_pd, 0, 4096);
        uint64_t pd_phys = (uint64_t)new_pd;
        pdpt[pdpt_idx] = (pd_phys & EPT_ADDR_MASK) | intermediate_flags;
    }

    uint64_t *pd = (uint64_t *)(pdpt[pdpt_idx] & EPT_ADDR_MASK);
    uint64_t flags = (perms & 0x7ULL) | EPT_FLAG_MEM_TYPE_WB | EPT_FLAG_PAGE_SIZE_2MB | EPT_FLAG_IGNORE_PAT;
    pd[pd_idx] = (hpa & 0x000FFFFFE00000ULL) | flags;
    return true;
}

/* Unmap a page in EPT */
bool ept_unmap_page(GuestMemory *mem, uint64_t gpa) {
    if (!mem || !mem->ept_pml4_virt) return false;

    bool is_2mb = false;
    uint64_t *entry = ept_walk_entry((uint64_t *)mem->ept_pml4_virt, gpa, false, &is_2mb);
    if (!entry || (*entry & 0x7) == 0) return false;

    *entry = 0;
    return true;
}

/* Update access permissions for a page in EPT */
bool ept_set_permissions(GuestMemory *mem, uint64_t gpa, uint32_t perms) {
    if (!mem || !mem->ept_pml4_virt) return false;

    bool is_2mb = false;
    uint64_t *entry = ept_walk_entry((uint64_t *)mem->ept_pml4_virt, gpa, false, &is_2mb);
    if (!entry || (*entry & 0x7) == 0) return false;

    uint64_t base_entry = *entry & ~0x7ULL;
    *entry = base_entry | (perms & 0x7ULL);
    return true;
}

/* Translate GPA to HPA using EPT */
bool ept_translate_gpa(GuestMemory *mem, uint64_t gpa, uint64_t *out_hpa) {
    if (!mem || !mem->ept_pml4_virt || !out_hpa) return false;

    bool is_2mb = false;
    uint64_t *entry = ept_walk_entry((uint64_t *)mem->ept_pml4_virt, gpa, false, &is_2mb);
    if (!entry || (*entry & 0x7) == 0) return false;

    if (is_2mb) {
        *out_hpa = (*entry & 0x000FFFFFE00000ULL) | (gpa & 0x1FFFFFULL);
    } else {
        *out_hpa = (*entry & EPT_ADDR_MASK) | (gpa & 0xFFFULL);
    }
    return true;
}

/* Query metadata for a GPA */
bool ept_query_page(GuestMemory *mem, uint64_t gpa, GuestPageInfo *out_info) {
    if (!mem || !mem->ept_pml4_virt || !out_info) return false;

    bool is_2mb = false;
    uint64_t *entry = ept_walk_entry((uint64_t *)mem->ept_pml4_virt, gpa, false, &is_2mb);
    if (!entry || (*entry & 0x7) == 0) {
        out_info->gpa = gpa;
        out_info->hpa = 0;
        out_info->perms = GUEST_PERM_NONE;
        out_info->is_mapped = false;
        out_info->is_2mb = false;
        return false;
    }

    out_info->gpa = gpa;
    out_info->perms = (uint32_t)(*entry & 0x7);
    out_info->is_mapped = true;
    out_info->is_2mb = is_2mb;
    if (is_2mb) {
        out_info->hpa = (*entry & 0x000FFFFFE00000ULL) | (gpa & 0x1FFFFFULL);
    } else {
        out_info->hpa = (*entry & EPT_ADDR_MASK) | (gpa & 0xFFFULL);
    }
    return true;
}

/* Execute INVEPT invalidation */
void ept_invalidate_tlb(GuestMemory *mem, bool all_contexts) {
    if (!mem || !mem->eptp) return;

    invept_desc_t desc;
    desc.eptp = mem->eptp;
    desc.reserved = 0;

    uint64_t type = all_contexts ? INVEPT_TYPE_ALL_CONTEXT : INVEPT_TYPE_SINGLE_CONTEXT;
    invept_execute(type, &desc);
}

/* Handle Intel EPT Violation VM-Exit */
bool atoms_hypervisor_handle_ept_violation(vCPU *vcpu, const VMExitContext *ctx) {
    if (!vcpu || !ctx) return false;

    uint64_t qual = ctx->exit_qualification;
    uint64_t gpa  = ctx->guest_rip; // Synthetic or hardware GPA field

    com1_puts("\n[EPT VIOLATION INTERCEPT]\n");
    com1_puts("  VM ID: "); ept_print_hex64(vcpu->vm ? vcpu->vm->vm_id : 0); com1_puts("\n");
    com1_puts("  vCPU ID: "); ept_print_hex64(vcpu->id); com1_puts("\n");
    com1_puts("  Guest RIP: "); ept_print_hex64(ctx->guest_rip); com1_puts("\n");
    com1_puts("  Qualification: "); ept_print_hex64(qual); com1_puts("\n");

    if (qual & EPT_VIOLATION_DATA_READ) {
        com1_puts("  Fault Type: DATA READ violation\n");
    }
    if (qual & EPT_VIOLATION_DATA_WRITE) {
        com1_puts("  Fault Type: DATA WRITE violation\n");
    }
    if (qual & EPT_VIOLATION_INSTR_FETCH) {
        com1_puts("  Fault Type: INSTRUCTION FETCH violation\n");
    }

    /* Stop vCPU to protect host BOS memory integrity */
    com1_puts("[EPT SECURITY ACTION] Halting vCPU to prevent guest escaping partition boundary.\n");
    vcpu->state = VM_STATE_STOPPED;
    if (vcpu->vm) vcpu->vm->state = VM_STATE_STOPPED;
    return true;
}
