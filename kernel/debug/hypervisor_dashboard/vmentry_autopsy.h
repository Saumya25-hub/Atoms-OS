/*
 * ATOMS OS — VM-Entry Autopsy Engine
 * Architecture: Intel VT-x (VMX) Silicon Diagnostic & Forensic Subsystem
 * Target: Intel Core i3-14100F / ASUS PRIME B760M-K (LGA1700)
 * Version: 1.0
 */

#ifndef ATOMS_VMENTRY_AUTOPSY_H
#define ATOMS_VMENTRY_AUTOPSY_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "kernel/core/hypervisor/include/hypervisor.h"
#include "kernel/core/core_legacy/boot/include/boot_info.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AUTOPSY_MAX_WRITE_AUDITS 64
#define AUTOPSY_MAX_FINDINGS     32
#define AUTOPSY_MAX_DESCRIPTORS  8

/* --------------------------------------------------------------------------
 * Finding Classification & Severity
 * -------------------------------------------------------------------------- */
typedef enum {
    AUTOPSY_SEV_CRITICAL = 0,
    AUTOPSY_SEV_HIGH,
    AUTOPSY_SEV_MEDIUM,
    AUTOPSY_SEV_LOW,
    AUTOPSY_SEV_INFO
} AutopsySeverity;

typedef enum {
    AUTOPSY_STATUS_PROVEN_FAILURE = 0,
    AUTOPSY_STATUS_PASS,
    AUTOPSY_STATUS_NOT_APPLICABLE,
    AUTOPSY_STATUS_UNKNOWN
} AutopsyFindingStatus;

typedef struct {
    uint32_t            id;
    AutopsySeverity     severity;
    char                field[32];
    uint64_t            actual_val;
    uint64_t            expected_val;
    char                rule_desc[96];
    char                evidence[128];
    AutopsyFindingStatus status;
} AutopsyFinding;

/* --------------------------------------------------------------------------
 * VMCS Write / Readback Audit (Panel 20)
 * -------------------------------------------------------------------------- */
typedef struct {
    uint64_t    field;
    uint64_t    val_written;
    uint64_t    val_readback;
    bool        matched;
} VMCSWriteAuditEntry;

/* --------------------------------------------------------------------------
 * GDT Descriptor Byte Autopsy (Panel 9 & 11)
 * -------------------------------------------------------------------------- */
typedef struct {
    const char  *name;
    uint16_t    selector;
    uint16_t    index;
    bool        is_tss;
    uint8_t     raw_bytes[16];
    uint64_t    base;
    uint32_t    limit;
    uint8_t     type;
    bool        s_bit;
    uint8_t     dpl;
    bool        p_bit;
    bool        l_bit;
    bool        db_bit;
    bool        g_bit;
    uint32_t    vmcs_ar;
    uint64_t    vmcs_base;
    uint32_t    vmcs_limit;
    bool        descriptor_match;
} GDTDescriptorAuditEntry;

/* --------------------------------------------------------------------------
 * Complete VMCS & Silicon State Snapshot (Panels 1 - 25)
 * -------------------------------------------------------------------------- */
typedef struct {
    uint64_t    tsc;
    uint32_t    apic_id;
    char        run_id[32];

    /* Instruction Results (Panel 1) */
    bool        vmxon_ok;
    bool        vmclear_ok;
    bool        vmptrld_ok;
    bool        vmlaunch_attempted;
    bool        vmlaunch_instruction_ok;
    bool        vmentry_accepted;
    uint64_t    rflags;
    uint32_t    instruction_error;
    uint32_t    exit_reason;
    uint32_t    basic_exit_reason;
    bool        is_entry_failure;
    uint64_t    exit_qualification;
    uint32_t    exit_instruction_len;
    uint32_t    guest_instructions_executed;

    /* Guest State (Panel 2) */
    uint64_t    guest_cr0;
    uint64_t    guest_cr3;
    uint64_t    guest_cr4;
    uint64_t    guest_rip;
    uint64_t    guest_rsp;
    uint64_t    guest_rflags;
    uint64_t    guest_dr7;
    uint64_t    guest_efer;
    uint64_t    guest_pat;
    uint64_t    guest_debugctl;

    uint16_t    guest_cs_sel; uint64_t guest_cs_base; uint32_t guest_cs_lim; uint32_t guest_cs_ar;
    uint16_t    guest_ss_sel; uint64_t guest_ss_base; uint32_t guest_ss_lim; uint32_t guest_ss_ar;
    uint16_t    guest_ds_sel; uint64_t guest_ds_base; uint32_t guest_ds_lim; uint32_t guest_ds_ar;
    uint16_t    guest_es_sel; uint64_t guest_es_base; uint32_t guest_es_lim; uint32_t guest_es_ar;
    uint16_t    guest_fs_sel; uint64_t guest_fs_base; uint32_t guest_fs_lim; uint32_t guest_fs_ar;
    uint16_t    guest_gs_sel; uint64_t guest_gs_base; uint32_t guest_gs_lim; uint32_t guest_gs_ar;
    uint16_t    guest_tr_sel; uint64_t guest_tr_base; uint32_t guest_tr_lim; uint32_t guest_tr_ar;
    uint16_t    guest_ldtr_sel; uint64_t guest_ldtr_base; uint32_t guest_ldtr_lim; uint32_t guest_ldtr_ar;

    uint64_t    guest_gdtr_base; uint32_t guest_gdtr_lim;
    uint64_t    guest_idtr_base; uint32_t guest_idtr_lim;

    uint32_t    guest_sysenter_cs;
    uint64_t    guest_sysenter_esp;
    uint64_t    guest_sysenter_eip;

    uint32_t    guest_activity_state;
    uint32_t    guest_interruptibility;
    uint64_t    guest_pending_dbg_exceptions;
    uint64_t    vmcs_link_pointer;

    uint64_t    pdpte0; uint64_t pdpte1; uint64_t pdpte2; uint64_t pdpte3;
    uint64_t    eptp;

    /* Controls (Panels 6 - 8) */
    uint32_t    pin_ctls;
    uint32_t    proc_ctls;
    uint32_t    sec_ctls;
    uint32_t    exit_ctls;
    uint32_t    entry_ctls;

    /* Hardware MSR Capabilities */
    uint64_t    msr_vmx_basic;
    uint64_t    msr_entry_ctls;
    uint64_t    msr_exit_ctls;
    uint64_t    msr_pin_ctls;
    uint64_t    msr_proc_ctls;
    uint64_t    msr_sec_ctls;
    uint64_t    msr_cr0_fixed0;
    uint64_t    msr_cr0_fixed1;
    uint64_t    msr_cr4_fixed0;
    uint64_t    msr_cr4_fixed1;
    uint64_t    msr_host_efer;

    /* Host State (Panel 21 & 22) */
    uint64_t    host_cr0;
    uint64_t    host_cr3;
    uint64_t    host_cr4;
    uint64_t    host_rsp;
    uint64_t    host_rip;
    uint16_t    host_cs; uint16_t host_ss; uint16_t host_ds;
    uint16_t    host_es; uint16_t host_fs; uint16_t host_gs; uint16_t host_tr;
    uint64_t    host_fs_base; uint64_t host_gs_base; uint64_t host_tr_base;
    uint64_t    host_gdtr_base; uint64_t host_idtr_base;
    uint64_t    host_stack_base; uint64_t host_stack_top;

    /* FreeBSD Entry State (Panel 24) */
    uint64_t    freebsd_entry_rip;
    uint64_t    freebsd_stack_gpa;
    uint64_t    freebsd_modulep_gpa;
} AutopsySnapshot;

/* --------------------------------------------------------------------------
 * Autopsy Engine Master State
 * -------------------------------------------------------------------------- */
typedef struct {
    bool                    initialized;
    char                    cpu_brand[64];
    char                    platform[32];
    char                    run_id[32];

    /* Snapshots (Panel 25 & 26) */
    AutopsySnapshot         pre_launch;
    AutopsySnapshot         post_failure;
    bool                    has_pre_launch;
    bool                    has_post_failure;

    /* Write Audits (Panel 20) */
    VMCSWriteAuditEntry     write_audits[AUTOPSY_MAX_WRITE_AUDITS];
    uint32_t                write_audit_count;
    uint32_t                write_audit_mismatches;

    /* Descriptor Audits (Panel 9 & 11) */
    GDTDescriptorAuditEntry desc_audits[AUTOPSY_MAX_DESCRIPTORS];
    uint32_t                desc_audit_count;

    /* Rule Findings (Panel 27) */
    AutopsyFinding          findings[AUTOPSY_MAX_FINDINGS];
    uint32_t                finding_count;
    uint32_t                proven_failure_count;
    char                    first_proven_field[64];
    char                    first_proven_rule[96];
    char                    top_suspect_1[96];
    char                    top_suspect_2[96];
    char                    top_suspect_3[96];
    char                    confidence_str[16];

    /* Run-to-Run Diff (Engine) */
    bool                    has_previous_run;
    AutopsySnapshot         previous_run;
    char                    diff_changed_fields[256];
    char                    diff_unchanged_fields[256];

    /* Reference Implementation Correlation Matrix (Mode 2) */
    struct {
        char sdm_status[96];
        char kvm_diff[96];
        char bhyve_diff[96];
        char xen_diff[96];
        char vmcs_readback[96];
        char silicon_result[96];
    } ref_matrix;
} VmEntryAutopsyEngine;

extern VmEntryAutopsyEngine g_autopsy_engine;

/* --------------------------------------------------------------------------
 * Autopsy Engine Public API
 * -------------------------------------------------------------------------- */
void vmentry_autopsy_init(boot_info_t *boot_info);
void vmentry_autopsy_record_write(uint64_t field, uint64_t val_written);
void vmentry_autopsy_pre_launch_snapshot(vCPU *vcpu);
void vmentry_autopsy_post_failure_autopsy(vCPU *vcpu);
void vmentry_autopsy_evaluate_all_rules(void);
void vmentry_autopsy_evaluate_reference_correlation(void);
void vmentry_autopsy_render_operator_view(void);
void vmentry_autopsy_render_machine_log_panel(void);
void vmentry_autopsy_render_heartbeat(char spin_char);
void vmentry_autopsy_emit_machine_log(void);

/* Snack Bot Deep Hardware & Silicon Sniffer API */
void snack_bot_run_deep_probe(void);

#ifdef __cplusplus
}
#endif

#endif /* ATOMS_VMENTRY_AUTOPSY_H */
