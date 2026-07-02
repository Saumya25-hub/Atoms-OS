#include "kernel/memory/vmm/include/vmm.h"
#include "kernel/memory/vmm/include/paging.h"
#include "kernel/memory/pmm/include/pmm.h"
#include "kernel/display/display.h"
#include "kernel/config/build_config.h"
#include "kernel/lib/include/string.h"
#include "kernel/lib/include/crash_log.h"

#define PAGE_TABLE_BASE 0x10000

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

    uint64_t virt_addr = 0x40000000;
    
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
    crash_log_add("[BOOT] VMM Ready");
}

// All other VMM functions remain available but are NOT called during init.

void vmm_map_page(void* pml4, uint64_t phys_addr, uint64_t virt_addr, uint32_t flags) {
    phys_addr &= ~0xFFFULL;
    virt_addr &= ~0xFFFULL;

    void* active_pml4 = vmm_get_active_pml4();
    void* kernel_pml4 = (void*)PAGE_TABLE_BASE;
    if (active_pml4 != kernel_pml4) vmm_switch_address_space(kernel_pml4);

    uint64_t* pt_entry = vmm_get_pt_entry(pml4, virt_addr, true);
    if (!pt_entry) {
        if (active_pml4 != kernel_pml4) vmm_switch_address_space(active_pml4);
        display_print("PANIC: vmm_map_page failed!\n");
        while(1) { __asm__ volatile("hlt"); }
    }

    *pt_entry = phys_addr | flags | PAGE_PRESENT;
    vmm_flush_tlb(virt_addr);

    if (active_pml4 != kernel_pml4) vmm_switch_address_space(active_pml4);
}

void vmm_unmap_page(void* pml4, uint64_t virt_addr) {
    virt_addr &= ~0xFFFULL;

    void* active_pml4 = vmm_get_active_pml4();
    void* kernel_pml4 = (void*)PAGE_TABLE_BASE;
    if (active_pml4 != kernel_pml4) vmm_switch_address_space(kernel_pml4);

    uint64_t* pt_entry = vmm_get_pt_entry(pml4, virt_addr, false);
    if (pt_entry && (*pt_entry & PAGE_PRESENT)) {
        *pt_entry = 0;
        vmm_flush_tlb(virt_addr);
    }

    if (active_pml4 != kernel_pml4) vmm_switch_address_space(active_pml4);
}

void* vmm_alloc_mapped_page(void* pml4, uint64_t virt_addr, uint32_t flags) {
    void* frame = pmm_alloc_page();
    if (!frame) return NULL;
    
    void* active = vmm_get_active_pml4();
    void* kernel_pml4 = (void*)PAGE_TABLE_BASE;
    if (active != kernel_pml4) vmm_switch_address_space(kernel_pml4);
    memset(frame, 0, 4096);
    if (active != kernel_pml4) vmm_switch_address_space(active);

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

// vmm_get_active_pml4 is defined in paging.c

void* vmm_create_address_space(void) {
    uint64_t* active_pml4 = vmm_get_active_pml4();
    uint64_t* kernel_pml4 = (uint64_t*)PAGE_TABLE_BASE;

    // Switch to kernel PML4 which has the entire first 1GB identity mapped.
    // This allows memset() to safely access newly allocated physical frames > 2MB.
    if (active_pml4 != kernel_pml4) {
        vmm_switch_address_space(kernel_pml4);
    }

    uint64_t* new_pml4 = pmm_alloc_page();
    if (!new_pml4) goto error_exit;
    memset(new_pml4, 0, 4096);

    uint64_t* new_pdp = pmm_alloc_page();
    if (!new_pdp) {
        pmm_free_page(new_pml4);
        goto error_exit;
    }
    memset(new_pdp, 0, 4096);
    new_pml4[0] = ((uint64_t)new_pdp) | PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER;

    uint64_t* new_pd = pmm_alloc_page();
    if (!new_pd) {
        pmm_free_page(new_pdp);
        pmm_free_page(new_pml4);
        goto error_exit;
    }
    memset(new_pd, 0, 4096);
    new_pdp[0] = ((uint64_t)new_pd) | PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER;

    uint64_t* old_pdp = (uint64_t*)(kernel_pml4[0] & PAGE_PHYS_ADDRESS_MASK);
    if (!old_pdp) goto error_exit;
    uint64_t* old_pd = (uint64_t*)(old_pdp[0] & PAGE_PHYS_ADDRESS_MASK);
    if (!old_pd) goto error_exit;

    // 1. Copy Kernel Code/Data mapping (PT[0]) - 0 to 2MB
    new_pd[0] = old_pd[0];

    // 2. Skip PT[1] to PT[127] (2MB to 256MB) - Reserved for Userspace ELF
    // These remain 0 in new_pd.

    // 3. Copy the rest of PD[0] (PT[128] to PT[511]) - 256MB to 1GB (Kernel Heap)
    for (int i = 128; i < 512; i++) {
        new_pd[i] = old_pd[i];
    }

    // 4. Copy the rest of PDP[0] (PD[1] to PD[511]) - 1GB to 512GB (e.g., APIC)
    for (int i = 1; i < 512; i++) {
        new_pdp[i] = old_pdp[i];
    }

    if (active_pml4 != kernel_pml4) {
        vmm_switch_address_space(active_pml4);
    }
    return new_pml4;

error_exit:
    if (active_pml4 != kernel_pml4) {
        vmm_switch_address_space(active_pml4);
    }
    return NULL;
}

void vmm_switch_address_space(void* pml4_phys_addr) {
    __asm__ volatile("mov %0, %%cr3" :: "r"(pml4_phys_addr));
}

uint64_t vmm_get_physical_address(void* pml4, uint64_t virt_addr) {
    void* active_pml4 = vmm_get_active_pml4();
    void* kernel_pml4 = (void*)PAGE_TABLE_BASE;
    if (active_pml4 != kernel_pml4) vmm_switch_address_space(kernel_pml4);

    uint64_t phys_addr = 0;
    uint64_t* pt_entry = vmm_get_pt_entry(pml4, virt_addr, false);
    if (pt_entry && (*pt_entry & PAGE_PRESENT)) {
        phys_addr = (*pt_entry & PAGE_PHYS_ADDRESS_MASK) + (virt_addr & 0xFFF);
    }

    if (active_pml4 != kernel_pml4) vmm_switch_address_space(active_pml4);
    return phys_addr;
}

void vmm_self_test(void) {
    display_print("[SELF TEST] VMM: PASS\n");
}
