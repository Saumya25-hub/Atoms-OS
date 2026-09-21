/*
 * ATOMS OS — Hardware Virtualization Hypervisor Core Interface
 * Copyright © 2026 ATOMS OS Project / Saumya Chaudhari
 *
 * Phase 1: VMX / SVM Hardware-Assisted Virtualization Foundation
 */

#ifndef ATOMS_HYPERVISOR_H
#define ATOMS_HYPERVISOR_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Hypervisor Architecture Backend Types */
typedef enum {
    HYPERVISOR_BACKEND_NONE = 0,
    HYPERVISOR_BACKEND_INTEL_VMX = 1,
    HYPERVISOR_BACKEND_AMD_SVM = 2
} HypervisorBackend;

/* Virtual Machine Execution Lifecycle State */
typedef enum {
    VM_STATE_UNINITIALIZED = 0,
    VM_STATE_CREATED = 1,
    VM_STATE_INITIALIZED = 2,
    VM_STATE_RUNNING = 3,
    VM_STATE_EXITED = 4,
    VM_STATE_STOPPED = 5,
    VM_STATE_DESTROYED = 6,
    VM_STATE_ERROR = 7
} VMState;

/* Standard x86_64 Register State for vCPU */
typedef struct {
    uint64_t rax;
    uint64_t rbx;
    uint64_t rcx;
    uint64_t rdx;
    uint64_t rsi;
    uint64_t rdi;
    uint64_t rbp;
    uint64_t rsp;
    uint64_t r8;
    uint64_t r9;
    uint64_t r10;
    uint64_t r11;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;
    uint64_t rip;
    uint64_t rflags;
    uint64_t cr0;
    uint64_t cr3;
    uint64_t cr4;
} GuestCpuRegisters;

/* VM Exit Reason Info */
typedef struct {
    uint32_t exit_reason;
    uint64_t exit_qualification;
    uint64_t guest_rip;
    uint64_t guest_rsp;
    uint64_t guest_cr3;
    uint64_t guest_physical_address;
    uint32_t instruction_length;
    bool handled;
} VMExitContext;

/* vCPU Structure */
typedef struct vcpu {
    uint32_t id;
    VMState state;
    GuestCpuRegisters guest_regs;
    VMExitContext last_exit;

    /* Hardware Specific Control Structures (Page-Aligned) */
    void *vmcs_region;       /* Intel VMCS physical/virtual buffer (4KB) */
    uint64_t vmcs_phys;
    void *vmcb_region;       /* AMD VMCB buffer (4KB) */
    uint64_t vmcb_phys;

    /* Host Return Context */
    uint64_t host_rsp;
    uint64_t host_rip;
    bool launched;

    /* Guest Control Registers & Architectural MSRs */
    uint64_t cr0;
    uint64_t cr3;
    uint64_t cr4;
    uint64_t efer;
    uint64_t msr_star;
    uint64_t msr_lstar;
    uint64_t msr_fmask;
    uint64_t msr_fs_base;
    uint64_t msr_gs_base;
    uint64_t msr_kernel_gs_base;

    struct atoms_vm *vm;
} vCPU;

/* Virtual Machine Instance Structure */
typedef struct atoms_vm {
    uint32_t vm_id;
    VMState state;
    HypervisorBackend backend;
    vCPU *bsp_vcpu;

    /* Hypervisor Memory Descriptors (Phase 2 Second-Level Paging) */
    uint64_t guest_ram_size;
    void *guest_ram_host_virt;
    uint64_t guest_ram_host_phys;
    struct guest_memory *guest_mem;

    /* Virtual Hardware & VirtIO Subsystem (Phase 3) */
    struct VirtualPCIBus *pci_bus;
    struct virtio_blk_dev *blk_dev;
    struct virtio_net_dev *net_dev;
    struct virtio_input_dev *input_dev;
    struct virtio_display_dev *display_dev;

    /* Virtual Platform & Chipset Subsystem (Phase 4) */
    struct VirtualPlatform *platform;

    uint64_t total_vmexits;
} VirtualMachine;

/* Global Hypervisor Core API */
bool atoms_hypervisor_init(void);
void atoms_hypervisor_shutdown(void);
HypervisorBackend atoms_hypervisor_get_backend(void);
bool atoms_hypervisor_is_available(void);

/* VM Management */
VirtualMachine *atoms_vm_create(uint64_t ram_size);
bool atoms_vm_initialize(VirtualMachine *vm, uint64_t entry_point);
bool atoms_vm_run(VirtualMachine *vm);
void atoms_vm_stop(VirtualMachine *vm);
void atoms_vm_destroy(VirtualMachine *vm);
bool atoms_vm_boot_freebsd(VirtualMachine *vm, const void *kernel_elf, size_t size);

/* vCPU Management */
vCPU *atoms_vcpu_create(VirtualMachine *vm, uint32_t id);
bool atoms_vcpu_run(vCPU *vcpu);
void atoms_vcpu_destroy(vCPU *vcpu);

/* VM-Exit Dispatcher & Forensic Diagnostic Dumper */
bool atoms_vmexit_dispatch(vCPU *vcpu);
void atoms_hypervisor_dump_vcpu_state(const vCPU *vcpu);

/* Synthetic Verification Test Suite & Genuine Guest Launcher */
bool atoms_hypervisor_run_synthetic_test(void);
bool atoms_hypervisor_boot_genuine_freebsd(void);

/* Forensic Pre-Flight Gate & Instruction Error Decoder */
bool atoms_hypervisor_validate_vmcs_host_state(vCPU *vcpu, char *out_reason, size_t max_len);
bool atoms_hypervisor_validate_vmcs_guest_state(vCPU *vcpu, char *out_reason, size_t max_len);
void atoms_hypervisor_dump_vmcs_snapshot(const vCPU *vcpu, const char *label);
const char *atoms_hypervisor_decode_vm_instruction_error(uint32_t error_code);
bool atoms_hypervisor_setup_vmcs_host_state(vCPU *vcpu);

#ifdef __cplusplus
}
#endif

#endif /* ATOMS_HYPERVISOR_H */
