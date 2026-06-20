#include "kernel/memory/vmm/include/paging.h"
#include "kernel/memory/pmm/include/pmm.h"
#include "kernel/display/display.h"

// Simple memset implementation for zeroing out new page tables
static void vmm_memset(void* ptr, uint8_t value, uint64_t num) {
    uint8_t* p = (uint8_t*)ptr;
    for (uint64_t i = 0; i < num; i++) {
        p[i] = value;
    }
}

// Walks the page tables and returns a pointer to the Page Table Entry (PTE)
// for the given virtual address. Optionally allocates intermediate tables.
uint64_t* vmm_get_pt_entry(void* pml4, uint64_t virt_addr, bool create_if_missing) {
    uint64_t pml4_index = (virt_addr >> 39) & 0x1FF;
    uint64_t pdp_index  = (virt_addr >> 30) & 0x1FF;
    uint64_t pd_index   = (virt_addr >> 21) & 0x1FF;
    uint64_t pt_index   = (virt_addr >> 12) & 0x1FF;

    uint64_t* pml4_table = (uint64_t*)pml4;

    // Level 4 (PML4) -> Level 3 (PDP)
    if (!(pml4_table[pml4_index] & PAGE_PRESENT)) {
        if (!create_if_missing) return NULL;
        void* new_table = pmm_alloc_page();
        if (!new_table) return NULL;
        vmm_memset(new_table, 0, 4096);
        pml4_table[pml4_index] = (uint64_t)new_table | PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER;
    }

    uint64_t* pdp_table = (uint64_t*)(pml4_table[pml4_index] & PAGE_PHYS_ADDRESS_MASK);

    // Level 3 (PDP) -> Level 2 (PD)
    if (!(pdp_table[pdp_index] & PAGE_PRESENT)) {
        if (!create_if_missing) return NULL;
        void* new_table = pmm_alloc_page();
        if (!new_table) return NULL;
        vmm_memset(new_table, 0, 4096);
        pdp_table[pdp_index] = (uint64_t)new_table | PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER;
    }

    uint64_t* pd_table = (uint64_t*)(pdp_table[pdp_index] & PAGE_PHYS_ADDRESS_MASK);

    // Level 2 (PD) -> Level 1 (PT)
    // Check if it's a huge page (2MB) before assuming it points to a PT
    if (pd_table[pd_index] & PAGE_HUGE) {
        return NULL; // For now, we only handle 4KB pages in the VMM API
    }

    if (!(pd_table[pd_index] & PAGE_PRESENT)) {
        if (!create_if_missing) return NULL;
        void* new_table = pmm_alloc_page();
        if (!new_table) return NULL;
        vmm_memset(new_table, 0, 4096);
        pd_table[pd_index] = (uint64_t)new_table | PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER;
    }

    uint64_t* pt_table = (uint64_t*)(pd_table[pd_index] & PAGE_PHYS_ADDRESS_MASK);

    return &pt_table[pt_index];
}

void vmm_flush_tlb(uint64_t virt_addr) {
    __asm__ volatile("invlpg (%0)" ::"r"(virt_addr) : "memory");
}

void* vmm_get_active_pml4(void) {
    uint64_t cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
    return (void*)(cr3 & PAGE_PHYS_ADDRESS_MASK);
}
