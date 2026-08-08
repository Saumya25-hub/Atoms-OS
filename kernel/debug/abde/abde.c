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

    // Register Subsystem Certification Board Modules
    abde_add_module("CPU",  DIAG_STATUS_PASS);
    abde_add_module("GDT",  DIAG_STATUS_PASS);
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
