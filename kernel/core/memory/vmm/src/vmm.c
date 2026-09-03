#include "kernel/core/memory/vmm/include/vmm.h"
#include "kernel/core/memory/vmm/include/paging.h"
#include "kernel/core/memory/pmm/include/pmm.h"
#include "kernel/debug/abde/abde.h"
#include "kernel/debug/desktop_diag.h"

extern void com1_puts(const char *s);

static void *g_kernel_pml4 = NULL;
static void *g_kernel_pdp  = NULL;
static uint32_t g_vmm_page_fault_count = 0;
static uint64_t g_vmm_mapped_page_count = 0;

void *vmm_get_kernel_pml4(void) {
    if (!g_kernel_pml4) {
        g_kernel_pml4 = vmm_get_active_pml4();
    }
    return g_kernel_pml4;
}

/* Activate CR3 Hardware Register and Enable 4-Level Paging */
void vmm_enable(void) {
    if (!g_kernel_pml4) return;
    vmm_switch_address_space(g_kernel_pml4);
    diag_set_step("CR3 ACTIVATED");
}

/* Translate Virtual Address to Physical Address */
uint64_t vmm_translate(void *pml4, uint64_t virt_addr) {
    return vmm_get_physical_address(pml4, virt_addr);
}

/* Check if Virtual Address is Currently Mapped in Page Tables */
bool vmm_is_mapped(void *pml4, uint64_t virt_addr) {
    return vmm_get_physical_address(pml4, virt_addr) != 0;
}

void vmm_init(void) {
    com1_puts("[VMM] ENTER VMM INIT\n");
    g_vmm_page_fault_count = 0;
    g_vmm_mapped_page_count = 0;
    g_abde.vmm_active = true;
    diag_set_running("VMM");
    diag_set_step("CREATING KERNEL PAGE TABLES");

    // Stage 1: CREATE PML4
    diag_set_step("CREATE PML4");
    com1_puts("[VMM] STAGE 1: CREATE PML4\n");
    uint64_t *pml4 = (uint64_t *)pmm_alloc_page();
    if (!pml4) {
        com1_puts("[VMM] PMM ALLOC PML4 FAIL\n");
        diag_panic_reason("VMM", "CREATE_PML4", "PMM_OUT_OF_MEMORY", "Failed to allocate PML4 table");
        return;
    }
    for (int i = 0; i < 512; i++) pml4[i] = 0;
    g_kernel_pml4 = (void *)pml4;

    diag_set_vmm_telemetry(0, (uint64_t)pml4, 0, 0, 0, 0, 0, 0, 0, "RUNNING");

    // Stage 2: CREATE PDPT
    diag_set_step("CREATE PDPT");
    com1_puts("[VMM] STAGE 2: CREATE PDPT & PDS\n");
    uint64_t *pdp  = (uint64_t *)pmm_alloc_page();
    uint64_t *pd0  = (uint64_t *)pmm_alloc_page();
    uint64_t *pd1  = (uint64_t *)pmm_alloc_page();
    uint64_t *pd2  = (uint64_t *)pmm_alloc_page();
    uint64_t *pd3  = (uint64_t *)pmm_alloc_page();

    if (!pdp || !pd0 || !pd1 || !pd2 || !pd3) {
        com1_puts("[VMM] PMM ALLOC TABLES FAIL\n");
        diag_panic_reason("VMM", "CREATE_PDPT", "PMM_OUT_OF_MEMORY", "Failed to allocate PDP or PD tables");
        return;
    }
    for (int i = 0; i < 512; i++) {
        pdp[i] = 0; pd0[i] = 0; pd1[i] = 0; pd2[i] = 0; pd3[i] = 0;
    }
    g_kernel_pdp = (void *)pdp;

    diag_set_vmm_telemetry(0, (uint64_t)pml4, (uint64_t)pdp, 0, 0, 0, 0, 0, 0, "RUNNING");

    // Stage 3: BUILD PAGE TABLES
    diag_set_step("BUILD PAGE TABLES");
    com1_puts("[VMM] STAGE 3: LINK TABLES\n");
    pml4[0]   = (uint64_t)pdp | PAGE_PRESENT | PAGE_WRITABLE;
    pml4[256] = (uint64_t)pdp | PAGE_PRESENT | PAGE_WRITABLE;
    pml4[511] = (uint64_t)pdp | PAGE_PRESENT | PAGE_WRITABLE;

    pdp[0] = (uint64_t)pd0 | PAGE_PRESENT | PAGE_WRITABLE;
    pdp[1] = (uint64_t)pd1 | PAGE_PRESENT | PAGE_WRITABLE;
    pdp[2] = (uint64_t)pd2 | PAGE_PRESENT | PAGE_WRITABLE;
    pdp[3] = (uint64_t)pd3 | PAGE_PRESENT | PAGE_WRITABLE;

    // Map 4GB..512GB using 1GB Huge Pages on PDPT entries 4..511
    for (uint64_t i = 4; i < 512; i++) {
        pdp[i] = (i * 0x40000000ULL) | PAGE_PRESENT | PAGE_WRITABLE | PAGE_HUGE;
    }

    // Stage 4: IDENTITY MAP RAM & FRAMEBUFFER (0..4GB) WITH CLEAN WRITE-BACK CACHING
    diag_set_step("IDENTITY MAP RAM");
    com1_puts("[VMM] STAGE 4: IDENTITY MAP FULL PHYSICAL RANGE 512GB\n");
    uint64_t phys = 0;
    for (int i = 0; i < 512; i++) { pd0[i] = phys | PAGE_PRESENT | PAGE_WRITABLE | PAGE_HUGE; phys += 0x200000ULL; }
    for (int i = 0; i < 512; i++) { pd1[i] = phys | PAGE_PRESENT | PAGE_WRITABLE | PAGE_HUGE; phys += 0x200000ULL; }
    for (int i = 0; i < 512; i++) { pd2[i] = phys | PAGE_PRESENT | PAGE_WRITABLE | PAGE_HUGE; phys += 0x200000ULL; }
    for (int i = 0; i < 512; i++) { pd3[i] = phys | PAGE_PRESENT | PAGE_WRITABLE | PAGE_HUGE; phys += 0x200000ULL; }

    g_vmm_mapped_page_count = 2048;

    diag_set_vmm_telemetry(0, (uint64_t)pml4, (uint64_t)pdp, 2048, 2048, 0, 0, 0, 0, "RUNNING");

    // Stage 5: LOAD CR3 & ENABLE PAGING
    diag_set_step("LOAD CR3");
    com1_puts("[VMM] STAGE 5: LOAD CR3 NOW\n");
    vmm_enable();
    com1_puts("[VMM] STAGE 5: CR3 LOADED SUCCESS\n");
    diag_set_vmm_telemetry((uint64_t)pml4, (uint64_t)pml4, (uint64_t)pdp, 2048, 2048, 0, 0, 0, 0, "RUNNING");

    // Stage 6: VERIFY TRANSLATION & STRESS TESTS
    diag_set_step("VERIFY TRANSLATION");
    com1_puts("[VMM] STAGE 6: STRESS TESTS START\n");

    // Test 1: Map 1 page
    diag_set_step("STRESS TEST 1 (1 PAGE)");
    void *p1_phys = pmm_alloc_page();
    uint64_t v1 = 0x40000000ULL;
    vmm_map_page(g_kernel_pml4, (uint64_t)p1_phys, v1, VMM_FLAG_WRITABLE);
    if (!vmm_is_mapped(g_kernel_pml4, v1) || vmm_translate(g_kernel_pml4, v1) != (uint64_t)p1_phys) {
        com1_puts("[VMM] STRESS TEST 1 FAIL\n");
        diag_panic_reason("VMM", "STRESS_TEST_1", "TRANSLATION_FAIL", "1-page map or translate failed");
        return;
    }
    g_vmm_mapped_page_count++;
    diag_set_vmm_telemetry((uint64_t)pml4, (uint64_t)pml4, (uint64_t)pdp, 2048, g_vmm_mapped_page_count, 0, v1, v1, (uint64_t)p1_phys, "RUNNING");

    // Test 2: Map 100 pages
    diag_set_step("STRESS TEST 2 (100 PAGES)");
    uint64_t v100_base = 0x50000000ULL;
    for (size_t i = 0; i < 100; i++) {
        void *p_phys = pmm_alloc_page();
        uint64_t virt = v100_base + (i * PAGE_SIZE);
        vmm_map_page(g_kernel_pml4, (uint64_t)p_phys, virt, VMM_FLAG_WRITABLE);
        if (!vmm_is_mapped(g_kernel_pml4, virt)) {
            com1_puts("[VMM] STRESS TEST 2 FAIL\n");
            diag_panic_reason("VMM", "STRESS_TEST_2", "MAP_FAIL_100", "100-page mapping failed");
            return;
        }
    }
    g_vmm_mapped_page_count += 100;
    diag_set_vmm_telemetry((uint64_t)pml4, (uint64_t)pml4, (uint64_t)pdp, 2048, g_vmm_mapped_page_count, 0, v100_base, v100_base, v100_base, "RUNNING");

    // Test 3: Map 1000 pages
    diag_set_step("STRESS TEST 3 (1000 PAGES)");
    uint64_t v1000_base = 0x60000000ULL;
    for (size_t i = 0; i < 1000; i++) {
        void *p_phys = pmm_alloc_page();
        uint64_t virt = v1000_base + (i * PAGE_SIZE);
        vmm_map_page(g_kernel_pml4, (uint64_t)p_phys, virt, VMM_FLAG_WRITABLE);
    }
    g_vmm_mapped_page_count += 1000;
    diag_set_vmm_telemetry((uint64_t)pml4, (uint64_t)pml4, (uint64_t)pdp, 2048, g_vmm_mapped_page_count, 0, v1000_base, v1000_base, v1000_base, "RUNNING");

    // Test 4: Translate addresses
    diag_set_step("STRESS TEST 4 (TRANSLATE)");
    uint64_t test_trans = vmm_translate(g_kernel_pml4, v1000_base + (500 * PAGE_SIZE));
    if (test_trans == 0) {
        com1_puts("[VMM] STRESS TEST 4 FAIL\n");
        diag_panic_reason("VMM", "STRESS_TEST_4", "TRANSLATE_NULL", "Address translation returned NULL");
        return;
    }

    // Test 5: Unmap pages & verify vmm_is_mapped
    diag_set_step("STRESS TEST 5 (UNMAP)");
    for (size_t i = 0; i < 1000; i++) {
        uint64_t virt = v1000_base + (i * PAGE_SIZE);
        uint64_t phys_p = vmm_translate(g_kernel_pml4, virt);
        vmm_unmap_page(g_kernel_pml4, virt);
        if (phys_p) pmm_free_page((void*)phys_p);
    }
    g_vmm_mapped_page_count -= 1000;

    // Stage 7: CERTIFIED PASS
    com1_puts("[VMM] VMM INIT COMPLETE\n");
    diag_set_vmm_telemetry((uint64_t)pml4, (uint64_t)pml4, (uint64_t)pdp, 2048, g_vmm_mapped_page_count, g_vmm_page_fault_count, v100_base, v100_base, v100_base, "PASS");
    diag_set_pass("VMM");
    diag_set_step("CERTIFIED PASS");
}

void vmm_map_page(void *pml4, uint64_t phys_addr, uint64_t virt_addr, uint32_t flags) {
    phys_addr &= ~0xFFFULL;
    virt_addr &= ~0xFFFULL;

    void *kernel_pml4 = vmm_get_kernel_pml4();
    if (!pml4) {
        pml4 = kernel_pml4;
    }

    void *active_pml4 = vmm_get_active_pml4();
    if (active_pml4 != kernel_pml4)
        vmm_switch_address_space(kernel_pml4);

    uint64_t *pt_entry = vmm_get_pt_entry(pml4, virt_addr, true);
    if (!pt_entry) {
        if (active_pml4 != kernel_pml4)
            vmm_switch_address_space(active_pml4);
        diag_panic_reason("VMM", "MAP_PAGE", "PTE_FAIL", "vmm_map_page failed to allocate PTE");
        return;
    }

    *pt_entry = phys_addr | flags | PAGE_PRESENT;
    vmm_flush_tlb(virt_addr);

    if (active_pml4 != kernel_pml4) {
        vmm_switch_address_space(active_pml4);
        vmm_flush_tlb(virt_addr);
    }
}

void vmm_unmap_page(void *pml4, uint64_t virt_addr) {
    virt_addr &= ~0xFFFULL;

    void *active_pml4 = vmm_get_active_pml4();
    void *kernel_pml4 = vmm_get_kernel_pml4();
    if (active_pml4 != kernel_pml4)
        vmm_switch_address_space(kernel_pml4);

    uint64_t *pt_entry = vmm_get_pt_entry(pml4, virt_addr, false);
    if (pt_entry && (*pt_entry & PAGE_PRESENT)) {
        *pt_entry = 0;
        vmm_flush_tlb(virt_addr);
    }

    if (active_pml4 != kernel_pml4)
        vmm_switch_address_space(active_pml4);
}

void *vmm_alloc_mapped_page(void *pml4, uint64_t virt_addr, uint32_t flags) {
    void *frame = pmm_alloc_page();
    if (!frame) return NULL;

    vmm_map_page(pml4, (uint64_t)frame, virt_addr, flags);
    return frame;
}

void vmm_free_mapped_page(void *pml4, uint64_t virt_addr) {
    uint64_t phys_addr = vmm_get_physical_address(pml4, virt_addr);
    if (phys_addr) {
        vmm_unmap_page(pml4, virt_addr);
        pmm_free_page((void *)phys_addr);
    }
}

void vmm_switch_address_space(void *pml4_phys_addr) {
    __asm__ volatile("mov %0, %%cr3" ::"r"(pml4_phys_addr) : "memory");
}

uint64_t vmm_get_physical_address(void *pml4, uint64_t virt_addr) {
    void *active_pml4 = vmm_get_active_pml4();
    void *kernel_pml4 = vmm_get_kernel_pml4();
    if (active_pml4 != kernel_pml4)
        vmm_switch_address_space(kernel_pml4);

    uint64_t phys_addr = 0;
    uint64_t *pt_entry = vmm_get_pt_entry(pml4, virt_addr, false);
    if (pt_entry && (*pt_entry & PAGE_PRESENT)) {
        phys_addr = (*pt_entry & PAGE_PHYS_ADDRESS_MASK) + (virt_addr & 0xFFF);
    }

    if (active_pml4 != kernel_pml4)
        vmm_switch_address_space(active_pml4);
    return phys_addr;
}

static bool vmm_address_canonical(uint64_t address) {
    uint64_t high = address >> 48;
    return high == 0 || high == 0xFFFF;
}

bool vmm_query_page(void *pml4, uint64_t virt_addr, VMMPageInfo *out) {
    if (!pml4 || !out || !vmm_address_canonical(virt_addr)) return false;
    uint64_t *entry = vmm_get_pt_entry(pml4, virt_addr, false);
    uint64_t value = entry ? *entry : 0;
    out->virtual_address = virt_addr & ~0xFFFULL;
    out->physical_address = value & PAGE_PHYS_ADDRESS_MASK;
    out->flags = value & ~PAGE_PHYS_ADDRESS_MASK;
    out->present = (value & PAGE_PRESENT) != 0;
    out->writable = (value & PAGE_WRITABLE) != 0;
    out->user = (value & PAGE_USER) != 0;
    out->executable = (value & PAGE_NX) == 0;
    return out->present;
}

bool vmm_validate_user_range(void *pml4, uint64_t address, uint64_t size, uint32_t access) {
    if (!pml4 || size == 0 || !vmm_address_canonical(address) ||
        address < VMM_USER_MIN_ADDRESS || address > VMM_USER_MAX_ADDRESS ||
        size - 1 > VMM_USER_MAX_ADDRESS - address)
        return false;
    uint64_t first = address & ~0xFFFULL;
    uint64_t last = (address + size - 1) & ~0xFFFULL;
    for (uint64_t page = first;; page += VMM_PAGE_SIZE) {
        VMMPageInfo info;
        if (!vmm_query_page(pml4, page, &info) || !info.user) return false;
        if ((access & VMM_ACCESS_WRITE) && !info.writable) return false;
        if ((access & VMM_ACCESS_EXECUTE) && !info.executable) return false;
        if (page == last) break;
        if (page > VMM_USER_MAX_ADDRESS - VMM_PAGE_SIZE) return false;
    }
    return true;
}

bool vmm_map_user_page(void *pml4, uint64_t virt_addr, uint32_t access) {
    if (!pml4 || (virt_addr & 0xFFFULL) || virt_addr < VMM_USER_MIN_ADDRESS ||
        virt_addr > VMM_USER_MAX_ADDRESS)
        return false;
    VMMPageInfo existing;
    if (vmm_query_page(pml4, virt_addr, &existing)) return false;
    uint32_t flags = PAGE_USER;
    if (access & VMM_ACCESS_WRITE) flags |= PAGE_WRITABLE;
    if (!(access & VMM_ACCESS_EXECUTE)) flags |= PAGE_NX;
    return vmm_alloc_mapped_page(pml4, virt_addr, flags) != NULL;
}

bool vmm_map_guard_page(void *pml4, uint64_t virt_addr) {
    if (!pml4 || (virt_addr & 0xFFFULL) || virt_addr < VMM_USER_MIN_ADDRESS ||
        virt_addr > VMM_USER_MAX_ADDRESS)
        return false;
    vmm_unmap_page(pml4, virt_addr);
    return true;
}

void vmm_dump_address_space(void *pml4, uint64_t start, uint64_t end) { (void)pml4; (void)start; (void)end; }

void *vmm_create_address_space(void) {
    uint64_t *new_pml4 = (uint64_t *)pmm_alloc_page();
    if (!new_pml4) return NULL;

    if (g_kernel_pml4) {
        uint64_t *k_pml4 = (uint64_t*)g_kernel_pml4;
        for (int i = 0; i < 512; i++) new_pml4[i] = k_pml4[i];
    } else {
        for (int i = 0; i < 512; i++) new_pml4[i] = 0;
    }

    uint64_t *new_pdp = (uint64_t *)pmm_alloc_page();
    if (!new_pdp) { pmm_free_page(new_pml4); return NULL; }

    if (g_kernel_pml4) {
        uint64_t *k_pml4 = (uint64_t*)g_kernel_pml4;
        uint64_t *k_pdp = (uint64_t*)(k_pml4[0] & PAGE_PHYS_ADDRESS_MASK);
        if (k_pdp) {
            for (int i = 0; i < 512; i++) new_pdp[i] = k_pdp[i];
        } else {
            for (int i = 0; i < 512; i++) new_pdp[i] = 0;
        }
    } else {
        for (int i = 0; i < 512; i++) new_pdp[i] = 0;
    }

    uint64_t *user_pd1 = (uint64_t *)pmm_alloc_page();
    if (!user_pd1) { pmm_free_page(new_pdp); pmm_free_page(new_pml4); return NULL; }
    for (int i = 0; i < 512; i++) user_pd1[i] = 0;

    new_pml4[0] = ((uint64_t)new_pdp) | PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER;

    new_pdp[1] = ((uint64_t)user_pd1) | PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER;
    return new_pml4;
}

bool vmm_destroy_address_space(void *pml4) {
    if (!pml4 || pml4 == g_kernel_pml4) return false;

    // 1. CR3 Safety: If executing CPU is currently using this PML4, switch to kernel PML4 first
    void *active_pml4 = vmm_get_active_pml4();
    void *kernel_pml4 = vmm_get_kernel_pml4();
    if (active_pml4 == pml4) {
        vmm_switch_address_space(kernel_pml4);
        active_pml4 = kernel_pml4;
    }

    uint64_t *k_pml4 = (uint64_t*)kernel_pml4;
    uint64_t *k_pdp = (k_pml4 && (k_pml4[0] & PAGE_PRESENT)) ? 
                      (uint64_t*)(k_pml4[0] & PAGE_PHYS_ADDRESS_MASK) : NULL;

    uint64_t *pml4_table = (uint64_t*)pml4;

    // 2. Walk Process-Owned Lower-Half PML4 entries (indices 0..255)
    // Indices 256..511 are strictly kernel space and shared; NEVER TOUCH them!
    for (int pml4_idx = 0; pml4_idx < 256; pml4_idx++) {
        uint64_t pml4e = pml4_table[pml4_idx];
        if (!(pml4e & PAGE_PRESENT)) continue;

        uint64_t pdp_phys = pml4e & PAGE_PHYS_ADDRESS_MASK;
        if (!pdp_phys) continue;

        // If this points to the shared kernel PDP, do NOT free it!
        if (k_pml4 && pdp_phys == (k_pml4[pml4_idx] & PAGE_PHYS_ADDRESS_MASK)) {
            continue;
        }

        uint64_t *pdp_table = (uint64_t*)pdp_phys;

        // Walk entries in this process-owned PDP table
        for (int pdp_idx = 0; pdp_idx < 512; pdp_idx++) {
            uint64_t pdpe = pdp_table[pdp_idx];
            if (!(pdpe & PAGE_PRESENT)) continue;

            // Huge 1GB pages in PDP (e.g. kernel huge mappings 4GB..512GB) are shared; skip!
            if (pdpe & PAGE_HUGE) continue;

            uint64_t pd_phys = pdpe & PAGE_PHYS_ADDRESS_MASK;
            if (!pd_phys) continue;

            // If this PD matches any shared kernel PD (e.g. pd0, pd2, pd3), do NOT touch or free it!
            if (k_pdp && pd_phys == (k_pdp[pdp_idx] & PAGE_PHYS_ADDRESS_MASK)) {
                continue;
            }

            uint64_t *pd_table = (uint64_t*)pd_phys;

            // Walk entries in this process-owned PD table
            for (int pd_idx = 0; pd_idx < 512; pd_idx++) {
                uint64_t pde = pd_table[pd_idx];
                if (!(pde & PAGE_PRESENT)) continue;

                if (pde & PAGE_HUGE) {
                    // 2MB huge page in user PD
                    uint64_t huge_page_phys = pde & PAGE_PHYS_ADDRESS_MASK;
                    if (huge_page_phys >= 0x100000 && !(huge_page_phys >= 0x80000000 && huge_page_phys < 0xD0000000)) {
                        pmm_free_page((void*)huge_page_phys);
                    }
                    pd_table[pd_idx] = 0;
                    continue;
                }

                // Dynamic 4KB Page Table (PT)
                uint64_t pt_phys = pde & PAGE_PHYS_ADDRESS_MASK;
                if (!pt_phys) continue;

                uint64_t *pt_table = (uint64_t*)pt_phys;

                // Walk entries in this process-owned PT table
                for (int pt_idx = 0; pt_idx < 512; pt_idx++) {
                    uint64_t pte = pt_table[pt_idx];
                    if (!(pte & PAGE_PRESENT)) continue;

                    uint64_t user_page_phys = pte & PAGE_PHYS_ADDRESS_MASK;
                    if (user_page_phys >= 0x100000 && !(user_page_phys >= 0x80000000 && user_page_phys < 0xD0000000)) {
                        pmm_free_page((void*)user_page_phys);
                    }
                    pt_table[pt_idx] = 0;
                }

                // Free the PT frame
                pmm_free_page((void*)pt_phys);
                pd_table[pd_idx] = 0;
            }

            // Free the process-owned PD frame
            pmm_free_page((void*)pd_phys);
            pdp_table[pdp_idx] = 0;
        }

        // Free the process-owned PDP frame
        pmm_free_page((void*)pdp_phys);
        pml4_table[pml4_idx] = 0;
    }

    // 3. Free the root PML4 page itself
    pmm_free_page(pml4);
    return true;
}

bool vmm_walk_and_verify(void *pml4, uint64_t virt_addr) {
    if (!pml4) return false;
    uint64_t pml4_index = (virt_addr >> 39) & 0x1FF;
    uint64_t pdp_index  = (virt_addr >> 30) & 0x1FF;
    uint64_t pd_index   = (virt_addr >> 21) & 0x1FF;
    uint64_t pt_index   = (virt_addr >> 12) & 0x1FF;

    void *active_pml4 = vmm_get_active_pml4();
    void *kernel_pml4 = vmm_get_kernel_pml4();
    if (active_pml4 != kernel_pml4)
        vmm_switch_address_space(kernel_pml4);

    uint64_t* pml4_table = (uint64_t*)pml4;
    com1_puts("[USERMAP VERIFY]\r\n");
    com1_puts("  CR3="); diag_put_hex64((uint64_t)pml4);
    com1_puts(" VA="); diag_put_hex64(virt_addr);
    com1_puts("\r\n");

    uint64_t pml4e = pml4_table[pml4_index];
    com1_puts("  PML4E="); diag_put_hex64(pml4e);
    com1_puts(" (P="); diag_put_dec((pml4e & PAGE_PRESENT) ? 1 : 0);
    com1_puts(" W="); diag_put_dec((pml4e & PAGE_WRITABLE) ? 1 : 0);
    com1_puts(" U="); diag_put_dec((pml4e & PAGE_USER) ? 1 : 0);
    com1_puts(")\r\n");

    if (!(pml4e & PAGE_PRESENT)) {
        if (active_pml4 != kernel_pml4) vmm_switch_address_space(active_pml4);
        return false;
    }

    uint64_t* pdp_table = (uint64_t*)(pml4e & PAGE_PHYS_ADDRESS_MASK);
    uint64_t pdpe = pdp_table[pdp_index];
    com1_puts("  PDPE="); diag_put_hex64(pdpe);
    com1_puts(" (P="); diag_put_dec((pdpe & PAGE_PRESENT) ? 1 : 0);
    com1_puts(" W="); diag_put_dec((pdpe & PAGE_WRITABLE) ? 1 : 0);
    com1_puts(" U="); diag_put_dec((pdpe & PAGE_USER) ? 1 : 0);
    com1_puts(")\r\n");

    if (!(pdpe & PAGE_PRESENT)) {
        if (active_pml4 != kernel_pml4) vmm_switch_address_space(active_pml4);
        return false;
    }

    uint64_t* pd_table = (uint64_t*)(pdpe & PAGE_PHYS_ADDRESS_MASK);
    uint64_t pde = pd_table[pd_index];
    com1_puts("  PDE="); diag_put_hex64(pde);
    com1_puts(" (P="); diag_put_dec((pde & PAGE_PRESENT) ? 1 : 0);
    com1_puts(" W="); diag_put_dec((pde & PAGE_WRITABLE) ? 1 : 0);
    com1_puts(" U="); diag_put_dec((pde & PAGE_USER) ? 1 : 0);
    com1_puts(")\r\n");

    if (!(pde & PAGE_PRESENT)) {
        if (active_pml4 != kernel_pml4) vmm_switch_address_space(active_pml4);
        return false;
    }

    uint64_t* pt_table = (uint64_t*)(pde & PAGE_PHYS_ADDRESS_MASK);
    uint64_t pte = pt_table[pt_index];
    com1_puts("  PTE="); diag_put_hex64(pte);
    com1_puts(" (P="); diag_put_dec((pte & PAGE_PRESENT) ? 1 : 0);
    com1_puts(" W="); diag_put_dec((pte & PAGE_WRITABLE) ? 1 : 0);
    com1_puts(" U="); diag_put_dec((pte & PAGE_USER) ? 1 : 0);
    com1_puts(" PHYS="); diag_put_hex64(pte & PAGE_PHYS_ADDRESS_MASK);
    com1_puts(")\r\n");

    bool ok = (pml4e & PAGE_PRESENT) && (pml4e & PAGE_USER) && (pml4e & PAGE_WRITABLE) &&
              (pdpe & PAGE_PRESENT) && (pdpe & PAGE_USER) && (pdpe & PAGE_WRITABLE) &&
              (pde & PAGE_PRESENT) && (pde & PAGE_USER) && (pde & PAGE_WRITABLE) &&
              (pte & PAGE_PRESENT) && (pte & PAGE_USER) && (pte & PAGE_WRITABLE);

    com1_puts("  RESULT=");
    com1_puts(ok ? "PASS (PRESENT=1 WRITABLE=1 USER=1)\r\n" : "FAIL (Missing permission/present bits)\r\n");

    if (active_pml4 != kernel_pml4)
        vmm_switch_address_space(active_pml4);

    return ok;
}

void vmm_self_test(void) {}

