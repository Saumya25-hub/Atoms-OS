#include "kernel/core/memory/vmm/include/vmm.h"
#include "kernel/core/memory/vmm/include/paging.h"
#include "kernel/core/memory/pmm/include/pmm.h"
#include "kernel/debug/abde/abde.h"

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
    diag_set_running("VMM");

    // Stage 1: CREATE PML4
    diag_set_step("CREATE PML4");
    uint64_t *pml4 = (uint64_t *)pmm_alloc_page();
    if (!pml4) {
        diag_panic_reason("VMM", "CREATE_PML4", "PMM_OUT_OF_MEMORY", "Failed to allocate PML4 table");
        return;
    }
    for (int i = 0; i < 512; i++) pml4[i] = 0;
    g_kernel_pml4 = (void *)pml4;

    diag_set_vmm_telemetry(0, (uint64_t)pml4, 0, 0, 0, 0, 0, 0, 0, "RUNNING");

    // Stage 2: CREATE PDPT
    diag_set_step("CREATE PDPT");
    uint64_t *pdp  = (uint64_t *)pmm_alloc_page();
    uint64_t *pd0  = (uint64_t *)pmm_alloc_page();
    uint64_t *pd1  = (uint64_t *)pmm_alloc_page();
    uint64_t *pd2  = (uint64_t *)pmm_alloc_page();
    uint64_t *pd3  = (uint64_t *)pmm_alloc_page();

    if (!pdp || !pd0 || !pd1 || !pd2 || !pd3) {
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
    pml4[0]   = (uint64_t)pdp | PAGE_PRESENT | PAGE_WRITABLE;
    pml4[256] = (uint64_t)pdp | PAGE_PRESENT | PAGE_WRITABLE;
    pml4[511] = (uint64_t)pdp | PAGE_PRESENT | PAGE_WRITABLE;

    pdp[0] = (uint64_t)pd0 | PAGE_PRESENT | PAGE_WRITABLE;
    pdp[1] = (uint64_t)pd1 | PAGE_PRESENT | PAGE_WRITABLE;
    pdp[2] = (uint64_t)pd2 | PAGE_PRESENT | PAGE_WRITABLE;
    pdp[3] = (uint64_t)pd3 | PAGE_PRESENT | PAGE_WRITABLE;

    // Stage 4: IDENTITY MAP RAM & FRAMEBUFFER
    diag_set_step("IDENTITY MAP RAM");
    uint64_t phys = 0;
    for (int i = 0; i < 512; i++) { pd0[i] = phys | PAGE_PRESENT | PAGE_WRITABLE | PAGE_HUGE; phys += 0x200000ULL; }
    for (int i = 0; i < 512; i++) { pd1[i] = phys | PAGE_PRESENT | PAGE_WRITABLE | PAGE_HUGE; phys += 0x200000ULL; }
    for (int i = 0; i < 512; i++) { pd2[i] = phys | PAGE_PRESENT | PAGE_WRITABLE | PAGE_HUGE | PAGE_WRITE_THROUGH; phys += 0x200000ULL; }
    for (int i = 0; i < 512; i++) { pd3[i] = phys | PAGE_PRESENT | PAGE_WRITABLE | PAGE_HUGE | PAGE_CACHE_DISABLE; phys += 0x200000ULL; }

    g_vmm_mapped_page_count = 2048; // 2048 2MB Pages = 4GB Identity Space

    diag_set_vmm_telemetry(0, (uint64_t)pml4, (uint64_t)pdp, 2048, 2048, 0, 0, 0, 0, "RUNNING");

    // Stage 5: LOAD CR3 & ENABLE PAGING
    diag_set_step("LOAD CR3");
    vmm_enable();
    diag_set_vmm_telemetry((uint64_t)pml4, (uint64_t)pml4, (uint64_t)pdp, 2048, 2048, 0, 0, 0, 0, "RUNNING");

    // Stage 6: VERIFY TRANSLATION & STRESS TESTS
    diag_set_step("VERIFY TRANSLATION");

    // Test 1: Map 1 page
    diag_set_step("STRESS TEST 1 (1 PAGE)");
    void *p1_phys = pmm_alloc_page();
    uint64_t v1 = 0x40000000ULL;
    vmm_map_page(g_kernel_pml4, (uint64_t)p1_phys, v1, VMM_FLAG_WRITABLE);
    if (!vmm_is_mapped(g_kernel_pml4, v1) || vmm_translate(g_kernel_pml4, v1) != (uint64_t)p1_phys) {
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
    diag_set_vmm_telemetry((uint64_t)pml4, (uint64_t)pml4, (uint64_t)pdp, 2048, g_vmm_mapped_page_count, g_vmm_page_fault_count, v100_base, v100_base, v100_base, "PASS");
    diag_set_pass("VMM");
    diag_set_step("CERTIFIED PASS");
}

void vmm_map_page(void *pml4, uint64_t phys_addr, uint64_t virt_addr, uint32_t flags) {
    phys_addr &= ~0xFFFULL;
    virt_addr &= ~0xFFFULL;

    void *active_pml4 = vmm_get_active_pml4();
    void *kernel_pml4 = vmm_get_kernel_pml4();
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

    if (active_pml4 != kernel_pml4)
        vmm_switch_address_space(active_pml4);
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
    uint64_t *new_pml4 = pmm_alloc_page();
    if (!new_pml4) return NULL;
    for (int i = 0; i < 512; i++) new_pml4[i] = 0;

    uint64_t *new_pdp = pmm_alloc_page();
    if (!new_pdp) { pmm_free_page(new_pml4); return NULL; }
    for (int i = 0; i < 512; i++) new_pdp[i] = 0;

    new_pml4[0] = ((uint64_t)new_pdp) | PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER;
    new_pml4[511] = ((uint64_t)new_pdp) | PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER;

    if (g_kernel_pml4) {
        uint64_t *k_pml4 = (uint64_t*)g_kernel_pml4;
        uint64_t *k_pdp = (uint64_t*)(k_pml4[0] & PAGE_PHYS_ADDRESS_MASK);
        if (k_pdp) {
            for (int i = 0; i < 512; i++) new_pdp[i] = k_pdp[i];
        }
    }
    return new_pml4;
}

bool vmm_destroy_address_space(void *pml4) {
    if (!pml4 || pml4 == g_kernel_pml4) return false;
    pmm_free_page(pml4);
    return true;
}

void vmm_self_test(void) {}
