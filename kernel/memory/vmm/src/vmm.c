#include "kernel/memory/vmm/include/vmm.h"
#include "kernel/memory/vmm/include/paging.h"
#include "kernel/memory/pmm/include/pmm.h"
#include "kernel/display/display.h"
#include "kernel/config/build_config.h"

static void vmm_walk(uint64_t* pml4_table) {
    if (!pml4_table || (uint64_t)pml4_table > 0x200000) {
        display_print("[VMM] INVALID ENTRY: PML4 ptr\n");
        return;
    }

    display_print("\n[VMM] Page Walk (Index 0)\n");

    uint64_t pml4e = pml4_table[0];
    if (!(pml4e & PAGE_PRESENT)) {
        display_print("PML4[0] NOT PRESENT\n");
        return;
    }
    
    uint64_t* pdp_table = (uint64_t*)(pml4e & PAGE_PHYS_ADDRESS_MASK);
    display_print("PML4[0] -> "); display_print_hex((uint64_t)pdp_table); display_print("\n");
    
    if ((uint64_t)pdp_table > 0x200000) {
        display_print("[VMM] INVALID ENTRY: PDP ptr\n");
        return;
    }

    uint64_t pdpe = pdp_table[0];
    if (!(pdpe & PAGE_PRESENT)) {
        display_print("PDP[0] NOT PRESENT\n");
        return;
    }

    uint64_t* pd_table = (uint64_t*)(pdpe & PAGE_PHYS_ADDRESS_MASK);
    display_print("PDP[0]  -> "); display_print_hex((uint64_t)pd_table); display_print("\n");

    if ((uint64_t)pd_table > 0x200000) {
        display_print("[VMM] INVALID ENTRY: PD ptr\n");
        return;
    }

    uint64_t pde = pd_table[0];
    if (!(pde & PAGE_PRESENT)) {
        display_print("PD[0] NOT PRESENT\n");
        return;
    }

    if (pde & PAGE_HUGE) {
        display_print("PD[0] HUGE PAGE -> ");
        display_print_hex(pde & PAGE_PHYS_ADDRESS_MASK);
        display_print("\n");
        return;
    }

    uint64_t* pt_table = (uint64_t*)(pde & PAGE_PHYS_ADDRESS_MASK);
    display_print("PD[0]   -> "); display_print_hex((uint64_t)pt_table); display_print("\n");

    if ((uint64_t)pt_table > 0x200000) {
        display_print("[VMM] INVALID ENTRY: PT ptr\n");
        return;
    }

    display_print("\n");
    for (int i = 0; i < 12; i++) {
        uint64_t pte = pt_table[i];
        display_print("PT["); display_print_dec(i); display_print("]   -> ");
        if (pte & PAGE_PRESENT) {
            display_print_hex(pte & PAGE_PHYS_ADDRESS_MASK);
        } else {
            display_print("NOT PRESENT");
        }
        display_print("\n");
    }
}

void vmm_init(void) {
    display_print("\nVMM OK\n");

    // Read CR3. DO NOT WRITE CR3.
    uint64_t cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
    display_print("CR3 = "); display_print_hex(cr3); display_print("\n");

    void* pml4_addr = (void*)(cr3 & 0x000FFFFFFFFFF000ULL);
    display_print("PML4 = "); display_print_hex((uint64_t)pml4_addr); display_print("\n");

    // Step 3: First Page Mapping
    display_print("\n[VMM]\n");
    
    void* phys_frame = pmm_alloc_page();
    if (!phys_frame) {
        display_print("[VMM] PMM FAILED\n");
        return;
    }
    
    display_print("Allocated Physical:\n");
    display_print_hex((uint64_t)phys_frame); display_print("\n\n");

    uint64_t virt_addr = 0x400000;
    
    uint64_t* pt_entry = vmm_get_pt_entry(pml4_addr, virt_addr, true);
    if (!pt_entry) {
        display_print("[VMM] PT ALLOCATION FAILED\n");
        return;
    }

    if (*pt_entry & PAGE_PRESENT) {
        display_print("[VMM] PAGE ALREADY MAPPED\n");
        return;
    }

    *pt_entry = (uint64_t)phys_frame | PAGE_PRESENT | PAGE_WRITABLE;
    vmm_flush_tlb(virt_addr);

    display_print("Mapped\n");
    display_print_hex(virt_addr); display_print(" -> "); display_print_hex((uint64_t)phys_frame); display_print("\n\n");

    display_print("Translation Verified\n");
    
    uint64_t read_back = vmm_get_physical_address(pml4_addr, virt_addr);
    if (read_back != (uint64_t)phys_frame) {
        display_print("[VMM] VERIFY FAILED\n");
        return;
    }

    display_print("Virtual\n");
    display_print_hex(virt_addr); display_print("\n");
    display_print("Physical\n");
    display_print_hex(read_back); display_print("\n\n");

    if (*pt_entry & PAGE_PRESENT) {
        display_print("Present Bit   PASS\n");
    }
    if (*pt_entry & PAGE_WRITABLE) {
        display_print("Writable Bit  PASS\n");
    }

    display_print("\n[VMM] STEP 3 PASS\n");
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
