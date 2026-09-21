/*
 * ATOMS OS — AMD Nested Page Tables (NPT) Management Core
 * Copyright © 2026 ATOMS OS Project / Saumya Chaudhari
 *
 * Phase 2: Second-Level Address Translation for AMD SVM
 */

#include "kernel/core/hypervisor/include/npt.h"
#include "kernel/core/hypervisor/include/guest_memory.h"
#include "kernel/core/memory/pmm/include/pmm.h"
#include "kernel/core/memory/vmm/include/vmm.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/core/lib/include/string.h"

extern void com1_puts(const char *s);

static void npt_print_hex64(uint64_t val) {
    char hex_chars[] = "0123456789ABCDEF";
    char buf[19];
    buf[0] = '0'; buf[1] = 'x';
    for (int i = 0; i < 16; i++) {
        buf[2 + i] = hex_chars[(val >> (60 - i * 4)) & 0xF];
    }
    buf[18] = '\0';
    com1_puts(buf);
}

/* Allocate and initialize an NPT PML4 root table */
bool npt_init_pml4(GuestMemory *mem) {
    if (!mem) return false;

    void *pml4 = pmm_alloc_page();
    if (!pml4) {
        com1_puts("[NPT ERROR] Failed to allocate NPT PML4 root table!\n");
        return false;
    }
    memset(pml4, 0, 4096);

    uint64_t pml4_phys = (uint64_t)pml4;
    mem->npt_pml4_virt = pml4;
    mem->npt_pml4_phys = pml4_phys;
    mem->n_cr3 = pml4_phys;

    return true;
}

/* Destroy all intermediate NPT page tables */
void npt_destroy_pml4(GuestMemory *mem) {
    if (!mem || !mem->npt_pml4_virt) return;

    uint64_t *pml4 = (uint64_t *)mem->npt_pml4_virt;

    for (int pml4_idx = 0; pml4_idx < 512; pml4_idx++) {
        if (!(pml4[pml4_idx] & NPT_FLAG_PRESENT)) continue;

        uint64_t pdpt_phys = pml4[pml4_idx] & NPT_ADDR_MASK;
        uint64_t *pdpt = (uint64_t *)pdpt_phys;

        for (int pdpt_idx = 0; pdpt_idx < 512; pdpt_idx++) {
            if (!(pdpt[pdpt_idx] & NPT_FLAG_PRESENT)) continue;

            uint64_t pd_phys = pdpt[pdpt_idx] & NPT_ADDR_MASK;
            uint64_t *pd = (uint64_t *)pd_phys;

            for (int pd_idx = 0; pd_idx < 512; pd_idx++) {
                if (!(pd[pd_idx] & NPT_FLAG_PRESENT)) continue;

                if (!(pd[pd_idx] & NPT_FLAG_PAGE_SIZE_2MB)) {
                    uint64_t pt_phys = pd[pd_idx] & NPT_ADDR_MASK;
                    pmm_free_page((void *)pt_phys);
                }
            }
            pmm_free_page((void *)pd_phys);
        }
        pmm_free_page((void *)pdpt_phys);
    }

    pmm_free_page(mem->npt_pml4_virt);
    mem->npt_pml4_virt = NULL;
    mem->npt_pml4_phys = 0;
    mem->n_cr3 = 0;
}

/* Walk NPT page tables and locate entry for GPA */
static uint64_t *npt_walk_entry(uint64_t *pml4, uint64_t gpa, bool create_if_missing, bool *out_is_2mb) {
    if (!pml4) return NULL;
    if (out_is_2mb) *out_is_2mb = false;

    uint64_t pml4_idx = (gpa >> 39) & 0x1FF;
    uint64_t pdpt_idx = (gpa >> 30) & 0x1FF;
    uint64_t pd_idx   = (gpa >> 21) & 0x1FF;
    uint64_t pt_idx   = (gpa >> 12) & 0x1FF;

    uint64_t intermediate_flags = NPT_FLAG_PRESENT | NPT_FLAG_WRITE | NPT_FLAG_USER;

    /* Level 4: PML4 -> Level 3: PDPT */
    if (!(pml4[pml4_idx] & NPT_FLAG_PRESENT)) {
        if (!create_if_missing) return NULL;
        void *new_pdpt = pmm_alloc_page();
        if (!new_pdpt) return NULL;
        memset(new_pdpt, 0, 4096);
        uint64_t pdpt_phys = (uint64_t)new_pdpt;
        pml4[pml4_idx] = (pdpt_phys & NPT_ADDR_MASK) | intermediate_flags;
    }

    uint64_t *pdpt = (uint64_t *)(pml4[pml4_idx] & NPT_ADDR_MASK);

    /* Level 3: PDPT -> Level 2: PD */
    if (!(pdpt[pdpt_idx] & NPT_FLAG_PRESENT)) {
        if (!create_if_missing) return NULL;
        void *new_pd = pmm_alloc_page();
        if (!new_pd) return NULL;
        memset(new_pd, 0, 4096);
        uint64_t pd_phys = (uint64_t)new_pd;
        pdpt[pdpt_idx] = (pd_phys & NPT_ADDR_MASK) | intermediate_flags;
    }

    uint64_t *pd = (uint64_t *)(pdpt[pdpt_idx] & NPT_ADDR_MASK);

    /* Check for 2MB page */
    if (pd[pd_idx] & NPT_FLAG_PAGE_SIZE_2MB) {
        if (out_is_2mb) *out_is_2mb = true;
        return &pd[pd_idx];
    }

    /* Level 2: PD -> Level 1: PT */
    if (!(pd[pd_idx] & NPT_FLAG_PRESENT)) {
        if (!create_if_missing) return NULL;
        void *new_pt = pmm_alloc_page();
        if (!new_pt) return NULL;
        memset(new_pt, 0, 4096);
        uint64_t pt_phys = (uint64_t)new_pt;
        pd[pd_idx] = (pt_phys & NPT_ADDR_MASK) | intermediate_flags;
    }

    uint64_t *pt = (uint64_t *)(pd[pd_idx] & NPT_ADDR_MASK);
    return &pt[pt_idx];
}

/* Map a 4KB Page in NPT */
bool npt_map_page(GuestMemory *mem, uint64_t gpa, uint64_t hpa, uint32_t perms) {
    if (!mem || !mem->npt_pml4_virt) return false;

    bool is_2mb = false;
    uint64_t *pte = npt_walk_entry((uint64_t *)mem->npt_pml4_virt, gpa, true, &is_2mb);
    if (!pte || is_2mb) return false;

    uint64_t flags = NPT_FLAG_PRESENT | NPT_FLAG_USER;
    if (perms & GUEST_PERM_WRITE) flags |= NPT_FLAG_WRITE;
    if (!(perms & GUEST_PERM_EXEC)) flags |= NPT_FLAG_NX;

    *pte = (hpa & NPT_ADDR_MASK) | flags;
    return true;
}

/* Map a 2MB Page in NPT */
bool npt_map_2mb_page(GuestMemory *mem, uint64_t gpa, uint64_t hpa, uint32_t perms) {
    if (!mem || !mem->npt_pml4_virt) return false;
    if ((gpa & 0x1FFFFFULL) != 0 || (hpa & 0x1FFFFFULL) != 0) return false;

    uint64_t pml4_idx = (gpa >> 39) & 0x1FF;
    uint64_t pdpt_idx = (gpa >> 30) & 0x1FF;
    uint64_t pd_idx   = (gpa >> 21) & 0x1FF;

    uint64_t *pml4 = (uint64_t *)mem->npt_pml4_virt;
    uint64_t intermediate_flags = NPT_FLAG_PRESENT | NPT_FLAG_WRITE | NPT_FLAG_USER;

    if (!(pml4[pml4_idx] & NPT_FLAG_PRESENT)) {
        void *new_pdpt = pmm_alloc_page();
        if (!new_pdpt) return false;
        memset(new_pdpt, 0, 4096);
        uint64_t pdpt_phys = (uint64_t)new_pdpt;
        pml4[pml4_idx] = (pdpt_phys & NPT_ADDR_MASK) | intermediate_flags;
    }

    uint64_t *pdpt = (uint64_t *)(pml4[pml4_idx] & NPT_ADDR_MASK);
    if (!(pdpt[pdpt_idx] & NPT_FLAG_PRESENT)) {
        void *new_pd = pmm_alloc_page();
        if (!new_pd) return false;
        memset(new_pd, 0, 4096);
        uint64_t pd_phys = (uint64_t)new_pd;
        pdpt[pdpt_idx] = (pd_phys & NPT_ADDR_MASK) | intermediate_flags;
    }

    uint64_t *pd = (uint64_t *)(pdpt[pdpt_idx] & NPT_ADDR_MASK);
    uint64_t flags = NPT_FLAG_PRESENT | NPT_FLAG_USER | NPT_FLAG_PAGE_SIZE_2MB;
    if (perms & GUEST_PERM_WRITE) flags |= NPT_FLAG_WRITE;
    if (!(perms & GUEST_PERM_EXEC)) flags |= NPT_FLAG_NX;

    pd[pd_idx] = (hpa & 0x000FFFFFE00000ULL) | flags;
    return true;
}

/* Unmap a page in NPT */
bool npt_unmap_page(GuestMemory *mem, uint64_t gpa) {
    if (!mem || !mem->npt_pml4_virt) return false;

    bool is_2mb = false;
    uint64_t *entry = npt_walk_entry((uint64_t *)mem->npt_pml4_virt, gpa, false, &is_2mb);
    if (!entry || !(*entry & NPT_FLAG_PRESENT)) return false;

    *entry = 0;
    return true;
}

/* Update permissions in NPT */
bool npt_set_permissions(GuestMemory *mem, uint64_t gpa, uint32_t perms) {
    if (!mem || !mem->npt_pml4_virt) return false;

    bool is_2mb = false;
    uint64_t *entry = npt_walk_entry((uint64_t *)mem->npt_pml4_virt, gpa, false, &is_2mb);
    if (!entry || !(*entry & NPT_FLAG_PRESENT)) return false;

    uint64_t addr = *entry & (is_2mb ? 0x000FFFFFE00000ULL : NPT_ADDR_MASK);
    uint64_t flags = NPT_FLAG_PRESENT | NPT_FLAG_USER | (is_2mb ? NPT_FLAG_PAGE_SIZE_2MB : 0);
    if (perms & GUEST_PERM_WRITE) flags |= NPT_FLAG_WRITE;
    if (!(perms & GUEST_PERM_EXEC)) flags |= NPT_FLAG_NX;

    *entry = addr | flags;
    return true;
}

/* Translate GPA to HPA in NPT */
bool npt_translate_gpa(GuestMemory *mem, uint64_t gpa, uint64_t *out_hpa) {
    if (!mem || !mem->npt_pml4_virt || !out_hpa) return false;

    bool is_2mb = false;
    uint64_t *entry = npt_walk_entry((uint64_t *)mem->npt_pml4_virt, gpa, false, &is_2mb);
    if (!entry || !(*entry & NPT_FLAG_PRESENT)) return false;

    if (is_2mb) {
        *out_hpa = (*entry & 0x000FFFFFE00000ULL) | (gpa & 0x1FFFFFULL);
    } else {
        *out_hpa = (*entry & NPT_ADDR_MASK) | (gpa & 0xFFFULL);
    }
    return true;
}

/* Query page in NPT */
bool npt_query_page(GuestMemory *mem, uint64_t gpa, GuestPageInfo *out_info) {
    if (!mem || !mem->npt_pml4_virt || !out_info) return false;

    bool is_2mb = false;
    uint64_t *entry = npt_walk_entry((uint64_t *)mem->npt_pml4_virt, gpa, false, &is_2mb);
    if (!entry || !(*entry & NPT_FLAG_PRESENT)) {
        out_info->gpa = gpa;
        out_info->hpa = 0;
        out_info->perms = GUEST_PERM_NONE;
        out_info->is_mapped = false;
        out_info->is_2mb = false;
        return false;
    }

    out_info->gpa = gpa;
    out_info->is_mapped = true;
    out_info->is_2mb = is_2mb;
    out_info->perms = GUEST_PERM_READ;
    if (*entry & NPT_FLAG_WRITE) out_info->perms |= GUEST_PERM_WRITE;
    if (!(*entry & NPT_FLAG_NX))  out_info->perms |= GUEST_PERM_EXEC;

    if (is_2mb) {
        out_info->hpa = (*entry & 0x000FFFFFE00000ULL) | (gpa & 0x1FFFFFULL);
    } else {
        out_info->hpa = (*entry & NPT_ADDR_MASK) | (gpa & 0xFFFULL);
    }
    return true;
}

/* Handle AMD Nested Page Fault (NPF) Intercept */
bool atoms_hypervisor_handle_npt_fault(vCPU *vcpu, const VMExitContext *ctx) {
    if (!vcpu || !ctx) return false;

    com1_puts("\n[AMD NPT NESTED PAGE FAULT INTERCEPT]\n");
    com1_puts("  VM ID: "); npt_print_hex64(vcpu->vm ? vcpu->vm->vm_id : 0); com1_puts("\n");
    com1_puts("  vCPU ID: "); npt_print_hex64(vcpu->id); com1_puts("\n");
    com1_puts("  Guest RIP: "); npt_print_hex64(ctx->guest_rip); com1_puts("\n");
    com1_puts("  Exit Qualification / ErrorCode: "); npt_print_hex64(ctx->exit_qualification); com1_puts("\n");

    /* Stop vCPU to protect host BOS memory integrity */
    com1_puts("[NPT SECURITY ACTION] Halting vCPU to prevent guest escaping partition boundary.\n");
    vcpu->state = VM_STATE_STOPPED;
    if (vcpu->vm) vcpu->vm->state = VM_STATE_STOPPED;
    return true;
}
