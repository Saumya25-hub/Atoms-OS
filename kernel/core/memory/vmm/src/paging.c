#include "kernel/core/memory/vmm/include/paging.h"
#include "kernel/core/memory/pmm/include/pmm.h"
#include "kernel/drivers/display/display.h"

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
    bool is_user = (virt_addr >= 0x40000000ULL && virt_addr < 0x80000000ULL);
    uint64_t table_flags = PAGE_PRESENT | PAGE_WRITABLE | (is_user ? PAGE_USER : 0);

    // Level 4 (PML4) -> Level 3 (PDP)
    if (!(pml4_table[pml4_index] & PAGE_PRESENT)) {
        if (!create_if_missing) return NULL;
        void* new_table = pmm_alloc_page();
        if (!new_table) return NULL;
        vmm_memset(new_table, 0, 4096);
        pml4_table[pml4_index] = (uint64_t)new_table | table_flags;
    } else if (create_if_missing && is_user) {
        pml4_table[pml4_index] |= (PAGE_WRITABLE | PAGE_USER);
    }

    uint64_t* pdp_table = (uint64_t*)(pml4_table[pml4_index] & PAGE_PHYS_ADDRESS_MASK);

    // Level 3 (PDP) -> Level 2 (PD)
    if (pdp_table[pdp_index] & PAGE_HUGE) {
        void* new_table = pmm_alloc_page();
        if (!new_table) return NULL;

        uint64_t huge_phys_base = pdp_table[pdp_index] & ~0x3FFFFFFFULL & PAGE_PHYS_ADDRESS_MASK;
        uint64_t pdpe_flags = (pdp_table[pdp_index] & ~PAGE_PHYS_ADDRESS_MASK) | PAGE_PRESENT | PAGE_WRITABLE;

        uint64_t* pd = (uint64_t*)new_table;
        for (int i = 0; i < 512; i++) {
            pd[i] = (huge_phys_base + ((uint64_t)i * 0x200000ULL)) | pdpe_flags;
        }

        pdp_table[pdp_index] = (uint64_t)new_table | table_flags;
        vmm_flush_tlb(virt_addr);
    }

    if (!(pdp_table[pdp_index] & PAGE_PRESENT)) {
        if (!create_if_missing) return NULL;
        void* new_table = pmm_alloc_page();
        if (!new_table) return NULL;
        vmm_memset(new_table, 0, 4096);
        pdp_table[pdp_index] = (uint64_t)new_table | table_flags;
    } else if (create_if_missing && is_user) {
        pdp_table[pdp_index] |= (PAGE_WRITABLE | PAGE_USER);
    }

    uint64_t* pd_table = (uint64_t*)(pdp_table[pdp_index] & PAGE_PHYS_ADDRESS_MASK);

    // Level 2 (PD) -> Level 1 (PT)
    // If it's a huge page (2MB), split it into a 4KB Page Table (PT)
    if (pd_table[pd_index] & PAGE_HUGE) {
        void* new_table = pmm_alloc_page();
        if (!new_table) return NULL;

        uint64_t huge_phys_base = pd_table[pd_index] & ~0x1FFFFFULL & PAGE_PHYS_ADDRESS_MASK;
        uint64_t pde_flags = pd_table[pd_index] & ~PAGE_PHYS_ADDRESS_MASK;
        uint64_t pte_flags = (pde_flags & ~PAGE_HUGE) | PAGE_PRESENT | PAGE_WRITABLE; // Remove HUGE bit for 4KB PTEs

        uint64_t* pt = (uint64_t*)new_table;
        for (int i = 0; i < 512; i++) {
            pt[i] = (huge_phys_base + ((uint64_t)i * 4096)) | pte_flags;
        }

        // Update PDE to point to the new 4KB Page Table (without PAGE_HUGE bit)
        pd_table[pd_index] = (uint64_t)new_table | table_flags;
        vmm_flush_tlb(virt_addr);
    }

    if (!(pd_table[pd_index] & PAGE_PRESENT)) {
        if (!create_if_missing) return NULL;
        void* new_table = pmm_alloc_page();
        if (!new_table) return NULL;
        vmm_memset(new_table, 0, 4096);
        pd_table[pd_index] = (uint64_t)new_table | table_flags;
    } else if (create_if_missing && is_user) {
        pd_table[pd_index] |= (PAGE_WRITABLE | PAGE_USER);
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
