#ifndef HYPERVISOR_DASHBOARD_H
#define HYPERVISOR_DASHBOARD_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "kernel/core/core_legacy/boot/include/boot_info.h"
#include "kernel/core/hypervisor/include/hypervisor.h"

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * 1. HYPERVISOR EXECUTION STAGES & STATUS DEFINITIONS
 * ========================================================================= */

typedef enum {
    HV_STAGE_HV_BOOT = 0,
    HV_STAGE_CPU_DETECTION,
    HV_STAGE_CPU_FEATURES,
    HV_STAGE_VMX_OR_SVM_ENABLE,
    HV_STAGE_VMXON_OR_SVM_INIT,
    HV_STAGE_VM_CREATE,
    HV_STAGE_VCPU_CREATE,
    HV_STAGE_VMCS_INIT,
    HV_STAGE_VMCS_GUEST_STATE,
    HV_STAGE_VMCS_HOST_STATE,
    HV_STAGE_VMCS_CONTROLS,
    HV_STAGE_EPT_OR_NPT,
    HV_STAGE_GUEST_MEMORY,
    HV_STAGE_VIRTIO,
    HV_STAGE_VIRTUAL_PCI,
    HV_STAGE_UART,
    HV_STAGE_APIC,
    HV_STAGE_ACPI,
    HV_STAGE_FREEBSD_PAYLOAD,
    HV_STAGE_FREEBSD_METADATA,
    HV_STAGE_FREEBSD_PAGING,
    HV_STAGE_VM_ENTRY_PREFLIGHT,
    HV_STAGE_VM_ENTRY,
    HV_STAGE_VM_EXIT,
    HV_STAGE_FREEBSD_KERNEL_EXECUTION,
    HV_STAGE_FREEBSD_DEVICE_DISCOVERY,
    HV_STAGE_FREEBSD_ROOTFS,
    HV_STAGE_FREEBSD_USERSPACE,
    HV_STAGE_MAX
} HypervisorStageId;

typedef enum {
    HV_STATE_NOT_STARTED = 0,
    HV_STATE_RUNNING,
    HV_STATE_PASS,
    HV_STATE_WARN,
    HV_STATE_FAIL,
    HV_STATE_BLOCKED,
    HV_STATE_SKIPPED
} HypervisorStageStatus;

typedef struct {
    HypervisorStageId   id;
    const char          *name;
    HypervisorStageStatus status;
    char                detail[64];
    uint64_t            timestamp_ticks;
} HypervisorStageRecord;

/* =========================================================================
 * 2. FORENSIC PANEL STRUCTURES
 * ========================================================================= */

/* CPU Forensic Information */
typedef struct {
    char        vendor[16];
    char        brand[64];
    uint32_t    family;
    uint32_t    model;
    uint32_t    stepping;
    uint32_t    logical_cpus;
    uint32_t    apic_id;
    bool        has_vmx;
    bool        has_svm;
    bool        has_ept;
    bool        has_npt;
    bool        has_vpid;
    bool        has_unrestricted_guest;
    bool        has_nx;
    bool        has_smep;
    bool        has_smap;
    bool        has_pge;
    bool        has_pae;
    bool        has_long_mode;
    uint32_t    cpuid_1_ecx;
    uint32_t    cpuid_1_edx;
    uint32_t    cpuid_80000001_edx;
} CPUForensicInfo;

/* VMX / Hardware Control Information */
typedef struct {
    uint64_t    feature_control;
    uint64_t    cr0;
    uint64_t    cr3;
    uint64_t    cr4;
    uint64_t    efer;
    uint64_t    vmxon_phys;
    uint32_t    vmxon_rev_id;
    bool        vmxon_result;
    uint64_t    vmx_basic_msr;
    uint32_t    pin_ctls_actual;
    uint32_t    proc_ctls_actual;
    uint32_t    sec_ctls_actual;
    uint32_t    exit_ctls_actual;
    uint32_t    entry_ctls_actual;
    uint64_t    ept_vpid_cap_msr;
} VMXForensicInfo;

/* GDT / IDT / TSS Information */
typedef struct {
    uint64_t    gdtr_base;
    uint16_t    gdtr_limit;
    uint64_t    idtr_base;
    uint16_t    idtr_limit;
    uint16_t    tr_selector;
    uint64_t    tr_base;
    uint32_t    tr_limit;
    uint8_t     tr_type;
    bool        tr_present;
    uint16_t    expected_tr_selector;
    uint64_t    expected_tr_base;
    bool        tr_selector_match;
    bool        tr_base_match;
} GDTIDTTSSForensicInfo;

/* Host VMCS Field Validation */
typedef struct {
    uint64_t    actual;
    uint64_t    expected;
    uint64_t    vmcs_val;
    bool        valid;
} HostFieldCheck;

typedef struct {
    HostFieldCheck cr0;
    HostFieldCheck cr3;
    HostFieldCheck cr4;
    HostFieldCheck cs;
    HostFieldCheck ss;
    HostFieldCheck ds;
    HostFieldCheck es;
    HostFieldCheck fs;
    HostFieldCheck gs;
    HostFieldCheck tr;
    HostFieldCheck fs_base;
    HostFieldCheck gs_base;
    HostFieldCheck tr_base;
    HostFieldCheck gdtr_base;
    HostFieldCheck idtr_base;
    HostFieldCheck rsp;
    HostFieldCheck rip;
    HostFieldCheck efer;
    bool           all_host_fields_valid;
} VMCSHostForensicInfo;

/* Guest VMCS Field State */
typedef struct {
    uint64_t    rip;
    uint64_t    rsp;
    uint64_t    rflags;
    uint64_t    cr0;
    uint64_t    cr3;
    uint64_t    cr4;
    uint64_t    efer;
    uint16_t    cs;
    uint16_t    ds;
    uint16_t    ss;
    uint16_t    es;
    uint16_t    fs;
    uint16_t    gs;
    uint16_t    tr;
    uint16_t    ldtr;
    uint64_t    gdtr_base;
    uint16_t    gdtr_limit;
    uint64_t    idtr_base;
    uint16_t    idtr_limit;
} VMCSGuestForensicInfo;

/* EPT Mapping Entry */
typedef struct {
    uint64_t    gpa;
    uint64_t    hpa;
    uint64_t    size;
    bool        read;
    bool        write;
    bool        exec;
    const char  *backing_type;
} EPTMappingEntry;

#define HV_MAX_EPT_MAPPINGS 16

typedef struct {
    HypervisorBackend backend;
    uint64_t    eptp;
    uint64_t    pml4_phys;
    uint32_t    mapping_count;
    EPTMappingEntry mappings[HV_MAX_EPT_MAPPINGS];
    uint32_t    violation_count;
    uint64_t    last_violation_gpa;
    uint64_t    last_violation_rip;
    uint64_t    last_violation_qual;
} EPTForensicInfo;

/* Guest Memory Status */
typedef struct {
    uint64_t    ram_size_bytes;
    uint64_t    ram_base_gpa;
    uint64_t    used_bytes;
    uint64_t    free_bytes;
    uint64_t    kernel_footprint_bytes;
    uint64_t    metadata_gpa;
    uint64_t    stack_gpa;
    bool        oob_detected;
    bool        unaligned_detected;
    bool        overlap_detected;
} GuestMemoryForensicInfo;

/* FreeBSD Loader & Payload Identity */
typedef struct {
    const char  *version_str;
    const char  *payload_path;
    uint64_t    file_size;
    bool        is_valid_elf64;
    uint64_t    entry_point;
    uint32_t    pt_load_count;
    uint64_t    kernend_gpa;
    uint64_t    modulep_gpa;
    uint64_t    envp_gpa;
    uint64_t    stack_top_gpa;
    bool        payload_sha256_verified;
    char        sha256_hex[65];
} FreeBSDLoaderForensicInfo;

/* VirtIO Virtual Device Status & Safety Interlock */
typedef struct {
    bool        pci_bus_active;
    bool        blk_active;
    bool        net_active;
    bool        input_active;
    bool        gpu_active;
    uint64_t    blk_capacity_bytes;
    uint64_t    blk_sectors;
    const char  *blk_backing_desc;
    uint32_t    blk_reads;
    uint32_t    blk_writes;
    /* Host Safety Interlock: Prohibit any host physical disk or NTFS partition access */
    bool        host_physical_disk_attached;
    bool        host_ntfs_attached;
    bool        safety_interlock_passed;
} VirtIOForensicInfo;

/* UART / Serial Segregation */
#define HV_UART_LOG_BUFFER_SIZE 4096

typedef struct {
    uint16_t    port_base;
    uint32_t    tx_byte_count;
    uint32_t    rx_byte_count;
    uint32_t    hypervisor_log_bytes;
    uint32_t    guest_uart_bytes;
    char        guest_uart_stream[HV_UART_LOG_BUFFER_SIZE];
    uint32_t    guest_uart_pos;
} UARTForensicInfo;

/* VM-Exit & VM-Instruction Error Forensic Info */
typedef struct {
    uint64_t    sequence_num;
    uint64_t    timestamp_ticks;
    uint32_t    exit_reason;
    const char  *exit_reason_str;
    uint64_t    exit_qualification;
    uint32_t    instruction_len;
    uint64_t    guest_rip;
    uint64_t    guest_rsp;
    uint64_t    guest_gpa;
    GuestCpuRegisters guest_regs;
    bool        vmlaunch_attempted;
    bool        vmlaunch_success;
    uint32_t    vm_instruction_error_code;
    const char  *vm_instruction_error_str;
} VMExitForensicInfo;

/* Event Timeline Record */
typedef struct {
    uint64_t    timestamp_ticks;
    HypervisorStageId stage;
    const char  *event_str;
    HypervisorStageStatus status;
} DashboardEvent;

#define HV_MAX_TIMELINE_EVENTS 64

/* Failure Snapshot */
typedef struct {
    bool        snapshot_valid;
    uint64_t    timestamp_ticks;
    HypervisorStageId failed_stage;
    char        failure_reason[128];
    char        failing_component[32];
    CPUForensicInfo cpu;
    VMXForensicInfo vmx;
    GDTIDTTSSForensicInfo gdt_tss;
    VMCSHostForensicInfo host_vmcs;
    VMCSGuestForensicInfo guest_vmcs;
    EPTForensicInfo ept;
    VMExitForensicInfo vmexit;
} FailureSnapshot;

/* =========================================================================
 * 3. MASTER FORENSIC DASHBOARD STATE
 * ========================================================================= */

typedef struct {
    boot_info_t             *boot_info;
    bool                    initialized;
    bool                    stop_on_fail_active;
    bool                    halted;
    HypervisorStageId       active_stage;
    HypervisorStageRecord   stages[HV_STAGE_MAX];
    uint32_t                total_stages_run;
    uint32_t                total_stages_passed;
    uint32_t                total_stages_failed;
    uint32_t                total_stages_blocked;

    /* Panels */
    CPUForensicInfo         cpu_info;
    VMXForensicInfo         vmx_info;
    GDTIDTTSSForensicInfo   gdt_tss_info;
    VMCSHostForensicInfo    host_vmcs_info;
    VMCSGuestForensicInfo   guest_vmcs_info;
    EPTForensicInfo         ept_info;
    GuestMemoryForensicInfo mem_info;
    FreeBSDLoaderForensicInfo freebsd_info;
    VirtIOForensicInfo      virtio_info;
    UARTForensicInfo        uart_info;
    VMExitForensicInfo      last_vmexit_info;

    /* Timeline & Snapshot */
    DashboardEvent          timeline[HV_MAX_TIMELINE_EVENTS];
    uint32_t                timeline_count;
    FailureSnapshot         failure_snapshot;

    /* Run Identification */
    char                    run_id[32];
    uint64_t                run_start_ticks;
} HypervisorDashboardState;

/* Global Dashboard Singleton */
extern HypervisorDashboardState g_hv_dashboard;

/* =========================================================================
 * 4. PUBLIC DASHBOARD API
 * ========================================================================= */

void hypervisor_dashboard_init(boot_info_t *boot_info);
void hypervisor_dashboard_reset(void);
void hypervisor_dashboard_log_event(HypervisorStageId stage, const char *event_str, HypervisorStageStatus status);
void hypervisor_dashboard_set_stage(HypervisorStageId stage, HypervisorStageStatus status, const char *detail);
void hypervisor_dashboard_trigger_failure(HypervisorStageId stage, const char *component, const char *reason);

/* Pre-Flight Gate */
bool hypervisor_dashboard_verify_vmcs_host_preflight(vCPU *vcpu);

/* Visual & Stream Renderers */
void hypervisor_dashboard_render_frame(void);
void hypervisor_dashboard_emit_telemetry_stream(void);
void hypervisor_dashboard_dump_failure_snapshot(void);

/* Main Entry Points */
void hypervisor_dashboard_run(boot_info_t *boot_info);
bool hypervisor_dashboard_run_stage_pipeline(void);

/* Deep VM-Entry On-Screen Forensic Mirror */
typedef struct {
    bool        valid;
    char        classification[64];
    uint32_t    cf;
    uint32_t    zf;
    uint64_t    rflags;
    uint32_t    raw_exit_reason;
    uint32_t    basic_exit_reason;
    bool        is_entry_failure;
    const char  *exit_reason_name;
    uint64_t    exit_qualification;
    uint32_t    instruction_error;
    const char  *instruction_error_name;

    uint64_t    guest_cr0;
    uint64_t    guest_cr3;
    uint64_t    guest_cr4;
    uint64_t    guest_efer;
    uint64_t    guest_rip;
    uint64_t    guest_rsp;
    uint64_t    guest_rflags;
    uint64_t    guest_dr7;

    uint16_t    cs_sel; uint64_t cs_base; uint32_t cs_lim; uint32_t cs_ar;
    uint16_t    ss_sel; uint64_t ss_base; uint32_t ss_lim; uint32_t ss_ar;
    uint16_t    ds_sel; uint64_t ds_base; uint32_t ds_lim; uint32_t ds_ar;
    uint16_t    es_sel; uint64_t es_base; uint32_t es_lim; uint32_t es_ar;
    uint16_t    fs_sel; uint64_t fs_base; uint32_t fs_lim; uint32_t fs_ar;
    uint16_t    gs_sel; uint64_t gs_base; uint32_t gs_lim; uint32_t gs_ar;
    uint16_t    tr_sel; uint64_t tr_base; uint32_t tr_lim; uint32_t tr_ar;
    uint16_t    ldtr_sel; uint64_t ldtr_base; uint32_t ldtr_lim; uint32_t ldtr_ar;

    uint64_t    gdtr_base; uint32_t gdtr_limit;
    uint64_t    idtr_base; uint32_t idtr_limit;
    uint64_t    eptp;

    /* VMCS Execution Controls */
    uint32_t    pin_ctls;
    uint32_t    proc_ctls;
    uint32_t    sec_ctls;
    uint32_t    exit_ctls;
    uint32_t    entry_ctls;

    /* Hardware MSR Invariants */
    uint64_t    msr_entry_ctls;
    uint64_t    msr_cr0_fixed0;
    uint64_t    msr_cr0_fixed1;
    uint64_t    msr_cr4_fixed0;
    uint64_t    msr_cr4_fixed1;

    /* Guest Paging & PDPTEs */
    uint64_t    pdpte0;
    uint64_t    pdpte1;
    uint64_t    pdpte2;
    uint64_t    pdpte3;

    /* SYSENTER & Link Pointer */
    uint32_t    sysenter_cs;
    uint64_t    sysenter_esp;
    uint64_t    sysenter_eip;
    uint64_t    link_pointer;

    /* Automated Silicon Rule Evaluator */
    bool        rule_eval_failed;
    char        rule_eval_field[64];
    char        rule_eval_msg[128];

    char        suspected_field[128];
} VMEntryScreenForensics;

extern VMEntryScreenForensics g_vmentry_screen_forensics;
void hypervisor_dashboard_render_vmentry_forensics(void);

#ifdef __cplusplus
}
#endif

#endif /* HYPERVISOR_DASHBOARD_H */
