#include "abde.h"

/* Global ABDE Engine Instance */
abde_engine_t g_abde = {0};

/* Internal String Copy Helper (Zero LibC Dependency) */
static void abde_strcpy(char *dest, const char *src, uint32_t max_len) {
    if (!dest || !src || max_len == 0) return;
    uint32_t i = 0;
    while (src[i] != '\0' && i < max_len - 1) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

/* Internal String Compare Helper */
static bool abde_streq(const char *s1, const char *s2) {
    if (!s1 || !s2) return false;
    while (*s1 && *s2) {
        if (*s1 != *s2) return false;
        s1++;
        s2++;
    }
    return *s1 == *s2;
}

/* Add Single Subsystem Module Definition */
static void abde_add_module(const char *name, diag_status_t initial_status) {
    if (g_abde.module_count >= ABDE_MAX_MODULES) return;
    diag_module_t *mod = &g_abde.modules[g_abde.module_count++];
    abde_strcpy(mod->name, name, ABDE_MAX_NAME_LEN);
    mod->status = initial_status;
}

/* Initialize ABDE Diagnostic Engine V2.5 Dashboard */
void diag_init(boot_info_t *boot_info) {
    uint8_t *abde_ptr = (uint8_t*)&g_abde;
    for (uint32_t i = 0; i < (uint32_t)sizeof(abde_engine_t); i++) {
        abde_ptr[i] = 0;
    }

    g_abde.boot_info = boot_info;
    g_abde.framebuffer = 0;
    g_abde.pitch = 2560 * 4;
    g_abde.width = 2560;
    g_abde.height = 1600;

    if (boot_info) {
        g_abde.framebuffer = boot_info->vbe_framebuffer;
        if (boot_info->vbe_pitch > 0) g_abde.pitch = boot_info->vbe_pitch;
        if (boot_info->vbe_width > 0) g_abde.width = boot_info->vbe_width;
        if (boot_info->vbe_height > 0) g_abde.height = boot_info->vbe_height;
    }

    if (!g_abde.framebuffer && boot_info) {
        g_abde.framebuffer = *(uint64_t*)((uint8_t*)boot_info + 24);
    }

    g_abde.module_count = 0;
    g_abde.overall_status = DIAG_STATUS_RUNNING;
    g_abde.heartbeat_ticks = 0;
    g_abde.smp_bsp_id = 0;
    g_abde.smp_cpu_found = 1;
    g_abde.smp_cpu_online = 1;
    g_abde.smp_current_cpu = 0;
    g_abde.smp_init_ipis = 0;
    g_abde.smp_sipis_sent = 0;
    g_abde.smp_ap_responses = 0;
    g_abde.smp_last_ap = 0;

    g_abde.idt_entries = 0;
    g_abde.idt_base = 0;
    g_abde.isr_installed = 0;
    g_abde.exceptions_armed = false;
    abde_strcpy(g_abde.last_interrupt, "NONE", ABDE_MAX_STEP_LEN);
    abde_strcpy(g_abde.last_exception, "NONE", ABDE_MAX_STEP_LEN);
    g_abde.fault_count = 0;

    g_abde.pic_remapped = false;
    g_abde.apic_enabled = false;
    g_abde.ioapic_base = 0xFEC00000;
    g_abde.timer_irq0_ticks = 0;
    g_abde.kbd_irq1_count = 0;
    g_abde.last_irq = 0;
    g_abde.last_vector = 0;

    g_abde.pmm_active = false;
    g_abde.pmm_total_ram_mb = 0;
    g_abde.pmm_usable_ram_mb = 0;
    g_abde.pmm_reserved_ram_mb = 0;
    g_abde.pmm_free_pages = 0;
    g_abde.pmm_used_pages = 0;
    g_abde.pmm_reserved_pages = 0;
    g_abde.pmm_last_alloc = 0;
    g_abde.pmm_last_free = 0;

    g_abde.heap_active = false;
    g_abde.heap_base = 0;
    g_abde.heap_size_kb = 0;
    g_abde.heap_used_kb = 0;
    g_abde.heap_free_kb = 0;
    g_abde.heap_alloc_count = 0;
    g_abde.heap_free_count = 0;
    g_abde.heap_leak_count = 0;
    g_abde.heap_corruption_count = 0;
    g_abde.heap_largest_free_kb = 0;
    g_abde.heap_last_alloc = 0;
    g_abde.heap_last_caller_rip = 0;
    abde_strcpy(g_abde.heap_status_str, "WAIT", 16);

    g_abde.heap_active = false;
    g_abde.heap_base = 0;
    g_abde.heap_size_kb = 0;
    g_abde.heap_used_kb = 0;
    g_abde.heap_free_kb = 0;
    g_abde.heap_alloc_count = 0;
    g_abde.heap_free_count = 0;
    g_abde.heap_leak_count = 0;
    g_abde.heap_corruption_count = 0;
    g_abde.heap_largest_free_kb = 0;
    g_abde.heap_last_alloc = 0;
    g_abde.heap_last_caller_rip = 0;
    abde_strcpy(g_abde.heap_status_str, "WAIT", 16);

    g_abde.vmm_active = false;
    g_abde.vmm_cr3 = 0;
    g_abde.vmm_pml4 = 0;
    g_abde.vmm_pdpt = 0;
    g_abde.vmm_identity_pages = 0;
    g_abde.vmm_mapped_pages = 0;
    g_abde.vmm_page_faults = 0;
    g_abde.vmm_last_mapping = 0;
    g_abde.vmm_last_virt = 0;
    g_abde.vmm_last_phys = 0;
    abde_strcpy(g_abde.vmm_status_str, "WAIT", 16);

    for (int i = 0; i < ABDE_MAX_CPUS; i++) {
        g_abde.cpus[i].online = (i == 0);
        g_abde.cpus[i].starting = false;
        g_abde.cpus[i].failed = false;
        g_abde.cpus[i].heartbeat = 0;
    }

    abde_strcpy(g_abde.current_module, "Initialization", ABDE_MAX_NAME_LEN);
    abde_strcpy(g_abde.current_step, "Starting ABDE V2.5 Dashboard", ABDE_MAX_STEP_LEN);
    abde_strcpy(g_abde.last_event, "BOOT_START", ABDE_MAX_STEP_LEN);
    abde_strcpy(g_abde.error_code, "NONE", ABDE_MAX_ERR_LEN);
    abde_strcpy(g_abde.fault_detail, "NONE", ABDE_MAX_DETAIL_LEN);

    // Register Subsystem Certification Board Modules — ALL START AT WAIT
    abde_add_module("CPU",  DIAG_STATUS_WAIT);
    abde_add_module("GDT",  DIAG_STATUS_WAIT);
    abde_add_module("SMP",  DIAG_STATUS_WAIT);
    abde_add_module("IDT",  DIAG_STATUS_WAIT);
    abde_add_module("PIC",  DIAG_STATUS_WAIT);
    abde_add_module("PMM",  DIAG_STATUS_WAIT);
    abde_add_module("VMM",  DIAG_STATUS_WAIT);
    abde_add_module("HEAP", DIAG_STATUS_WAIT);

    diag_render();
}

/* Find Module by Name */
static diag_module_t* abde_find_module(const char *name) {
    for (uint32_t i = 0; i < g_abde.module_count; i++) {
        if (abde_streq(g_abde.modules[i].name, name)) {
            return &g_abde.modules[i];
        }
    }
    return 0;
}

/* Set Module Status to RUNNING */
void diag_set_running(const char *module_name) {
    diag_module_t *mod = abde_find_module(module_name);
    if (mod) {
        mod->status = DIAG_STATUS_RUNNING;
    } else {
        abde_add_module(module_name, DIAG_STATUS_RUNNING);
    }
    abde_strcpy(g_abde.current_module, module_name, ABDE_MAX_NAME_LEN);
    diag_render();
}

/* Set Module Status to PASS */
void diag_set_pass(const char *module_name) {
    diag_module_t *mod = abde_find_module(module_name);
    if (mod) {
        mod->status = DIAG_STATUS_PASS;
    }
    bool has_fail = false;
    for (uint32_t i = 0; i < g_abde.module_count; i++) {
        if (g_abde.modules[i].status == DIAG_STATUS_FAIL) {
            has_fail = true;
            break;
        }
    }
    if (!has_fail) {
        g_abde.overall_status = DIAG_STATUS_PASS;
        abde_strcpy(g_abde.error_code, "NONE", ABDE_MAX_ERR_LEN);
        abde_strcpy(g_abde.fault_detail, "NONE", ABDE_MAX_DETAIL_LEN);
    }
    diag_render();
}

/* Set Module Status to FAIL */
void diag_set_fail(const char *module_name) {
    diag_module_t *mod = abde_find_module(module_name);
    if (mod) {
        mod->status = DIAG_STATUS_FAIL;
    }
    g_abde.overall_status = DIAG_STATUS_FAIL;
    abde_strcpy(g_abde.current_module, module_name, ABDE_MAX_NAME_LEN);
    diag_render();
}

/* Set Current Step Name */
void diag_set_step(const char *step_name) {
    abde_strcpy(g_abde.last_event, g_abde.current_step, ABDE_MAX_STEP_LEN);
    abde_strcpy(g_abde.current_step, step_name, ABDE_MAX_STEP_LEN);
    diag_render();
}

/* Set Error Code */
void diag_set_error(const char *error_code) {
    abde_strcpy(g_abde.error_code, error_code, ABDE_MAX_ERR_LEN);
    diag_render();
}

/* Set Fault & Detail */
void diag_set_fault(const char *error_code, const char *detail) {
    g_abde.overall_status = DIAG_STATUS_FAIL;
    abde_strcpy(g_abde.error_code, error_code, ABDE_MAX_ERR_LEN);
    abde_strcpy(g_abde.fault_detail, detail, ABDE_MAX_DETAIL_LEN);
    diag_render();
}

/* Set SMP Forensic Telemetry */
void diag_set_smp_telemetry(uint32_t found, uint32_t online, uint32_t current_cpu, uint32_t init_ipis, uint32_t sipis, uint32_t responses) {
    g_abde.smp_cpu_found = found;
    g_abde.smp_cpu_online = online;
    g_abde.smp_current_cpu = current_cpu;
    g_abde.smp_init_ipis = init_ipis;
    g_abde.smp_sipis_sent = sipis;
    g_abde.smp_ap_responses = responses;
    if (current_cpu < ABDE_MAX_CPUS) {
        g_abde.cpus[current_cpu].online = (online > current_cpu);
        if (online > current_cpu) g_abde.smp_last_ap = current_cpu;
    }
    diag_render();
}

/* Set IDT Engine Telemetry */
void diag_set_idt_telemetry(uint32_t entries, uint64_t base, uint32_t isr_count, bool armed, const char *last_exc, uint32_t faults) {
    g_abde.idt_entries = entries;
    g_abde.idt_base = base;
    g_abde.isr_installed = isr_count;
    g_abde.exceptions_armed = armed;
    if (last_exc) {
        abde_strcpy(g_abde.last_exception, last_exc, ABDE_MAX_STEP_LEN);
    }
    g_abde.fault_count = faults;
    diag_render();
}

/* Set PIC / APIC Live Telemetry */
void diag_set_pic_telemetry(bool pic_remap, bool apic_en, uint64_t ioapic, uint64_t timer_ticks, uint64_t kbd_count, uint32_t last_irq, uint32_t last_vec) {
    g_abde.pic_remapped = pic_remap;
    g_abde.apic_enabled = apic_en;
    g_abde.ioapic_base = ioapic;
    g_abde.timer_irq0_ticks = timer_ticks;
    g_abde.kbd_irq1_count = kbd_count;
    g_abde.last_irq = last_irq;
    g_abde.last_vector = last_vec;
    diag_render();
}

/* Set PMM Physical Memory Manager Telemetry */
void diag_set_pmm_telemetry(uint64_t total_mb, uint64_t usable_mb, uint64_t reserved_mb, uint64_t free_p, uint64_t used_p, uint64_t res_p, uint64_t last_alloc, uint64_t last_free) {
    g_abde.pmm_active = true;
    g_abde.pmm_total_ram_mb = total_mb;
    g_abde.pmm_usable_ram_mb = usable_mb;
    g_abde.pmm_reserved_ram_mb = reserved_mb;
    g_abde.pmm_free_pages = free_p;
    g_abde.pmm_used_pages = used_p;
    g_abde.pmm_reserved_pages = res_p;
    g_abde.pmm_last_alloc = last_alloc;
    g_abde.pmm_last_free = last_free;
    diag_render();
}

/* Set Dedicated VMM Live Telemetry */
void diag_set_vmm_telemetry(uint64_t cr3, uint64_t pml4, uint64_t pdpt, uint64_t identity_p, uint64_t mapped_p, uint64_t faults, uint64_t last_map, uint64_t last_virt, uint64_t last_phys, const char *status_str) {
    g_abde.vmm_active = true;
    g_abde.vmm_cr3 = cr3;
    g_abde.vmm_pml4 = pml4;
    g_abde.vmm_pdpt = pdpt;
    g_abde.vmm_identity_pages = identity_p;
    g_abde.vmm_mapped_pages = mapped_p;
    g_abde.vmm_page_faults = faults;
    g_abde.vmm_last_mapping = last_map;
    g_abde.vmm_last_virt = last_virt;
    g_abde.vmm_last_phys = last_phys;
    if (status_str) {
        abde_strcpy(g_abde.vmm_status_str, status_str, 16);
    }
    diag_render();
}

/* Set Dedicated HEAP Kernel Heap Engine Telemetry */
void diag_set_heap_telemetry(uint64_t base, uint64_t size_kb, uint64_t used_kb, uint64_t free_kb, uint64_t allocs, uint64_t frees, uint64_t leaks, uint64_t corruptions, uint64_t largest_free_kb, uint64_t last_alloc, uint64_t last_rip, const char *status_str) {
    g_abde.heap_active = true;
    g_abde.heap_base = base;
    g_abde.heap_size_kb = size_kb;
    g_abde.heap_used_kb = used_kb;
    g_abde.heap_free_kb = free_kb;
    g_abde.heap_alloc_count = allocs;
    g_abde.heap_free_count = frees;
    g_abde.heap_leak_count = leaks;
    g_abde.heap_corruption_count = corruptions;
    g_abde.heap_largest_free_kb = largest_free_kb;
    g_abde.heap_last_alloc = last_alloc;
    g_abde.heap_last_caller_rip = last_rip;
    if (status_str) {
        abde_strcpy(g_abde.heap_status_str, status_str, 16);
    }
    diag_render();
}

/* Per-CPU Heartbeat Counter */
void diag_cpu_heartbeat(uint32_t cpu_id) {
    if (cpu_id < ABDE_MAX_CPUS) {
        g_abde.cpus[cpu_id].online = true;
        g_abde.cpus[cpu_id].heartbeat++;
    }
    diag_render();
}

/* Heartbeat Tick for Safe Active Spinner Loop */
void diag_heartbeat_tick(void) {
    g_abde.heartbeat_ticks++;
    diag_cpu_heartbeat(0); // BSP Core Heartbeat
}

/* Safe Non-Freezing Panic Handler */
void diag_panic_reason(const char *module, const char *step, const char *err, const char *detail) {
    diag_set_fail(module);
    abde_strcpy(g_abde.current_step, step, ABDE_MAX_STEP_LEN);
    diag_set_fault(err, detail);

    for (;;) {
        diag_heartbeat_tick();
        for (volatile int i = 0; i < 5000000; i++) {
            __asm__ __volatile__("nop");
        }
    }
}
