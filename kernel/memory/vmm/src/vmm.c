#include "kernel/memory/vmm/include/vmm.h"
#include "kernel/memory/vmm/include/paging.h"
#include "kernel/memory/pmm/include/pmm.h"
#include "kernel/display/display.h"
#include "kernel/config/build_config.h"

void vmm_init(void) {
    display_print("VMM A\n");

    // Read CR3. DO NOT WRITE CR3.
    uint64_t cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
    display_print("VMM B CR3="); display_print_hex(cr3); display_print("\n");

    void* pml4_addr = (void*)(cr3 & 0x000FFFFFFFFFF000ULL);
    display_print("VMM C PML4="); display_print_hex((uint64_t)pml4_addr); display_print("\n");

    display_print("VMM D\n");

    // NO page table modifications.
    // NO CR3 writes.
    // NO TLB flushes.
    // NO mappings.
    // Pure inspection only.

    display_print("VMM E\n");
    display_print("[VMM] Init OK.\n");
    display_print("VMM F\n");
}

// All other VMM functions remain available but are NOT called during init.

void vmm_map_page(void* pml4, uint64_t phys_addr, uint64_t virt_addr, uint32_t flags) {
    phys_addr &= ~0xFFFULL;
    virt_addr &= ~0xFFFULL;

    uint64_t* pt_entry = vmm_get_pt_entry(pml4, virt_addr, true);
    if (!pt_entry) {
        display_print("PANIC: vmm_map_page failed!\n");
        while(1) { __asm__ volatile("hlt"); }
    }

    *pt_entry = phys_addr | flags | PAGE_PRESENT;
    vmm_flush_tlb(virt_addr);
}

void vmm_unmap_page(void* pml4, uint64_t virt_addr) {
    virt_addr &= ~0xFFFULL;

    uint64_t* pt_entry = vmm_get_pt_entry(pml4, virt_addr, false);
    if (pt_entry && (*pt_entry & PAGE_PRESENT)) {
        *pt_entry = 0;
        vmm_flush_tlb(virt_addr);
    }
}

void* vmm_alloc_mapped_page(void* pml4, uint64_t virt_addr, uint32_t flags) {
    void* frame = pmm_alloc_page();
    if (!frame) return NULL;
    
    vmm_map_page(pml4, (uint64_t)frame, virt_addr, flags);
    return frame;
}

void vmm_free_mapped_page(void* pml4, uint64_t virt_addr) {
    uint64_t phys_addr = vmm_get_physical_address(pml4, virt_addr);
    if (phys_addr) {
        vmm_unmap_page(pml4, virt_addr);
        pmm_free_page((void*)phys_addr);
    }
}

void* vmm_create_address_space(void) {
    return NULL;
}

void vmm_switch_address_space(void* pml4_phys_addr) {
    __asm__ volatile("mov %0, %%cr3" :: "r"(pml4_phys_addr));
}

uint64_t vmm_get_physical_address(void* pml4, uint64_t virt_addr) {
    uint64_t* pt_entry = vmm_get_pt_entry(pml4, virt_addr, false);
    if (pt_entry && (*pt_entry & PAGE_PRESENT)) {
        return (*pt_entry & PAGE_PHYS_ADDRESS_MASK) + (virt_addr & 0xFFF);
    }
    return 0;
}
