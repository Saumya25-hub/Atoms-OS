#ifndef ABDE_H
#define ABDE_H

#include <stdint.h>
#include <stdbool.h>
#include "kernel/core/core_legacy/boot/include/boot_info.h"

/* ABDE Module Status Types */
typedef enum {
    DIAG_STATUS_WAIT = 0,
    DIAG_STATUS_RUNNING,
    DIAG_STATUS_PASS,
    DIAG_STATUS_FAIL
} diag_status_t;

/* Maximum lengths */
#define ABDE_MAX_MODULES    16
#define ABDE_MAX_NAME_LEN   32
#define ABDE_MAX_STEP_LEN   32
#define ABDE_MAX_ERR_LEN    32
#define ABDE_MAX_DETAIL_LEN 64
#define ABDE_MAX_CPUS       8

/* Single Diagnostic Subsystem Module Definition */
typedef struct {
    char name[ABDE_MAX_NAME_LEN];
    diag_status_t status;
} diag_module_t;

/* Per-CPU Health Tracker for Live Grid */
typedef struct {
    bool online;
    bool starting;
    bool failed;
    uint64_t heartbeat;
} abde_cpu_health_t;

/* ABDE Global Diagnostic Engine State V2.5 Forensic Dashboard */
typedef struct {
    boot_info_t *boot_info;
    uint64_t framebuffer;
    uint32_t pitch;
    uint32_t width;
    uint32_t height;

    diag_module_t modules[ABDE_MAX_MODULES];
    uint32_t module_count;

    char current_module[ABDE_MAX_NAME_LEN];
    char current_step[ABDE_MAX_STEP_LEN];
    char last_event[ABDE_MAX_STEP_LEN];
    diag_status_t overall_status;
    char error_code[ABDE_MAX_ERR_LEN];
    char fault_detail[ABDE_MAX_DETAIL_LEN];

    // Dedicated SMP Multi-Core Forensic Telemetry Fields
    uint32_t smp_bsp_id;
    uint32_t smp_cpu_found;
    uint32_t smp_cpu_online;
    uint32_t smp_current_cpu;
    uint32_t smp_init_ipis;
    uint32_t smp_sipis_sent;
    uint32_t smp_ap_responses;
    uint32_t smp_last_ap;

    // Dedicated IDT Engine Telemetry Fields
    uint32_t idt_entries;
    uint64_t idt_base;
    uint32_t isr_installed;
    bool     exceptions_armed;
    char     last_interrupt[ABDE_MAX_STEP_LEN];
    char     last_exception[ABDE_MAX_STEP_LEN];
    uint32_t fault_count;

    // Dedicated PIC / APIC Live Telemetry Fields
    bool     pic_remapped;
    bool     apic_enabled;
    uint64_t ioapic_base;
    uint64_t timer_irq0_ticks;
    uint64_t kbd_irq1_count;
    uint32_t last_irq;
    uint32_t last_vector;

    // Per-CPU Live Heartbeat Grid
    abde_cpu_health_t cpus[ABDE_MAX_CPUS];

    uint64_t heartbeat_ticks;
} abde_engine_t;

/* Global Engine Instance */
extern abde_engine_t g_abde;

/* Public ABDE V2.5 API Functions */
void diag_init(boot_info_t *boot_info);
void diag_set_running(const char *module_name);
void diag_set_pass(const char *module_name);
void diag_set_fail(const char *module_name);
void diag_set_step(const char *step_name);
void diag_set_error(const char *error_code);
void diag_set_fault(const char *error_code, const char *detail);
void diag_set_smp_telemetry(uint32_t found, uint32_t online, uint32_t current_cpu, uint32_t init_ipis, uint32_t sipis, uint32_t responses);
void diag_set_idt_telemetry(uint32_t entries, uint64_t base, uint32_t isr_count, bool armed, const char *last_exc, uint32_t faults);
void diag_set_pic_telemetry(bool pic_remap, bool apic_en, uint64_t ioapic, uint64_t timer_ticks, uint64_t kbd_count, uint32_t last_irq, uint32_t last_vec);
void diag_cpu_heartbeat(uint32_t cpu_id);
void diag_heartbeat_tick(void);
void diag_render(void);
void diag_panic_reason(const char *module, const char *step, const char *err, const char *detail);

/* Low-level Framebuffer Text Renderer Helper Declarations */
void abde_render_char(uint32_t x, uint32_t y, char c, uint32_t fg_color, uint32_t bg_color);
void abde_render_string(uint32_t x, uint32_t y, const char *str, uint32_t fg_color, uint32_t bg_color);
void abde_render_string_padded(uint32_t x, uint32_t y, const char *str, uint32_t max_chars, uint32_t fg_color, uint32_t bg_color);
void abde_fill_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t color);

#endif // ABDE_H
