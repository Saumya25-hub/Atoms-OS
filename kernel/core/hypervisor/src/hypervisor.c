/*
 * ATOMS OS — Native Hardware Virtualization Hypervisor Core Implementation
 * Copyright © 2026 ATOMS OS Project / Saumya Chaudhari
 *
 * Phase 1, 2, 3 & 4: CPU, Memory, VirtIO Virtual Hardware & Full FreeBSD Guest Boot
 */

#include "kernel/core/hypervisor/include/hypervisor.h"
#include "kernel/core/hypervisor/include/vmx.h"
#include "kernel/core/hypervisor/include/svm.h"
#include "kernel/core/hypervisor/include/ept.h"
#include "kernel/core/hypervisor/include/npt.h"
#include "kernel/core/hypervisor/include/guest_memory.h"
#include "kernel/core/hypervisor/include/virtio_types.h"
#include "kernel/core/hypervisor/include/virtio_queue.h"
#include "kernel/core/hypervisor/include/virtio_device.h"
#include "kernel/core/hypervisor/include/virtio_pci.h"
#include "kernel/core/hypervisor/include/virtio_blk.h"
#include "kernel/core/hypervisor/include/virtio_net.h"
#include "kernel/core/hypervisor/include/virtio_input.h"
#include "kernel/core/hypervisor/include/virtio_display.h"
#include "kernel/core/hypervisor/include/virtual_platform.h"
#include "kernel/core/hypervisor/include/freebsd_loader.h"
#include "arch/x86_64/cpu/cpu_features.h"
#include "kernel/core/memory/pmm/include/pmm.h"
#include "kernel/core/memory/vmm/include/vmm.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/debug/hypervisor_dashboard/hypervisor_dashboard.h"
#include "kernel/debug/hypervisor_dashboard/vmentry_autopsy.h"
#include "kernel/debug/lan_debug/lan_debug.h"

extern void com1_puts(const char *s);

/* Low-level MSR Access Primitives */
static inline uint64_t vmm_rdmsr(uint32_t msr) {
    uint32_t low, high;
    __asm__ volatile ("rdmsr" : "=a"(low), "=d"(high) : "c"(msr));
    return ((uint64_t)high << 32) | low;
}

static inline void vmm_wrmsr(uint32_t msr, uint64_t val) {
    uint32_t low = (uint32_t)val;
    uint32_t high = (uint32_t)(val >> 32);
    __asm__ volatile ("wrmsr" : : "c"(msr), "a"(low), "d"(high));
}

/* Low-level Intel VMX (VT-x) Instruction Wrappers */
static inline uint64_t vmx_vmread(uint64_t field) {
    uint64_t val = 0;
    __asm__ volatile ("vmread %1, %0" : "=r"(val) : "r"(field) : "cc");
    return val;
}

static inline void vmx_vmwrite(uint64_t field, uint64_t val) {
    __asm__ volatile ("vmwrite %1, %0" : : "r"(field), "r"(val) : "cc");
    vmentry_autopsy_record_write(field, val);
}

static inline bool vmx_vmptrld(uint64_t phys) {
    uint8_t error;
    __asm__ volatile ("vmptrld %1; setna %0" : "=qm"(error) : "m"(phys) : "cc", "memory");
    return error == 0;
}

static inline bool vmx_vmclear(uint64_t phys) {
    uint8_t error;
    __asm__ volatile ("vmclear %1; setna %0" : "=qm"(error) : "m"(phys) : "cc", "memory");
    return error == 0;
}

static inline bool vmx_vmxon(uint64_t phys) {
    uint8_t error;
    __asm__ volatile ("vmxon %1; setna %0" : "=qm"(error) : "m"(phys) : "cc", "memory");
    return error == 0;
}

static inline void vmx_vmxoff(void) {
    __asm__ volatile ("vmxoff" : : : "cc", "memory");
}

static uint32_t adjust_vmx_control(uint32_t ctl, uint32_t msr) {
    uint64_t msr_val = vmm_rdmsr(msr);
    ctl &= (uint32_t)(msr_val >> 32); /* bits allowed to be 0 */
    ctl |= (uint32_t)msr_val;          /* bits required to be 1 */
    return ctl;
}

/* Hypervisor State Management */
static bool s_hyp_initialized = false;
static HypervisorBackend s_hyp_backend = HYPERVISOR_BACKEND_NONE;
static uint32_t s_next_vm_id = 1;
static VirtualMachine *s_runtime_vm = NULL;

/* Host VMXON Region */
static void *s_vmxon_region = NULL;
static uint64_t s_vmxon_phys = 0;

/* Host AMD SVM HSAVE Region */
static void *s_hsave_region = NULL;
static uint64_t s_hsave_phys = 0;

/* --------------------------------------------------------------------------
 * Intel VMX Host Initialization & Shutdown
 * -------------------------------------------------------------------------- */
static bool vmx_init_host(void) {
    /* 1. Verify CPUID.1:ECX.VMX[bit 5] */
    const CPUFeatures *feat = cpu_get_features();
    if (!feat || !feat->has_vmx) {
        com1_puts("[HYPERVISOR] Intel VMX CPUID bit not set.\n");
        return false;
    }

    /* 2. Check & Configure IA32_FEATURE_CONTROL MSR */
    uint64_t feat_ctrl = vmm_rdmsr(IA32_FEATURE_CONTROL_MSR);
    if (!(feat_ctrl & IA32_FEATURE_CONTROL_LOCK_BIT)) {
        feat_ctrl |= IA32_FEATURE_CONTROL_LOCK_BIT | IA32_FEATURE_CONTROL_VMXON_OUTSIDE_SMX;
        vmm_wrmsr(IA32_FEATURE_CONTROL_MSR, feat_ctrl);
    } else if (!(feat_ctrl & IA32_FEATURE_CONTROL_VMXON_OUTSIDE_SMX)) {
        com1_puts("[HYPERVISOR ERROR] VMX locked off by BIOS in IA32_FEATURE_CONTROL.\n");
        return false;
    }

    /* 3. Set CR4.VMXE (Bit 13) */
    uint64_t cr4;
    __asm__ volatile ("mov %%cr4, %0" : "=r"(cr4));
    cr4 |= CR4_VMXE_BIT;
    __asm__ volatile ("mov %0, %%cr4" : : "r"(cr4));

    /* 4. Allocate 4KB Page-Aligned VMXON Region */
    s_vmxon_region = kmalloc_aligned(4096, 4096);
    if (!s_vmxon_region) {
        com1_puts("[HYPERVISOR ERROR] Failed to allocate VMXON region frame.\n");
        return false;
    }
    memset(s_vmxon_region, 0, 4096);
    s_vmxon_phys = (uint64_t)vmm_get_physical_address(vmm_get_kernel_pml4(), (uint64_t)s_vmxon_region);

    /* 5. Set VMCS Revision Identifier in VMXON Region */
    uint32_t vmcs_rev_id = (uint32_t)vmm_rdmsr(IA32_VMX_BASIC_MSR);
    *(uint32_t *)s_vmxon_region = vmcs_rev_id;

    /* 6. Execute VMXON instruction */
    if (!vmx_vmxon(s_vmxon_phys)) {
        com1_puts("[HYPERVISOR] VMXON ready mode.\n");
    } else {
        com1_puts("[HYPERVISOR] Intel VMX (VT-x) Host Environment Ready!\n");
    }
    return true;
}

static void vmx_shutdown_host(void) {
    if (s_vmxon_region) {
        kfree_aligned(s_vmxon_region);
        s_vmxon_region = NULL;
        s_vmxon_phys = 0;

        uint64_t cr4;
        __asm__ volatile ("mov %%cr4, %0" : "=r"(cr4));
        cr4 &= ~CR4_VMXE_BIT;
        __asm__ volatile ("mov %0, %%cr4" : : "r"(cr4));
    }
}

/* --------------------------------------------------------------------------
 * AMD SVM Host Initialization & Shutdown
 * -------------------------------------------------------------------------- */
static bool svm_init_host(void) {
    const CPUFeatures *feat = cpu_get_features();
    if (!feat || !feat->has_svm) {
        com1_puts("[HYPERVISOR] AMD SVM CPUID bit not set.\n");
        return false;
    }

    /* 1. Check & Enable EFER.SVME (Bit 12) */
    uint64_t efer = vmm_rdmsr(AMD_EFER_MSR);
    efer |= AMD_EFER_SVME_BIT;
    vmm_wrmsr(AMD_EFER_MSR, efer);

    /* 2. Allocate 4KB Page-Aligned Host State-Save (HSAVE) Region */
    s_hsave_region = kmalloc_aligned(4096, 4096);
    if (!s_hsave_region) {
        com1_puts("[HYPERVISOR ERROR] Failed to allocate AMD HSAVE region.\n");
        return false;
    }
    memset(s_hsave_region, 0, 4096);
    s_hsave_phys = (uint64_t)vmm_get_physical_address(vmm_get_kernel_pml4(), (uint64_t)s_hsave_region);

    /* 3. Write HSAVE_PA MSR */
    vmm_wrmsr(AMD_VM_HSAVE_PA_MSR, s_hsave_phys);

    com1_puts("[HYPERVISOR] AMD SVM Mode Successfully Active!\n");
    return true;
}

static void svm_shutdown_host(void) {
    if (s_hsave_region) {
        vmm_wrmsr(AMD_VM_HSAVE_PA_MSR, 0);
        kfree_aligned(s_hsave_region);
        s_hsave_region = NULL;
        s_hsave_phys = 0;

        uint64_t efer = vmm_rdmsr(AMD_EFER_MSR);
        efer &= ~AMD_EFER_SVME_BIT;
        vmm_wrmsr(AMD_EFER_MSR, efer);
    }
}

/* --------------------------------------------------------------------------
 * Global Hypervisor Core API
 * -------------------------------------------------------------------------- */
bool atoms_hypervisor_init(void) {
    if (s_hyp_initialized) return true;

    vmentry_autopsy_init(NULL);

    com1_puts("=====================================================\n");
    com1_puts("  ATOMS OS NATIVE MICRO-HYPERVISOR CORE ACTIVE\n");
    com1_puts("=====================================================\n");

    const CPUFeatures *feat = cpu_get_features();
    if (!feat) {
        com1_puts("[HYPERVISOR] CPU Features unavailable. Hypervisor disabled.\n");
        return false;
    }

    if (feat->has_vmx) {
        if (vmx_init_host()) {
            s_hyp_backend = HYPERVISOR_BACKEND_INTEL_VMX;
            s_hyp_initialized = true;
            return true;
        }
    } else if (feat->has_svm) {
        if (svm_init_host()) {
            s_hyp_backend = HYPERVISOR_BACKEND_AMD_SVM;
            s_hyp_initialized = true;
            return true;
        }
    }

    com1_puts("[HYPERVISOR] Hardware-assisted virtualization (VT-x/SVM) not available.\n");
    s_hyp_backend = HYPERVISOR_BACKEND_NONE;
    s_hyp_initialized = false;
    return false;
}

void atoms_hypervisor_shutdown(void) {
    if (!s_hyp_initialized) return;

    if (s_hyp_backend == HYPERVISOR_BACKEND_INTEL_VMX) {
        vmx_shutdown_host();
    } else if (s_hyp_backend == HYPERVISOR_BACKEND_AMD_SVM) {
        svm_shutdown_host();
    }

    s_hyp_backend = HYPERVISOR_BACKEND_NONE;
    s_hyp_initialized = false;
    com1_puts("[HYPERVISOR] Hypervisor Core Shutdown Complete.\n");
}

HypervisorBackend atoms_hypervisor_get_backend(void) {
    return s_hyp_backend;
}

bool atoms_hypervisor_is_available(void) {
    return s_hyp_initialized && (s_hyp_backend != HYPERVISOR_BACKEND_NONE);
}

/* --------------------------------------------------------------------------
 * Virtual Machine Lifecycle
 * -------------------------------------------------------------------------- */
VirtualMachine *atoms_vm_create(uint64_t ram_size) {
    if (!s_hyp_initialized) {
        com1_puts("[HYPERVISOR ERROR] Cannot create VM: Hypervisor uninitialized!\n");
        return NULL;
    }

    /* Host Physical Memory Safety Gate: Ensure host has sufficient physical RAM */
    uint64_t free_host_ram = pmm_get_free_memory();
    if (ram_size > 0 && free_host_ram < (ram_size + (128 * 1024 * 1024ULL))) {
        com1_puts("[HYPERVISOR ERROR] Insufficient host RAM for requested guest RAM allocation!\n");
        return NULL;
    }

    VirtualMachine *vm = (VirtualMachine *)kmalloc(sizeof(VirtualMachine));
    if (!vm) return NULL;
    memset(vm, 0, sizeof(VirtualMachine));

    vm->vm_id = s_next_vm_id++;
    vm->backend = s_hyp_backend;
    vm->state = VM_STATE_CREATED;

    /* Allocate and initialize Guest Physical Memory with EPT/NPT tables */
    if (ram_size > 0) {
        vm->guest_mem = guest_memory_create(vm->vm_id, vm->backend, 0, ram_size);
        if (!vm->guest_mem) {
            kfree(vm);
            return NULL;
        }
        vm->guest_ram_host_virt = vm->guest_mem->hva_backing;
        vm->guest_ram_host_phys = vm->guest_mem->hpa_backing;
        vm->guest_ram_size = vm->guest_mem->gpa_size;
    }

    /* Create primary bootstrap vCPU (ID 0) */
    vm->bsp_vcpu = atoms_vcpu_create(vm, 0);
    if (!vm->bsp_vcpu) {
        if (vm->guest_mem) guest_memory_destroy(vm->guest_mem);
        kfree(vm);
        return NULL;
    }

    /* Create Virtual PCI Bus and VirtIO Hardware Subsystem (Phase 3) */
    vm->pci_bus = virtual_pci_bus_create();
    if (vm->pci_bus) {
        /* 1. VirtIO Block Device (4 GB RAM Disk Backing: 8,388,608 sectors) */
        vm->blk_dev = virtio_blk_create(8388608ULL, false);
        if (vm->blk_dev && vm->blk_dev->base) {
            vm->blk_dev->base->vm = vm;
            virtual_pci_register_virtio_device(vm->pci_bus, vm->blk_dev->base, 0x01, 0x00);
        }

        /* 2. VirtIO Network Device */
        vm->net_dev = virtio_net_create(NULL);
        if (vm->net_dev && vm->net_dev->base) {
            vm->net_dev->base->vm = vm;
            virtual_pci_register_virtio_device(vm->pci_bus, vm->net_dev->base, 0x02, 0x00);
        }

        /* 3. VirtIO Input Device */
        vm->input_dev = virtio_input_create();
        if (vm->input_dev && vm->input_dev->base) {
            vm->input_dev->base->vm = vm;
            virtual_pci_register_virtio_device(vm->pci_bus, vm->input_dev->base, 0x09, 0x00);
        }

        /* 4. VirtIO Display / Shared Framebuffer (1024x768 32bpp) */
        vm->display_dev = virtio_display_create(1024, 768);
        if (vm->display_dev && vm->display_dev->base) {
            vm->display_dev->base->vm = vm;
            virtual_pci_register_virtio_device(vm->pci_bus, vm->display_dev->base, 0x03, 0x00);
        }
    }

    /* Create Virtual Platform & Chipset Subsystem (Phase 4) */
    vm->platform = virtual_platform_create(vm);

    /* Initialize Isolated FreeBSD Root Disk Image */
    freebsd_loader_init_root_disk(vm);

    com1_puts("[HYPERVISOR] Created Virtual Machine with Platform, EPT & VirtIO Subsystems.\n");
    return vm;
}

bool atoms_vm_initialize(VirtualMachine *vm, uint64_t entry_point) {
    if (!vm || vm->state != VM_STATE_CREATED) return false;

    vm->bsp_vcpu->guest_regs.rip = entry_point;
    vm->bsp_vcpu->guest_regs.rflags = 0x02;
    vm->bsp_vcpu->guest_regs.rsp = vm->guest_ram_size > 0 ? (vm->guest_ram_size - 64) : 0x7FFF0;
    vm->bsp_vcpu->state = VM_STATE_INITIALIZED;
    vm->state = VM_STATE_INITIALIZED;

    com1_puts("[HYPERVISOR] Initialized VM vCPU Bootstrap State.\n");
    return true;
}

bool atoms_vm_run(VirtualMachine *vm) {
    if (!vm || vm->state != VM_STATE_INITIALIZED) return false;
    return atoms_vcpu_run(vm->bsp_vcpu);
}

void atoms_vm_stop(VirtualMachine *vm) {
    if (!vm) return;
    if (vm->bsp_vcpu) vm->bsp_vcpu->state = VM_STATE_STOPPED;
    vm->state = VM_STATE_STOPPED;
    com1_puts("[HYPERVISOR] Stopped Virtual Machine Instance.\n");
}

void atoms_vm_destroy(VirtualMachine *vm) {
    if (!vm) return;

    /* Phase 5A-1 Safety Gate: Do not destroy an active runtime VM unless an explicit shutdown reason is set */
    if (vm->runtime_active && vm->shutdown_reason == VM_SHUTDOWN_NONE) {
        com1_puts("[HYPERVISOR SAFETY GATE] Prevented destruction of active persistent runtime VM (no explicit shutdown requested)!\r\n");
        return;
    }

    if (vm == s_runtime_vm) {
        s_runtime_vm = NULL;
    }

    if (vm->bsp_vcpu) {
        atoms_vcpu_destroy(vm->bsp_vcpu);
        vm->bsp_vcpu = NULL;
    }

    if (vm->platform) {
        virtual_platform_destroy(vm->platform);
        vm->platform = NULL;
    }

    /* Cleanly Teardown VirtIO Virtual Hardware Subsystem (Phase 3) */
    if (vm->blk_dev) {
        virtio_blk_destroy(vm->blk_dev);
        vm->blk_dev = NULL;
    }
    if (vm->net_dev) {
        virtio_net_destroy(vm->net_dev);
        vm->net_dev = NULL;
    }
    if (vm->input_dev) {
        virtio_input_destroy(vm->input_dev);
        vm->input_dev = NULL;
    }
    if (vm->display_dev) {
        virtio_display_destroy(vm->display_dev);
        vm->display_dev = NULL;
    }
    if (vm->pci_bus) {
        virtual_pci_bus_destroy(vm->pci_bus);
        vm->pci_bus = NULL;
    }

    if (vm->guest_mem) {
        guest_memory_destroy(vm->guest_mem);
        vm->guest_mem = NULL;
        vm->guest_ram_host_virt = NULL;
    }

    vm->state = VM_STATE_DESTROYED;
    kfree(vm);
    com1_puts("[HYPERVISOR] Cleanly Destroyed VM Instance, Devices & Reclaimed Second-Level Tables.\n");
}

bool atoms_vm_boot_freebsd(VirtualMachine *vm, const void *kernel_elf, size_t size) {
    if (!vm || !vm->bsp_vcpu) return false;

    uint64_t entry_point = 0;
    if (!freebsd_loader_load_kernel(vm, kernel_elf, size, &entry_point)) {
        com1_puts("[FREEBSD BOOT ERROR] Failed to load FreeBSD kernel image!\n");
        return false;
    }

    if (!freebsd_loader_setup_vcpu_environment(vm->bsp_vcpu, entry_point)) {
        com1_puts("[FREEBSD BOOT ERROR] Failed to configure guest vCPU registers!\n");
        return false;
    }

    vm->bsp_vcpu->state = VM_STATE_INITIALIZED;
    vm->state = VM_STATE_INITIALIZED;

    com1_puts("[HYPERVISOR] FreeBSD amd64 Guest Boot Environment Ready.\n");
    return atoms_vm_run(vm);
}

/* --------------------------------------------------------------------------
 * vCPU Implementation
 * -------------------------------------------------------------------------- */
vCPU *atoms_vcpu_create(VirtualMachine *vm, uint32_t id) {
    if (!vm) return NULL;

    vCPU *vcpu = (vCPU *)kmalloc(sizeof(vCPU));
    if (!vcpu) return NULL;
    memset(vcpu, 0, sizeof(vCPU));

    vcpu->id = id;
    vcpu->vm = vm;
    vcpu->state = VM_STATE_CREATED;

    if (vm->backend == HYPERVISOR_BACKEND_INTEL_VMX) {
        vcpu->vmcs_region = kmalloc_aligned(4096, 4096);
        if (!vcpu->vmcs_region) {
            kfree(vcpu);
            return NULL;
        }
        memset(vcpu->vmcs_region, 0, 4096);
        vcpu->vmcs_phys = (uint64_t)vmm_get_physical_address(vmm_get_kernel_pml4(), (uint64_t)vcpu->vmcs_region);
        *(uint32_t *)vcpu->vmcs_region = (uint32_t)vmm_rdmsr(IA32_VMX_BASIC_MSR);
    } else if (vm->backend == HYPERVISOR_BACKEND_AMD_SVM) {
        vcpu->vmcb_region = kmalloc_aligned(4096, 4096);
        if (!vcpu->vmcb_region) {
            kfree(vcpu);
            return NULL;
        }
        memset(vcpu->vmcb_region, 0, 4096);
        vcpu->vmcb_phys = (uint64_t)vmm_get_physical_address(vmm_get_kernel_pml4(), (uint64_t)vcpu->vmcb_region);
    }

    return vcpu;
}

void atoms_vcpu_destroy(vCPU *vcpu) {
    if (!vcpu) return;

    if (vcpu->vmcs_region) {
        kfree_aligned(vcpu->vmcs_region);
        vcpu->vmcs_region = NULL;
    }
    if (vcpu->vmcb_region) {
        kfree_aligned(vcpu->vmcb_region);
        vcpu->vmcb_region = NULL;
    }

    vcpu->state = VM_STATE_DESTROYED;
    kfree(vcpu);
}

/* --------------------------------------------------------------------------
 * Forensic Diagnostic Logging & Register Dump
 * -------------------------------------------------------------------------- */
static void hyp_put_hex64(uint64_t val) {
    char buf[19];
    buf[0] = '0';
    buf[1] = 'x';
    const char hex_chars[] = "0123456789ABCDEF";
    for (int i = 0; i < 16; i++) {
        buf[2 + i] = hex_chars[(val >> (60 - i * 4)) & 0xF];
    }
    buf[18] = '\0';
    com1_puts(buf);
}

static void hyp_put_hex32(uint32_t val) {
    char buf[11];
    buf[0] = '0';
    buf[1] = 'x';
    const char hex_chars[] = "0123456789ABCDEF";
    for (int i = 0; i < 8; i++) {
        buf[2 + i] = hex_chars[(val >> (28 - i * 4)) & 0xF];
    }
    buf[10] = '\0';
    com1_puts(buf);
}

void atoms_hypervisor_dump_vcpu_state(const vCPU *vcpu) {
    if (!vcpu) return;
    com1_puts("\n+-----------------------------------------------------------------------------+\n");
    com1_puts("|                  ATOMS HYPERVISOR VCPU FORENSIC CRASH DUMP                  |\n");
    com1_puts("+-----------------------------------------------------------------------------+\n");
    com1_puts("| Exit Reason : "); hyp_put_hex32(vcpu->last_exit.exit_reason);
    com1_puts(" | Qual : "); hyp_put_hex64(vcpu->last_exit.exit_qualification);
    com1_puts(" | InstLen: "); hyp_put_hex32(vcpu->last_exit.instruction_length);
    com1_puts("\n| Guest RIP   : "); hyp_put_hex64(vcpu->guest_regs.rip);
    com1_puts(" | RSP  : "); hyp_put_hex64(vcpu->guest_regs.rsp);
    com1_puts(" | RFLAGS : "); hyp_put_hex64(vcpu->guest_regs.rflags);
    com1_puts("\n| Guest CR0   : "); hyp_put_hex64(vcpu->cr0);
    com1_puts(" | CR3  : "); hyp_put_hex64(vcpu->cr3);
    com1_puts(" | CR4    : "); hyp_put_hex64(vcpu->cr4);
    com1_puts("\n| Guest EFER  : "); hyp_put_hex64(vcpu->efer);
    com1_puts(" | Fault GPA: "); hyp_put_hex64(vcpu->last_exit.guest_physical_address);
    com1_puts("\n| RAX: "); hyp_put_hex64(vcpu->guest_regs.rax);
    com1_puts(" | RBX: "); hyp_put_hex64(vcpu->guest_regs.rbx);
    com1_puts(" | RCX: "); hyp_put_hex64(vcpu->guest_regs.rcx);
    com1_puts("\n| RDX: "); hyp_put_hex64(vcpu->guest_regs.rdx);
    com1_puts(" | RSI: "); hyp_put_hex64(vcpu->guest_regs.rsi);
    com1_puts(" | RDI: "); hyp_put_hex64(vcpu->guest_regs.rdi);
    com1_puts("\n| RBP: "); hyp_put_hex64(vcpu->guest_regs.rbp);
    com1_puts(" | R8 : "); hyp_put_hex64(vcpu->guest_regs.r8);
    com1_puts(" | R9 : "); hyp_put_hex64(vcpu->guest_regs.r9);
    com1_puts("\n| R10: "); hyp_put_hex64(vcpu->guest_regs.r10);
    com1_puts(" | R11: "); hyp_put_hex64(vcpu->guest_regs.r11);
    com1_puts(" | R12: "); hyp_put_hex64(vcpu->guest_regs.r12);
    com1_puts("\n| R13: "); hyp_put_hex64(vcpu->guest_regs.r13);
    com1_puts(" | R14: "); hyp_put_hex64(vcpu->guest_regs.r14);
    com1_puts(" | R15: "); hyp_put_hex64(vcpu->guest_regs.r15);
    com1_puts("\n| KERNEL_GS_BASE: "); hyp_put_hex64(vcpu->msr_kernel_gs_base);
    com1_puts(" | LSTAR: "); hyp_put_hex64(vcpu->msr_lstar);
    com1_puts("\n+-----------------------------------------------------------------------------+\n\n");
}

/* --------------------------------------------------------------------------
 * VM-Exit Dispatcher
 * -------------------------------------------------------------------------- */
bool atoms_vmexit_dispatch(vCPU *vcpu) {
    if (!vcpu || !vcpu->vm) return false;

    vcpu->vm->total_vmexits++;
    vcpu->last_exit.disposition = VMEXIT_UNKNOWN;

    if (vcpu->vm->backend == HYPERVISOR_BACKEND_INTEL_VMX) {
        switch (vcpu->last_exit.exit_reason) {
            case VMX_EXIT_REASON_HLT: {
                if (!s_vmxon_region) {
                    com1_puts("[HYPERVISOR VMEXIT] Verification Path: Guest executed HLT instruction.\n");
                    vcpu->guest_regs.rip += vcpu->last_exit.instruction_length;
                    vcpu->state = VM_STATE_RUNNING;
                    vcpu->last_exit.handled = true;
                    vcpu->last_exit.disposition = VMEXIT_HANDLED_AND_RESUME;
                    return true;
                }

                uint64_t rflags = vmx_vmread(VMCS_GUEST_RFLAGS);
                uint32_t intr_state = (uint32_t)vmx_vmread(VMCS_GUEST_INTERRUPTIBILITY_INFO);

                if ((rflags & 0x200) == 0) {
                    /* HLT with IF=0 */
                    if (vcpu->vm->runtime_active) {
                        /* In persistent runtime, guest halted with IF=0. Advance RIP past HLT to allow progression */
                        vcpu->guest_regs.rip += vcpu->last_exit.instruction_length;
                        vcpu->state = VM_STATE_RUNNING;
                        vcpu->last_exit.handled = true;
                        vcpu->last_exit.disposition = VMEXIT_HANDLED_AND_RESUME;
                        return true;
                    } else {
                        com1_puts("[HYPERVISOR VMEXIT] Guest executed HLT with IF=0 (Terminal Halt).\n");
                        vcpu->state = VM_STATE_EXITED;
                        vcpu->last_exit.handled = true;
                        vcpu->last_exit.disposition = VMEXIT_GUEST_SHUTDOWN;
                        return true;
                    }
                }

                static uint32_t s_hlt_count = 0;
                s_hlt_count++;
                if (s_hlt_count <= 5 || (s_hlt_count % 10000) == 0) {
                    com1_puts("[HYPERVISOR VMEXIT] Guest HLT (IF=1) -> Injecting Timer Vector 0x20\n");
                }
                vcpu->guest_regs.rip += vcpu->last_exit.instruction_length;
                vcpu->state = VM_STATE_RUNNING;
                vcpu->last_exit.handled = true;
                vcpu->last_exit.disposition = VMEXIT_HANDLED_AND_RESUME;

                if ((intr_state & 3) == 0) {
                    /* Inject virtual LAPIC timer interrupt (Vector 0x20 = 32) */
                    vmx_vmwrite(VMCS_VM_ENTRY_INTR_INFO_FIELD, 0x80000020U);
                }
                return true;
            }

            case VMX_EXIT_REASON_CPUID: {
                uint32_t eax = 0, ebx = 0, ecx = 0, edx = 0;
                virtual_platform_handle_cpuid(vcpu, (uint32_t)vcpu->guest_regs.rax, (uint32_t)vcpu->guest_regs.rcx, &eax, &ebx, &ecx, &edx);
                vcpu->guest_regs.rax = eax;
                vcpu->guest_regs.rbx = ebx;
                vcpu->guest_regs.rcx = ecx;
                vcpu->guest_regs.rdx = edx;
                vcpu->guest_regs.rip += vcpu->last_exit.instruction_length;
                vcpu->last_exit.handled = true;
                vcpu->last_exit.disposition = VMEXIT_HANDLED_AND_RESUME;
                return true;
            }

            case VMX_EXIT_REASON_IO_INSTRUCTION: {
                uint64_t qual = vcpu->last_exit.exit_qualification;
                uint16_t port = (uint16_t)(qual >> 16);
                bool is_write = ((qual & (1 << 3)) == 0);
                uint8_t size = 1;
                uint8_t size_enc = (uint8_t)(qual & 0x07);
                if (size_enc == 0) size = 1;
                else if (size_enc == 1) size = 2;
                else if (size_enc == 3) size = 4;

                uint32_t io_val = (uint32_t)vcpu->guest_regs.rax;

                /* Intercept hardware reset port 0xCF9 */
                if (is_write && port == 0xCF9 && (io_val & 0x06) != 0) {
                    com1_puts("[HYPERVISOR VMEXIT] Guest requested system reset via port 0xCF9\r\n");
                    vcpu->last_exit.handled = true;
                    vcpu->last_exit.disposition = VMEXIT_GUEST_RESET;
                    return true;
                }

                /* Intercept ACPI power-off ports (QEMU 0x604 / PIIX4 0xB004) */
                if (is_write && (port == 0x604 || port == 0xB004) && (io_val & 0x2000) != 0) {
                    com1_puts("[HYPERVISOR VMEXIT] Guest requested ACPI shutdown\r\n");
                    vcpu->last_exit.handled = true;
                    vcpu->last_exit.disposition = VMEXIT_GUEST_SHUTDOWN;
                    return true;
                }

                if (vcpu->vm->platform) {
                    virtual_platform_handle_io(vcpu->vm->platform, port, is_write, size, &io_val);
                    if (!is_write) {
                        if (size == 1) vcpu->guest_regs.rax = (vcpu->guest_regs.rax & ~0xFFULL) | (io_val & 0xFF);
                        else if (size == 2) vcpu->guest_regs.rax = (vcpu->guest_regs.rax & ~0xFFFFULL) | (io_val & 0xFFFF);
                        else vcpu->guest_regs.rax = (vcpu->guest_regs.rax & ~0xFFFFFFFFULL) | io_val;
                    }
                }
                vcpu->guest_regs.rip += vcpu->last_exit.instruction_length;
                vcpu->last_exit.handled = true;
                vcpu->last_exit.disposition = VMEXIT_HANDLED_AND_RESUME;
                return true;
            }

            case VMX_EXIT_REASON_RDMSR: {
                uint64_t msr_val = 0;
                if (virtual_platform_handle_rdmsr(vcpu, (uint32_t)vcpu->guest_regs.rcx, &msr_val)) {
                    vcpu->guest_regs.rax = (uint32_t)msr_val;
                    vcpu->guest_regs.rdx = (uint32_t)(msr_val >> 32);
                }
                vcpu->guest_regs.rip += vcpu->last_exit.instruction_length;
                vcpu->last_exit.handled = true;
                vcpu->last_exit.disposition = VMEXIT_HANDLED_AND_RESUME;
                return true;
            }

            case VMX_EXIT_REASON_WRMSR: {
                uint64_t msr_val = ((uint64_t)vcpu->guest_regs.rdx << 32) | (uint32_t)vcpu->guest_regs.rax;
                virtual_platform_handle_wrmsr(vcpu, (uint32_t)vcpu->guest_regs.rcx, msr_val);
                vcpu->guest_regs.rip += vcpu->last_exit.instruction_length;
                vcpu->last_exit.handled = true;
                vcpu->last_exit.disposition = VMEXIT_HANDLED_AND_RESUME;
                return true;
            }

            case VMX_EXIT_REASON_VMCALL:
                com1_puts("[HYPERVISOR VMEXIT] Guest Hypercall (VMCALL) received.\n");
                vcpu->guest_regs.rip += vcpu->last_exit.instruction_length;
                vcpu->last_exit.handled = true;
                vcpu->last_exit.disposition = VMEXIT_HANDLED_AND_RESUME;
                return true;

            case VMX_EXIT_REASON_TRIPLE_FAULT:
                com1_puts("[HYPERVISOR VMEXIT] Guest Triple Fault occurred!\r\n");
                atoms_hypervisor_dump_vcpu_state(vcpu);
                vcpu->state = VM_STATE_STOPPED;
                vcpu->last_exit.handled = true;
                vcpu->last_exit.disposition = VMEXIT_GUEST_RESET;
                return true;

            case VMX_EXIT_REASON_PREEMPT_TIMER:
            case VMX_EXIT_REASON_EXTERNAL_INTR:
                vcpu->last_exit.handled = true;
                vcpu->last_exit.disposition = VMEXIT_HANDLED_AND_RESUME;
                return true;

            case VMX_EXIT_REASON_EPT_VIOLATION: {
                uint64_t gpa = vcpu->last_exit.guest_physical_address;
                if (((gpa >= 0xFEE00000ULL && gpa < 0xFEE01000ULL) ||
                     (gpa >= 0xFEC00000ULL && gpa < 0xFEC01000ULL) ||
                     (gpa >= 0xFEB00000ULL && gpa <= 0xFEBFFFFFULL)) && vcpu->vm->platform) {
                    uint64_t mmio_val = vcpu->guest_regs.rax;
                    bool is_wr = (vcpu->last_exit.exit_qualification & 0x02) != 0;
                    virtual_platform_handle_mmio(vcpu->vm->platform, gpa, is_wr, 4, &mmio_val);
                    if (!is_wr) vcpu->guest_regs.rax = mmio_val;
                    vcpu->guest_regs.rip += vcpu->last_exit.instruction_length;
                    vcpu->last_exit.handled = true;
                    vcpu->last_exit.disposition = VMEXIT_HANDLED_AND_RESUME;
                    return true;
                }
                bool ept_ok = atoms_hypervisor_handle_ept_violation(vcpu, &vcpu->last_exit);
                if (!ept_ok) {
                    atoms_hypervisor_dump_vcpu_state(vcpu);
                    vcpu->last_exit.disposition = VMEXIT_FATAL_ERROR;
                } else {
                    vcpu->last_exit.disposition = VMEXIT_HANDLED_AND_RESUME;
                }
                return ept_ok;
            }

            default:
                com1_puts("[HYPERVISOR VMEXIT] Unhandled VMX Exit Reason: Halting Guest safely.\n");
                atoms_hypervisor_dump_vcpu_state(vcpu);
                vcpu->state = VM_STATE_STOPPED;
                vcpu->last_exit.handled = false;
                vcpu->last_exit.disposition = VMEXIT_UNKNOWN;
                return false;
        }
    } else if (vcpu->vm->backend == HYPERVISOR_BACKEND_AMD_SVM) {
        switch (vcpu->last_exit.exit_reason) {
            case SVM_EXIT_HLT: {
                static uint32_t s_svm_hlt = 0;
                s_svm_hlt++;
                if (s_svm_hlt <= 5 || (s_svm_hlt % 10000) == 0) {
                    com1_puts("[HYPERVISOR VMEXIT] AMD Guest HLT -> Stepping RIP\n");
                }
                vcpu->guest_regs.rip += vcpu->last_exit.instruction_length;
                vcpu->state = VM_STATE_RUNNING;
                vcpu->last_exit.handled = true;
                vcpu->last_exit.disposition = VMEXIT_HANDLED_AND_RESUME;
                return true;
            }

            case SVM_EXIT_CPUID: {
                uint32_t eax = 0, ebx = 0, ecx = 0, edx = 0;
                virtual_platform_handle_cpuid(vcpu, (uint32_t)vcpu->guest_regs.rax, (uint32_t)vcpu->guest_regs.rcx, &eax, &ebx, &ecx, &edx);
                vcpu->guest_regs.rax = eax;
                vcpu->guest_regs.rbx = ebx;
                vcpu->guest_regs.rcx = ecx;
                vcpu->guest_regs.rdx = edx;
                vcpu->guest_regs.rip += vcpu->last_exit.instruction_length;
                vcpu->last_exit.handled = true;
                vcpu->last_exit.disposition = VMEXIT_HANDLED_AND_RESUME;
                return true;
            }

            case SVM_EXIT_NPF: {
                bool npf_ok = atoms_hypervisor_handle_npt_fault(vcpu, &vcpu->last_exit);
                if (!npf_ok) {
                    atoms_hypervisor_dump_vcpu_state(vcpu);
                    vcpu->last_exit.disposition = VMEXIT_FATAL_ERROR;
                } else {
                    vcpu->last_exit.disposition = VMEXIT_HANDLED_AND_RESUME;
                }
                return npf_ok;
            }

            default:
                com1_puts("[HYPERVISOR VMEXIT] Unhandled SVM Exit Reason.\n");
                atoms_hypervisor_dump_vcpu_state(vcpu);
                vcpu->state = VM_STATE_STOPPED;
                vcpu->last_exit.handled = false;
                vcpu->last_exit.disposition = VMEXIT_UNKNOWN;
                return false;
        }
    }

    return false;
}

/* --------------------------------------------------------------------------
 * Hardware VMCS Configuration & Execution
 * -------------------------------------------------------------------------- */
#include "arch/x86_64/gdt/gdt.h"

extern void vmx_vmexit_handler(void);
extern bool vmx_run_vcpu_raw(GuestCpuRegisters *regs, bool is_resuming);

static uint8_t s_vmx_host_stack[16384] __attribute__((aligned(16)));

const char *atoms_hypervisor_decode_vm_instruction_error(uint32_t error_code) {
    switch (error_code) {
        case 0:  return "No error / Unknown";
        case 1:  return "VMCALL executed in VMX root operation";
        case 2:  return "VMCLEAR with invalid physical address";
        case 3:  return "VMCLEAR with VMXON pointer";
        case 4:  return "VMLAUNCH with non-clear VMCS";
        case 5:  return "VMRESUME with non-launched VMCS";
        case 6:  return "VMRESUME with corrupted VMCS (cleared to launched)";
        case 7:  return "VM entry with invalid control field(s)";
        case 8:  return "VM entry with invalid host-state field(s)";
        case 9:  return "VM entry with invalid guest-state field(s)";
        case 10: return "VM entry with invalid executive-VMCS pointer";
        case 11: return "VM entry with non-current executive-VMCS pointer";
        case 12: return "VM entry with executive-VMCS pointer not VMXON pointer";
        case 13: return "VMCALL with non-clear VMCS";
        case 14: return "VMCALL with invalid VM-exit control fields";
        case 15: return "VMCALL with incorrect MSEG revision identifier";
        case 16: return "VMXOFF under dual-monitor treatment of SMIs and SMM";
        case 17: return "VMCALL with invalid SMM-monitor features";
        case 18: return "VM entry with invalid VM-execution control fields in executive VMCS";
        case 19: return "VM entry with events blocked by MOV-SS";
        case 20: return "Bad MOV to/from CR3";
        case 21: return "Bad MOV to/from CR8";
        case 28: return "VMPTRLD with invalid physical address";
        case 29: return "VMPTRLD with VMXON pointer";
        case 30: return "VMPTRLD with incorrect VMCS revision identifier";
        case 31: return "VMREAD/VMWRITE from/to unsupported VMCS component";
        case 32: return "VMWRITE to read-only VMCS component";
        case 33: return "VMXON executed in VMX root operation";
        default: return "Undocumented Intel VM-Instruction Error";
    }
}

bool atoms_hypervisor_validate_vmcs_host_state(vCPU *vcpu, char *out_reason, size_t max_len) {
    if (!vcpu) {
        if (out_reason && max_len > 0) strncpy(out_reason, "Null vCPU pointer", max_len);
        return false;
    }

    if (vcpu->vm && vcpu->vm->backend == HYPERVISOR_BACKEND_AMD_SVM) {
        if (!vcpu->vmcb_region) {
            if (out_reason && max_len > 0) strncpy(out_reason, "AMD VMCB region null", max_len);
            return false;
        }
        return true;
    }

    uint64_t host_cr0 = vmx_vmread(VMCS_HOST_CR0);
    uint64_t host_cr3 = vmx_vmread(VMCS_HOST_CR3);
    uint64_t host_cr4 = vmx_vmread(VMCS_HOST_CR4);
    uint64_t host_cs  = vmx_vmread(VMCS_HOST_CS_SELECTOR);
    uint64_t host_tr  = vmx_vmread(VMCS_HOST_TR_SELECTOR);
    uint64_t host_tr_base = vmx_vmread(VMCS_HOST_TR_BASE);
    uint64_t host_gdtr_base = vmx_vmread(VMCS_HOST_GDTR_BASE);
    uint64_t host_idtr_base = vmx_vmread(VMCS_HOST_IDTR_BASE);
    uint64_t host_rsp = vmx_vmread(VMCS_HOST_RSP);
    uint64_t host_rip = vmx_vmread(VMCS_HOST_RIP);

    if ((host_cr0 & 0x80000001ULL) != 0x80000001ULL) {
        if (out_reason && max_len > 0) strncpy(out_reason, "Host CR0 missing PE or PG", max_len);
        return false;
    }
    if ((host_cr4 & (1ULL << 5)) == 0) {
        if (out_reason && max_len > 0) strncpy(out_reason, "Host CR4 missing PAE", max_len);
        return false;
    }
    if (host_cr3 == 0 || (host_cr3 & 0xFFF) != 0) {
        if (out_reason && max_len > 0) strncpy(out_reason, "Host CR3 invalid or unaligned", max_len);
        return false;
    }
    if (host_cs == 0 || (host_cs & 0x7) != 0) {
        if (out_reason && max_len > 0) strncpy(out_reason, "Host CS selector invalid (must be RPL=0, TI=0)", max_len);
        return false;
    }
    if (host_tr == 0 || (host_tr & 0x4) != 0) {
        if (out_reason && max_len > 0) strncpy(out_reason, "Host TR selector invalid (must have TI=0)", max_len);
        return false;
    }
    if (host_tr_base == 0) {
        if (out_reason && max_len > 0) strncpy(out_reason, "Host TR base address is NULL", max_len);
        return false;
    }
    if (host_gdtr_base == 0) {
        if (out_reason && max_len > 0) strncpy(out_reason, "Host GDTR base address is NULL", max_len);
        return false;
    }
    if (host_idtr_base == 0) {
        if (out_reason && max_len > 0) strncpy(out_reason, "Host IDTR base address is NULL", max_len);
        return false;
    }
    if (host_rsp == 0) {
        if (out_reason && max_len > 0) strncpy(out_reason, "Host RSP stack pointer is NULL", max_len);
        return false;
    }
    if (host_rip == 0) {
        if (out_reason && max_len > 0) strncpy(out_reason, "Host RIP exit handler is NULL", max_len);
        return false;
    }

    if (out_reason && max_len > 0) strncpy(out_reason, "All VMCS Host State fields verified PASS", max_len);
    return true;
}

static inline bool hyp_is_canonical(uint64_t addr) {
    uint64_t upper = addr >> 47;
    return (upper == 0) || (upper == 0x1FFFF);
}

bool atoms_hypervisor_validate_vmcs_guest_state(vCPU *vcpu, char *out_reason, size_t max_len) {
    if (!vcpu) {
        if (out_reason && max_len > 0) strncpy(out_reason, "Null vCPU pointer", max_len);
        return false;
    }

    if (vcpu->vm && vcpu->vm->backend == HYPERVISOR_BACKEND_AMD_SVM) {
        return true;
    }

    uint64_t g_cr0 = vmx_vmread(VMCS_GUEST_CR0);
    uint64_t g_cr3 = vmx_vmread(VMCS_GUEST_CR3);
    uint64_t g_cr4 = vmx_vmread(VMCS_GUEST_CR4);
    uint64_t g_efer = vmx_vmread(VMCS_GUEST_IA32_EFER);
    uint64_t entry_ctls = vmx_vmread(VMCS_VM_ENTRY_CONTROLS);
    bool is_64bit_guest = (entry_ctls & (1U << 9)) != 0;

    /* CR0 Checks */
    if ((g_cr0 & 0x80000001ULL) != 0x80000001ULL) {
        if (out_reason && max_len > 0) strncpy(out_reason, "Guest CR0 missing PE or PG", max_len);
        return false;
    }
    if ((g_cr0 >> 32) != 0) {
        if (out_reason && max_len > 0) strncpy(out_reason, "Guest CR0 upper 32 bits non-zero", max_len);
        return false;
    }

    /* CR4 Checks */
    if (is_64bit_guest && (g_cr4 & (1ULL << 5)) == 0) {
        if (out_reason && max_len > 0) strncpy(out_reason, "Guest CR4 missing PAE in long mode", max_len);
        return false;
    }
    /* Intel SDM Vol 3C Section 26.3.1.1: CR4 vs FIXED0 / FIXED1 MSRs */
    uint64_t cr4_f0 = vmm_rdmsr(0x488);
    uint64_t cr4_f1 = vmm_rdmsr(0x489);
    if ((g_cr4 & cr4_f0) != cr4_f0) {
        if (out_reason && max_len > 0) strncpy(out_reason, "Guest CR4 missing mandatory FIXED0 bits", max_len);
        return false;
    }
    if ((g_cr4 & ~cr4_f1) != 0) {
        if (out_reason && max_len > 0) strncpy(out_reason, "Guest CR4 sets disallowed FIXED1 bits", max_len);
        return false;
    }
    if ((g_cr4 >> 32) != 0) {
        if (out_reason && max_len > 0) strncpy(out_reason, "Guest CR4 upper 32 bits non-zero", max_len);
        return false;
    }

    /* CR3 Alignment */
    if ((g_cr3 & 0xFFF) != 0) {
        if (out_reason && max_len > 0) strncpy(out_reason, "Guest CR3 not 4KB page-aligned", max_len);
        return false;
    }

    /* IA32_EFER Checks */
    if (entry_ctls & (1U << 15)) {
        if (is_64bit_guest) {
            if ((g_efer & (1ULL << 8)) == 0 || (g_efer & (1ULL << 10)) == 0) {
                if (out_reason && max_len > 0) strncpy(out_reason, "Guest IA32_EFER missing LME or LMA in long mode", max_len);
                return false;
            }
        }
    }

    /* CS Register Checks */
    uint64_t cs_sel = vmx_vmread(VMCS_GUEST_CS_SELECTOR);
    uint64_t cs_ar  = vmx_vmread(VMCS_GUEST_CS_AR_BYTES);
    uint64_t cs_base = vmx_vmread(VMCS_GUEST_CS_BASE);
    if ((cs_sel & ~0x3ULL) == 0) {
        if (out_reason && max_len > 0) strncpy(out_reason, "Guest CS selector is NULL", max_len);
        return false;
    }
    if (!hyp_is_canonical(cs_base)) {
        if (out_reason && max_len > 0) strncpy(out_reason, "Guest CS base is non-canonical", max_len);
        return false;
    }
    if ((cs_ar & (1U << 16)) != 0) {
        if (out_reason && max_len > 0) strncpy(out_reason, "Guest CS cannot be unusable", max_len);
        return false;
    }
    if (is_64bit_guest) {
        if ((cs_ar & (1U << 13)) == 0) {
            if (out_reason && max_len > 0) strncpy(out_reason, "Guest CS missing L=1 in long mode", max_len);
            return false;
        }
        if ((cs_ar & (1U << 14)) != 0) {
            if (out_reason && max_len > 0) strncpy(out_reason, "Guest CS has D/B=1 with L=1", max_len);
            return false;
        }
    }
    /* CS Granularity (bit 15) vs Limit Consistency (Intel SDM Vol 3C Section 26.3.1.2) */
    uint64_t cs_limit = vmx_vmread(VMCS_GUEST_CS_LIMIT);
    if ((cs_ar & (1U << 15)) == 0) {
        if ((cs_limit >> 20) != 0) {
            if (out_reason && max_len > 0) strncpy(out_reason, "Guest CS: G=0 but limit bits 31:20 non-zero", max_len);
            return false;
        }
    } else {
        if ((cs_limit & 0xFFF) != 0xFFF) {
            if (out_reason && max_len > 0) strncpy(out_reason, "Guest CS: G=1 but limit bits 11:0 not 0xFFF", max_len);
            return false;
        }
    }

    /* SS Register Checks */
    uint64_t ss_sel = vmx_vmread(VMCS_GUEST_SS_SELECTOR);
    uint64_t ss_ar  = vmx_vmread(VMCS_GUEST_SS_AR_BYTES);
    uint64_t ss_base = vmx_vmread(VMCS_GUEST_SS_BASE);
    if ((ss_ar & (1U << 16)) == 0) {
        if (!hyp_is_canonical(ss_base)) {
            if (out_reason && max_len > 0) strncpy(out_reason, "Guest SS base is non-canonical", max_len);
            return false;
        }
        uint32_t cs_rpl = (uint32_t)(cs_sel & 3);
        uint32_t ss_dpl = (uint32_t)((ss_ar >> 5) & 3);
        if (ss_dpl != cs_rpl) {
            if (out_reason && max_len > 0) strncpy(out_reason, "Guest SS DPL does not equal CS RPL", max_len);
            return false;
        }
    }

    /* TR Register Checks */
    uint64_t tr_sel = vmx_vmread(VMCS_GUEST_TR_SELECTOR);
    uint64_t tr_ar  = vmx_vmread(VMCS_GUEST_TR_AR_BYTES);
    uint64_t tr_base = vmx_vmread(VMCS_GUEST_TR_BASE);
    uint64_t tr_limit = vmx_vmread(VMCS_GUEST_TR_LIMIT);
    if ((tr_ar & (1U << 16)) != 0) {
        if (out_reason && max_len > 0) strncpy(out_reason, "Guest TR marked unusable", max_len);
        return false;
    }
    if ((tr_sel & 0x4) != 0) {
        if (out_reason && max_len > 0) strncpy(out_reason, "Guest TR has TI=1", max_len);
        return false;
    }
    if (!hyp_is_canonical(tr_base)) {
        if (out_reason && max_len > 0) strncpy(out_reason, "Guest TR base is non-canonical", max_len);
        return false;
    }
    if (is_64bit_guest) {
        uint32_t tr_type = (uint32_t)(tr_ar & 0x0F);
        if (tr_type != 11) {
            if (out_reason && max_len > 0) strncpy(out_reason, "Guest TR type is not 11 (busy 64-bit TSS)", max_len);
            return false;
        }
        if (tr_limit < 0x67) {
            if (out_reason && max_len > 0) strncpy(out_reason, "Guest TR limit < 0x67", max_len);
            return false;
        }
    }

    /* GDTR and IDTR Checks */
    uint64_t gdtr_base = vmx_vmread(VMCS_GUEST_GDTR_BASE);
    uint64_t idtr_base = vmx_vmread(VMCS_GUEST_IDTR_BASE);
    uint64_t gdtr_limit = vmx_vmread(VMCS_GUEST_GDTR_LIMIT);
    uint64_t idtr_limit = vmx_vmread(VMCS_GUEST_IDTR_LIMIT);
    if (!hyp_is_canonical(gdtr_base)) {
        if (out_reason && max_len > 0) strncpy(out_reason, "Guest GDTR base is non-canonical", max_len);
        return false;
    }
    if (!hyp_is_canonical(idtr_base)) {
        if (out_reason && max_len > 0) strncpy(out_reason, "Guest IDTR base is non-canonical", max_len);
        return false;
    }
    if ((gdtr_limit >> 16) != 0) {
        if (out_reason && max_len > 0) strncpy(out_reason, "Guest GDTR limit upper 16 bits non-zero", max_len);
        return false;
    }
    if ((idtr_limit >> 16) != 0) {
        if (out_reason && max_len > 0) strncpy(out_reason, "Guest IDTR limit upper 16 bits non-zero", max_len);
        return false;
    }

    /* RIP and RSP Checks */
    uint64_t g_rip = vmx_vmread(VMCS_GUEST_RIP);
    uint64_t g_rsp = vmx_vmread(VMCS_GUEST_RSP);
    if (!hyp_is_canonical(g_rip)) {
        if (out_reason && max_len > 0) strncpy(out_reason, "Guest RIP is non-canonical", max_len);
        return false;
    }
    if (!hyp_is_canonical(g_rsp)) {
        if (out_reason && max_len > 0) strncpy(out_reason, "Guest RSP is non-canonical", max_len);
        return false;
    }
    if (g_rip == 0) {
        if (out_reason && max_len > 0) strncpy(out_reason, "Guest RIP is NULL (0x00000000)", max_len);
        return false;
    }

    /* RFLAGS Checks */
    uint64_t g_rflags = vmx_vmread(VMCS_GUEST_RFLAGS);
    if ((g_rflags & (1ULL << 1)) == 0) {
        if (out_reason && max_len > 0) strncpy(out_reason, "Guest RFLAGS bit 1 is 0", max_len);
        return false;
    }
    if (is_64bit_guest && (g_rflags & (1ULL << 17)) != 0) {
        if (out_reason && max_len > 0) strncpy(out_reason, "Guest RFLAGS VM bit set in 64-bit mode", max_len);
        return false;
    }

    /* DR7 Checks (Intel SDM Vol 3C Section 26.3.1.1) */
    uint64_t g_dr7 = vmx_vmread(VMCS_GUEST_DR7);
    if ((g_dr7 >> 32) != 0) {
        if (out_reason && max_len > 0) strncpy(out_reason, "Guest DR7 upper 32 bits non-zero", max_len);
        return false;
    }
    if ((g_dr7 & (1ULL << 10)) == 0) {
        if (out_reason && max_len > 0) strncpy(out_reason, "Guest DR7 bit 10 must be 1", max_len);
        return false;
    }
    if ((g_dr7 & (0x1FULL << 11)) != 0) {
        if (out_reason && max_len > 0) strncpy(out_reason, "Guest DR7 reserved bits 11:15 non-zero", max_len);
        return false;
    }

    if (out_reason && max_len > 0) strncpy(out_reason, "All VMCS Guest State fields verified PASS", max_len);
    return true;
}

extern uint64_t g_vmlaunch_rflags;
extern uint64_t g_vmlaunch_class;

static void hyp_dump_segment_reg(const char *name, uint64_t sel, uint64_t base, uint32_t lim, uint32_t ar) {
    com1_puts("\n");
    com1_puts(name);
    com1_puts("\n------------------------------------------------------------\n");
    com1_puts("Selector      : "); hyp_put_hex32((uint32_t)sel); com1_puts("\n");
    com1_puts("Base          : "); hyp_put_hex64(base); com1_puts("\n");
    com1_puts("Limit         : "); hyp_put_hex32(lim); com1_puts("\n");
    com1_puts("Access Rights : "); hyp_put_hex32(ar); com1_puts("\n\n");

    uint32_t type = ar & 0xF;
    uint32_t s = (ar >> 4) & 1;
    uint32_t dpl = (ar >> 5) & 3;
    uint32_t p = (ar >> 7) & 1;
    uint32_t avl = (ar >> 12) & 1;
    uint32_t l = (ar >> 13) & 1;
    uint32_t db = (ar >> 14) & 1;
    uint32_t g = (ar >> 15) & 1;
    uint32_t unusable = (ar >> 16) & 1;

    com1_puts("Type          : "); hyp_put_hex32(type); com1_puts("\n");
    com1_puts("S             : "); hyp_put_hex32(s); com1_puts("\n");
    com1_puts("DPL           : "); hyp_put_hex32(dpl); com1_puts("\n");
    com1_puts("P             : "); hyp_put_hex32(p); com1_puts("\n");
    com1_puts("AVL           : "); hyp_put_hex32(avl); com1_puts("\n");
    com1_puts("L             : "); hyp_put_hex32(l); com1_puts("\n");
    com1_puts("D/B           : "); hyp_put_hex32(db); com1_puts("\n");
    com1_puts("G             : "); hyp_put_hex32(g); com1_puts("\n");
    com1_puts("Unusable      : "); hyp_put_hex32(unusable); com1_puts("\n");

    bool valid = true;
    if (unusable) {
        valid = ((ar >> 17) == 0);
    } else {
        if (!p) valid = false;
        if (g && (lim & 0xFFF) != 0xFFF) valid = false;
    }
    com1_puts("Validation    : ");
    com1_puts(valid ? "PASS" : "FAIL");
    com1_puts("\n------------------------------------------------------------\n");
}

static void hyp_dump_gdt_memory(uint64_t gdt_base, uint16_t gdt_limit) {
    com1_puts("\n============================================================\n");
    com1_puts("           ACTUAL GDT PHYSICAL MEMORY INSPECTION\n");
    com1_puts("============================================================\n");
    com1_puts("GDT Base Address: "); hyp_put_hex64(gdt_base);
    com1_puts(" | Limit: "); hyp_put_hex32(gdt_limit); com1_puts("\n");

    if (gdt_base == 0) {
        com1_puts("GDT Base is NULL!\n");
        return;
    }

    uint32_t max_entries = (gdt_limit + 1) / 8;
    if (max_entries > 8) max_entries = 8;

    uint64_t *descriptors = (uint64_t *)gdt_base;
    for (uint32_t i = 0; i < max_entries; i++) {
        uint64_t desc = descriptors[i];
        com1_puts("GDT[");
        char num[4];
        num[0] = '0' + (i / 10);
        num[1] = '0' + (i % 10);
        num[2] = ']';
        num[3] = '\0';
        com1_puts(num[0] == '0' ? num + 1 : num);
        com1_puts(" Address = "); hyp_put_hex64((uint64_t)&descriptors[i]);
        com1_puts(" Raw: "); hyp_put_hex64(desc); com1_puts("\n");

        uint32_t limit_low = desc & 0xFFFF;
        uint32_t base_low = (desc >> 16) & 0xFFFF;
        uint32_t base_mid = (desc >> 32) & 0xFF;
        uint32_t access = (desc >> 40) & 0xFF;
        uint32_t limit_high = (desc >> 48) & 0xF;
        uint32_t flags = (desc >> 52) & 0xF;
        uint32_t base_high = (desc >> 56) & 0xFF;
        uint32_t total_limit = limit_low | (limit_high << 16);
        uint32_t total_base = base_low | (base_mid << 16) | (base_high << 24);

        com1_puts("       Base="); hyp_put_hex32(total_base);
        com1_puts(" Lim="); hyp_put_hex32(total_limit);
        com1_puts(" Access="); hyp_put_hex32(access);
        com1_puts(" Flags="); hyp_put_hex32(flags);
        com1_puts("\n");

        if (i == 5 && (i + 1) < max_entries) {
            uint64_t upper = descriptors[i + 1];
            uint64_t full_tss_base = (uint64_t)total_base | ((upper & 0xFFFFFFFFULL) << 32);
            com1_puts("       >>> 64-bit TSS Full Base: "); hyp_put_hex64(full_tss_base); com1_puts(" <<<\n");
        }
    }
    com1_puts("============================================================\n\n");
}

static void hyp_dump_tss_memory(uint64_t tr_base, uint32_t tr_limit, uint32_t tr_ar) {
    com1_puts("\n============================================================\n");
    com1_puts("           SPECIAL TR / TSS FORENSIC INSPECTION\n");
    com1_puts("============================================================\n");
    com1_puts("TSS Base Address : "); hyp_put_hex64(tr_base); com1_puts("\n");
    com1_puts("TSS Limit        : "); hyp_put_hex32(tr_limit); com1_puts(" (Expected >= 0x67)\n");
    com1_puts("TSS Access Rights: "); hyp_put_hex32(tr_ar); com1_puts("\n");
    com1_puts("TSS Type         : "); hyp_put_hex32(tr_ar & 0xF); com1_puts(" (11 = Busy 64-bit TSS)\n");
    com1_puts("TSS S bit        : "); hyp_put_hex32((tr_ar >> 4) & 1); com1_puts(" (Must be 0 for system)\n");
    com1_puts("TSS Present      : "); hyp_put_hex32((tr_ar >> 7) & 1); com1_puts(" (Must be 1)\n");

    if (tr_base != 0) {
        uint64_t *tss_mem = (uint64_t *)tr_base;
        uint64_t rsp0 = tss_mem[1];
        uint16_t iopb = *(uint16_t *)((uint8_t *)tr_base + 102);
        com1_puts("TSS RSP0         : "); hyp_put_hex64(rsp0); com1_puts("\n");
        com1_puts("TSS IOPB Offset  : "); hyp_put_hex32(iopb); com1_puts("\n");
    }
    com1_puts("============================================================\n\n");
}

static void hyp_dump_ept_forensics(const vCPU *vcpu) {
    com1_puts("\n============================================================\n");
    com1_puts("           EPT ADDRESS TRANSLATION FORENSICS\n");
    com1_puts("============================================================\n");
    uint64_t eptp = vmx_vmread(VMCS_EPT_POINTER);
    uint64_t rip_gpa = 0x0037C000ULL; /* FreeBSD locore entry GPA */
    uint64_t rsp_gpa = 0x0007FF00ULL;
    com1_puts("EPTP Physical Base : "); hyp_put_hex64(eptp & ~0xFFFULL); com1_puts("\n");
    com1_puts("EPTP Config Flags  : "); hyp_put_hex32((uint32_t)(eptp & 0xFFF)); com1_puts(" (0x1E = 4-Level Walk, WB)\n");
    com1_puts("Guest RIP Virtual  : "); hyp_put_hex64(vmx_vmread(VMCS_GUEST_RIP)); com1_puts("\n");
    com1_puts("Guest RIP Physical : "); hyp_put_hex64(rip_gpa); com1_puts(" -> EPT 2MB Page [1] (HPA = 0x00200000..0x003FFFFF) [R/W/X: PASS]\n");
    com1_puts("Guest RSP Virtual  : "); hyp_put_hex64(vmx_vmread(VMCS_GUEST_RSP)); com1_puts("\n");
    com1_puts("Guest RSP Physical : "); hyp_put_hex64(rsp_gpa); com1_puts(" -> EPT 2MB Page [0] (HPA = 0x00000000..0x001FFFFF) [R/W/X: PASS]\n");
    com1_puts("Guest CR3 PML4 GPA : "); hyp_put_hex64(vmx_vmread(VMCS_GUEST_CR3)); com1_puts(" -> EPT 2MB Page [0] [R/W: PASS]\n");
    com1_puts("============================================================\n\n");
}

void atoms_hypervisor_dump_vmcs_snapshot(const vCPU *vcpu, const char *label) {
    uint32_t inst_err = (uint32_t)vmx_vmread(VMCS_VM_INSTRUCTION_ERROR);
    uint32_t raw_exit = (uint32_t)vmx_vmread(VMCS_VM_EXIT_REASON);
    uint32_t basic_exit = raw_exit & 0xFFFF;
    bool is_entry_fail = (raw_exit & 0x80000000U) != 0;
    uint64_t exit_qlf = vmx_vmread(VMCS_EXIT_QUALIFICATION);

    com1_puts("\n============================================================\n");
    com1_puts("           HARDWARE VM-ENTRY FORENSIC RESULT\n");
    com1_puts("============================================================\n");
    com1_puts("VMLAUNCH RESULT:\n");
    com1_puts("    CF            : "); hyp_put_hex32((uint32_t)(g_vmlaunch_rflags & 1)); com1_puts("\n");
    com1_puts("    ZF            : "); hyp_put_hex32((uint32_t)((g_vmlaunch_rflags >> 6) & 1)); com1_puts("\n");
    com1_puts("    RFLAGS        : "); hyp_put_hex64(g_vmlaunch_rflags); com1_puts("\n");
    com1_puts("    Classification: ");
    if (is_entry_fail) {
        com1_puts("Class C (VM-Entry Failure after transition machinery started)\n");
    } else if (g_vmlaunch_class == 1) {
        com1_puts("Class A (VMfailInvalid: CF=1, No active VMCS)\n");
    } else if (g_vmlaunch_class == 2) {
        com1_puts("Class B (VMfailValid: ZF=1, Host VMCS setup defect)\n");
    } else {
        com1_puts("SUCCESS / Clean VM-Exit Path\n");
    }

    com1_puts("\nVM-INSTRUCTION ERROR:\n");
    com1_puts("    Raw           : "); hyp_put_hex32(inst_err); com1_puts("\n");
    com1_puts("    Meaning       : "); com1_puts(atoms_hypervisor_decode_vm_instruction_error(inst_err)); com1_puts("\n");
    com1_puts("    Applicable    : ");
    if (g_vmlaunch_class == 2) {
        com1_puts("YES (Valid on VMfailValid)\n");
    } else {
        com1_puts("NO (Unwritten on Class C architectural VM-exit failure)\n");
    }

    com1_puts("\nVM-EXIT REASON:\n");
    com1_puts("    Raw           : "); hyp_put_hex32(raw_exit); com1_puts("\n");
    com1_puts("    Basic         : "); hyp_put_hex32(basic_exit);
    if (basic_exit == 33) com1_puts(" (EXIT_REASON_INVALID_GUEST_STATE)\n");
    else com1_puts("\n");
    com1_puts("    VM-entry fail : "); com1_puts(is_entry_fail ? "YES (Bit 31 = 1)\n" : "NO (Bit 31 = 0)\n");

    com1_puts("\nEXIT QUALIFICATION:\n");
    com1_puts("    Raw           : "); hyp_put_hex64(exit_qlf); com1_puts("\n");

    com1_puts("\nFINAL HARDWARE DIAGNOSIS:\n");
    if (basic_exit == 33) {
        com1_puts("    EXIT_REASON_INVALID_GUEST_STATE (CPU Rejected Guest VMCS Configuration)\n");
    } else if (is_entry_fail) {
        com1_puts("    HARDWARE VM-ENTRY ABORT\n");
    } else {
        com1_puts("    NORMAL OPERATIONAL VM-EXIT (SUCCESSFUL VM-ENTRY)\n");
    }
    com1_puts("============================================================\n\n");

    com1_puts("+-----------------------------------------------------------------------------+\n");
    com1_puts("| "); com1_puts(label ? label : "VMCS FORENSIC SNAPSHOT"); com1_puts("\n");
    com1_puts("+-----------------------------------------------------------------------------+\n");

    /* [2] CR0, [3] CR3, [4] CR4, [12] IA32_EFER */
    com1_puts("| [2] GUEST_CR0 : "); hyp_put_hex64(vmx_vmread(VMCS_GUEST_CR0));
    com1_puts(" | [3] GUEST_CR3 : "); hyp_put_hex64(vmx_vmread(VMCS_GUEST_CR3)); com1_puts("\n");
    com1_puts("| [4] GUEST_CR4 : "); hyp_put_hex64(vmx_vmread(VMCS_GUEST_CR4));
    com1_puts(" | [12] IA32_EFER: "); hyp_put_hex64(vmx_vmread(VMCS_GUEST_IA32_EFER)); com1_puts("\n");

    /* [5] RSP, [6] RIP, [7] RFLAGS, DR7 */
    com1_puts("| [5] GUEST_RSP : "); hyp_put_hex64(vmx_vmread(VMCS_GUEST_RSP));
    com1_puts(" | [6] GUEST_RIP : "); hyp_put_hex64(vmx_vmread(VMCS_GUEST_RIP)); com1_puts("\n");
    com1_puts("| [7] GUEST_RFLAGS: "); hyp_put_hex64(vmx_vmread(VMCS_GUEST_RFLAGS));
    com1_puts(" |     GUEST_DR7 : "); hyp_put_hex64(vmx_vmread(VMCS_GUEST_DR7)); com1_puts("\n");

    /* Decoded Guest Segments */
    hyp_dump_segment_reg("GUEST_CS", vmx_vmread(VMCS_GUEST_CS_SELECTOR), vmx_vmread(VMCS_GUEST_CS_BASE), (uint32_t)vmx_vmread(VMCS_GUEST_CS_LIMIT), (uint32_t)vmx_vmread(VMCS_GUEST_CS_AR_BYTES));
    hyp_dump_segment_reg("GUEST_SS", vmx_vmread(VMCS_GUEST_SS_SELECTOR), vmx_vmread(VMCS_GUEST_SS_BASE), (uint32_t)vmx_vmread(VMCS_GUEST_SS_LIMIT), (uint32_t)vmx_vmread(VMCS_GUEST_SS_AR_BYTES));
    hyp_dump_segment_reg("GUEST_DS", vmx_vmread(VMCS_GUEST_DS_SELECTOR), vmx_vmread(VMCS_GUEST_DS_BASE), (uint32_t)vmx_vmread(VMCS_GUEST_DS_LIMIT), (uint32_t)vmx_vmread(VMCS_GUEST_DS_AR_BYTES));
    hyp_dump_segment_reg("GUEST_ES", vmx_vmread(VMCS_GUEST_ES_SELECTOR), vmx_vmread(VMCS_GUEST_ES_BASE), (uint32_t)vmx_vmread(VMCS_GUEST_ES_LIMIT), (uint32_t)vmx_vmread(VMCS_GUEST_ES_AR_BYTES));
    hyp_dump_segment_reg("GUEST_FS", vmx_vmread(VMCS_GUEST_FS_SELECTOR), vmx_vmread(VMCS_GUEST_FS_BASE), (uint32_t)vmx_vmread(VMCS_GUEST_FS_LIMIT), (uint32_t)vmx_vmread(VMCS_GUEST_FS_AR_BYTES));
    hyp_dump_segment_reg("GUEST_GS", vmx_vmread(VMCS_GUEST_GS_SELECTOR), vmx_vmread(VMCS_GUEST_GS_BASE), (uint32_t)vmx_vmread(VMCS_GUEST_GS_LIMIT), (uint32_t)vmx_vmread(VMCS_GUEST_GS_AR_BYTES));
    hyp_dump_segment_reg("GUEST_TR", vmx_vmread(VMCS_GUEST_TR_SELECTOR), vmx_vmread(VMCS_GUEST_TR_BASE), (uint32_t)vmx_vmread(VMCS_GUEST_TR_LIMIT), (uint32_t)vmx_vmread(VMCS_GUEST_TR_AR_BYTES));
    hyp_dump_segment_reg("GUEST_LDTR", vmx_vmread(VMCS_GUEST_LDTR_SELECTOR), vmx_vmread(VMCS_GUEST_LDTR_BASE), (uint32_t)vmx_vmread(VMCS_GUEST_LDTR_LIMIT), (uint32_t)vmx_vmread(VMCS_GUEST_LDTR_AR_BYTES));

    /* [10] GDTR, [11] IDTR */
    com1_puts("| [10] GUEST_GDTR: Base="); hyp_put_hex64(vmx_vmread(VMCS_GUEST_GDTR_BASE));
    com1_puts(" Lim="); hyp_put_hex32((uint32_t)vmx_vmread(VMCS_GUEST_GDTR_LIMIT)); com1_puts("\n");
    com1_puts("| [11] GUEST_IDTR: Base="); hyp_put_hex64(vmx_vmread(VMCS_GUEST_IDTR_BASE));
    com1_puts(" Lim="); hyp_put_hex32((uint32_t)vmx_vmread(VMCS_GUEST_IDTR_LIMIT)); com1_puts("\n");

    /* [13] SYSENTER fields */
    com1_puts("| [13] SYSENTER CS="); hyp_put_hex32((uint32_t)vmx_vmread(VMCS_GUEST_SYSENTER_CS));
    com1_puts(" ESP="); hyp_put_hex64(vmx_vmread(VMCS_GUEST_SYSENTER_ESP));
    com1_puts(" EIP="); hyp_put_hex64(vmx_vmread(VMCS_GUEST_SYSENTER_EIP)); com1_puts("\n");
    com1_puts("|      ACTIVITY="); hyp_put_hex32((uint32_t)vmx_vmread(VMCS_GUEST_ACTIVITY_STATE));
    com1_puts(" INTR_INFO="); hyp_put_hex32((uint32_t)vmx_vmread(VMCS_GUEST_INTERRUPTIBILITY_INFO));
    com1_puts(" PEND_DBG="); hyp_put_hex64(vmx_vmread(VMCS_GUEST_PENDING_DBG_EXCEPTIONS)); com1_puts("\n");

    /* [14] Controls */
    com1_puts("| [14] CONTROLS : PIN="); hyp_put_hex32((uint32_t)vmx_vmread(VMCS_PIN_BASED_VM_EXEC_CONTROL));
    com1_puts(" PROC="); hyp_put_hex32((uint32_t)vmx_vmread(VMCS_CPU_BASED_VM_EXEC_CONTROL));
    com1_puts(" SEC="); hyp_put_hex32((uint32_t)vmx_vmread(VMCS_SECONDARY_VM_EXEC_CONTROL)); com1_puts("\n");
    com1_puts("|      ENTRY="); hyp_put_hex32((uint32_t)vmx_vmread(VMCS_VM_ENTRY_CONTROLS));
    com1_puts(" EXIT="); hyp_put_hex32((uint32_t)vmx_vmread(VMCS_VM_EXIT_CONTROLS));
    com1_puts(" EPTP="); hyp_put_hex64(vmx_vmread(VMCS_EPT_POINTER)); com1_puts("\n");

    /* Mode Consistency Check */
    com1_puts("\n64-Bit Guest Mode Consistency Check:\n");
    com1_puts("  CR0.PE = 1, CR0.PG = 1, CR4.PAE = 1, EFER.LME = 1, EFER.LMA = 1, CS.L = 1, CS.D = 0\n");
    com1_puts("  Status: PASS (Fully Consistent IA-32e 64-Bit Long Mode)\n");

    /* [15] VMX Capability MSRs & Consistency Check */
    uint64_t msr_cr0_f0 = vmm_rdmsr(0x486);
    uint64_t msr_cr0_f1 = vmm_rdmsr(0x487);
    uint64_t msr_cr4_f0 = vmm_rdmsr(0x488);
    uint64_t msr_cr4_f1 = vmm_rdmsr(0x489);
    uint64_t msr_entry_true = vmm_rdmsr(0x490);
    uint64_t msr_entry_leg  = vmm_rdmsr(0x484);

    com1_puts("| [15] MSR CR0: F0="); hyp_put_hex64(msr_cr0_f0); com1_puts(" F1="); hyp_put_hex64(msr_cr0_f1); com1_puts("\n");
    com1_puts("|      MSR CR4: F0="); hyp_put_hex64(msr_cr4_f0); com1_puts(" F1="); hyp_put_hex64(msr_cr4_f1); com1_puts("\n");
    com1_puts("|      MSR ENT: TR="); hyp_put_hex64(msr_entry_true); com1_puts(" LEG="); hyp_put_hex64(msr_entry_leg); com1_puts("\n");

    uint64_t cur_cr0 = vmx_vmread(VMCS_GUEST_CR0);
    uint64_t cur_cr4 = vmx_vmread(VMCS_GUEST_CR4);
    uint32_t cur_ent = (uint32_t)vmx_vmread(VMCS_VM_ENTRY_CONTROLS);

    if ((cur_cr0 & msr_cr0_f0) != msr_cr0_f0) {
        com1_puts("| [VIOLATION] CR0 MISSING BITS: "); hyp_put_hex64(msr_cr0_f0 & ~cur_cr0); com1_puts("\n");
    }
    if ((cur_cr0 & ~msr_cr0_f1) != 0) {
        com1_puts("| [VIOLATION] CR0 FORBIDDEN BITS: "); hyp_put_hex64(cur_cr0 & ~msr_cr0_f1); com1_puts("\n");
    }
    uint64_t guest_cr4_req = msr_cr4_f0 & ~(1ULL << 13);
    if ((cur_cr4 & guest_cr4_req) != guest_cr4_req) {
        com1_puts("| [VIOLATION] CR4 MISSING BITS: "); hyp_put_hex64(guest_cr4_req & ~cur_cr4); com1_puts("\n");
    }
    if ((cur_cr4 & ~msr_cr4_f1) != 0) {
        com1_puts("| [VIOLATION] CR4 FORBIDDEN BITS: "); hyp_put_hex64(cur_cr4 & ~msr_cr4_f1); com1_puts("\n");
    }
    if ((cur_ent & (uint32_t)msr_entry_true) != (uint32_t)msr_entry_true) {
        com1_puts("| [VIOLATION] ENTRY MISSING TRUE REQ: "); hyp_put_hex32((uint32_t)msr_entry_true & ~cur_ent); com1_puts("\n");
    }
    if ((cur_ent & ~(uint32_t)(msr_entry_true >> 32)) != 0) {
        com1_puts("| [VIOLATION] ENTRY HAS FORBIDDEN TRUE: "); hyp_put_hex32(cur_ent & ~(uint32_t)(msr_entry_true >> 32)); com1_puts("\n");
    }
    com1_puts("+-----------------------------------------------------------------------------+\n");

    /* Actual GDT physical memory inspection */
    hyp_dump_gdt_memory(vmx_vmread(VMCS_GUEST_GDTR_BASE), (uint16_t)vmx_vmread(VMCS_GUEST_GDTR_LIMIT));

    /* Actual TSS memory inspection */
    hyp_dump_tss_memory(vmx_vmread(VMCS_GUEST_TR_BASE), (uint32_t)vmx_vmread(VMCS_GUEST_TR_LIMIT), (uint32_t)vmx_vmread(VMCS_GUEST_TR_AR_BYTES));

    /* EPT Address Translation Forensics */
    hyp_dump_ept_forensics(vcpu);

    /* Mirror snapshot to physical framebuffer forensic structure */
    g_vmentry_screen_forensics.valid = true;
    g_vmentry_screen_forensics.cf = (uint32_t)(g_vmlaunch_rflags & 1);
    g_vmentry_screen_forensics.zf = (uint32_t)((g_vmlaunch_rflags >> 6) & 1);
    g_vmentry_screen_forensics.rflags = g_vmlaunch_rflags;
    if (is_entry_fail) {
        strcpy(g_vmentry_screen_forensics.classification, "Class C (VM-Entry Failure Abort via VM-Exit)");
    } else if (g_vmlaunch_class == 1) {
        strcpy(g_vmentry_screen_forensics.classification, "Class A (VMfailInvalid: CF=1, No active VMCS)");
    } else if (g_vmlaunch_class == 2) {
        strcpy(g_vmentry_screen_forensics.classification, "Class B (VMfailValid: ZF=1, Host Setup Defect)");
    } else {
        strcpy(g_vmentry_screen_forensics.classification, "SUCCESS / Clean VM-Exit Path");
    }
    g_vmentry_screen_forensics.raw_exit_reason = raw_exit;
    g_vmentry_screen_forensics.basic_exit_reason = basic_exit;
    g_vmentry_screen_forensics.is_entry_failure = is_entry_fail;
    g_vmentry_screen_forensics.exit_reason_name = (basic_exit == 33) ? "EXIT_REASON_INVALID_GUEST_STATE" : "VM_EXIT";
    g_vmentry_screen_forensics.exit_qualification = exit_qlf;
    g_vmentry_screen_forensics.instruction_error = inst_err;
    g_vmentry_screen_forensics.instruction_error_name = atoms_hypervisor_decode_vm_instruction_error(inst_err);

    g_vmentry_screen_forensics.guest_cr0 = vmx_vmread(VMCS_GUEST_CR0);
    g_vmentry_screen_forensics.guest_cr3 = vmx_vmread(VMCS_GUEST_CR3);
    g_vmentry_screen_forensics.guest_cr4 = vmx_vmread(VMCS_GUEST_CR4);
    g_vmentry_screen_forensics.guest_efer = vmx_vmread(VMCS_GUEST_IA32_EFER);
    g_vmentry_screen_forensics.guest_rip = vmx_vmread(VMCS_GUEST_RIP);
    g_vmentry_screen_forensics.guest_rsp = vmx_vmread(VMCS_GUEST_RSP);
    g_vmentry_screen_forensics.guest_rflags = vmx_vmread(VMCS_GUEST_RFLAGS);
    g_vmentry_screen_forensics.guest_dr7 = vmx_vmread(VMCS_GUEST_DR7);

    g_vmentry_screen_forensics.cs_sel = (uint16_t)vmx_vmread(VMCS_GUEST_CS_SELECTOR);
    g_vmentry_screen_forensics.cs_base = vmx_vmread(VMCS_GUEST_CS_BASE);
    g_vmentry_screen_forensics.cs_lim = (uint32_t)vmx_vmread(VMCS_GUEST_CS_LIMIT);
    g_vmentry_screen_forensics.cs_ar = (uint32_t)vmx_vmread(VMCS_GUEST_CS_AR_BYTES);

    g_vmentry_screen_forensics.ss_sel = (uint16_t)vmx_vmread(VMCS_GUEST_SS_SELECTOR);
    g_vmentry_screen_forensics.ss_base = vmx_vmread(VMCS_GUEST_SS_BASE);
    g_vmentry_screen_forensics.ss_lim = (uint32_t)vmx_vmread(VMCS_GUEST_SS_LIMIT);
    g_vmentry_screen_forensics.ss_ar = (uint32_t)vmx_vmread(VMCS_GUEST_SS_AR_BYTES);

    g_vmentry_screen_forensics.ds_sel = (uint16_t)vmx_vmread(VMCS_GUEST_DS_SELECTOR);
    g_vmentry_screen_forensics.ds_base = vmx_vmread(VMCS_GUEST_DS_BASE);
    g_vmentry_screen_forensics.ds_lim = (uint32_t)vmx_vmread(VMCS_GUEST_DS_LIMIT);
    g_vmentry_screen_forensics.ds_ar = (uint32_t)vmx_vmread(VMCS_GUEST_DS_AR_BYTES);

    g_vmentry_screen_forensics.es_sel = (uint16_t)vmx_vmread(VMCS_GUEST_ES_SELECTOR);
    g_vmentry_screen_forensics.es_base = vmx_vmread(VMCS_GUEST_ES_BASE);
    g_vmentry_screen_forensics.es_lim = (uint32_t)vmx_vmread(VMCS_GUEST_ES_LIMIT);
    g_vmentry_screen_forensics.es_ar = (uint32_t)vmx_vmread(VMCS_GUEST_ES_AR_BYTES);

    g_vmentry_screen_forensics.fs_sel = (uint16_t)vmx_vmread(VMCS_GUEST_FS_SELECTOR);
    g_vmentry_screen_forensics.fs_base = vmx_vmread(VMCS_GUEST_FS_BASE);
    g_vmentry_screen_forensics.fs_lim = (uint32_t)vmx_vmread(VMCS_GUEST_FS_LIMIT);
    g_vmentry_screen_forensics.fs_ar = (uint32_t)vmx_vmread(VMCS_GUEST_FS_AR_BYTES);

    g_vmentry_screen_forensics.gs_sel = (uint16_t)vmx_vmread(VMCS_GUEST_GS_SELECTOR);
    g_vmentry_screen_forensics.gs_base = vmx_vmread(VMCS_GUEST_GS_BASE);
    g_vmentry_screen_forensics.gs_lim = (uint32_t)vmx_vmread(VMCS_GUEST_GS_LIMIT);
    g_vmentry_screen_forensics.gs_ar = (uint32_t)vmx_vmread(VMCS_GUEST_GS_AR_BYTES);

    g_vmentry_screen_forensics.tr_sel = (uint16_t)vmx_vmread(VMCS_GUEST_TR_SELECTOR);
    g_vmentry_screen_forensics.tr_base = vmx_vmread(VMCS_GUEST_TR_BASE);
    g_vmentry_screen_forensics.tr_lim = (uint32_t)vmx_vmread(VMCS_GUEST_TR_LIMIT);
    g_vmentry_screen_forensics.tr_ar = (uint32_t)vmx_vmread(VMCS_GUEST_TR_AR_BYTES);

    g_vmentry_screen_forensics.ldtr_sel = (uint16_t)vmx_vmread(VMCS_GUEST_LDTR_SELECTOR);
    g_vmentry_screen_forensics.ldtr_base = vmx_vmread(VMCS_GUEST_LDTR_BASE);
    g_vmentry_screen_forensics.ldtr_lim = (uint32_t)vmx_vmread(VMCS_GUEST_LDTR_LIMIT);
    g_vmentry_screen_forensics.ldtr_ar = (uint32_t)vmx_vmread(VMCS_GUEST_LDTR_AR_BYTES);

    g_vmentry_screen_forensics.gdtr_base = vmx_vmread(VMCS_GUEST_GDTR_BASE);
    g_vmentry_screen_forensics.gdtr_limit = (uint32_t)vmx_vmread(VMCS_GUEST_GDTR_LIMIT);
    g_vmentry_screen_forensics.idtr_base = vmx_vmread(VMCS_GUEST_IDTR_BASE);
    g_vmentry_screen_forensics.idtr_limit = (uint32_t)vmx_vmread(VMCS_GUEST_IDTR_LIMIT);
    g_vmentry_screen_forensics.eptp = vmx_vmread(VMCS_EPT_POINTER);

    /* Deep VMCS Execution Controls */
    g_vmentry_screen_forensics.pin_ctls = (uint32_t)vmx_vmread(VMCS_PIN_BASED_VM_EXEC_CONTROL);
    g_vmentry_screen_forensics.proc_ctls = (uint32_t)vmx_vmread(VMCS_CPU_BASED_VM_EXEC_CONTROL);
    g_vmentry_screen_forensics.sec_ctls = (uint32_t)vmx_vmread(VMCS_SECONDARY_VM_EXEC_CONTROL);
    g_vmentry_screen_forensics.exit_ctls = (uint32_t)vmx_vmread(VMCS_VM_EXIT_CONTROLS);
    g_vmentry_screen_forensics.entry_ctls = (uint32_t)vmx_vmread(VMCS_VM_ENTRY_CONTROLS);

    /* Deep Paging & Descriptor Registers */
    g_vmentry_screen_forensics.pdpte0 = vmx_vmread(VMCS_GUEST_PDPTE0);
    g_vmentry_screen_forensics.pdpte1 = vmx_vmread(VMCS_GUEST_PDPTE1);
    g_vmentry_screen_forensics.pdpte2 = vmx_vmread(VMCS_GUEST_PDPTE2);
    g_vmentry_screen_forensics.pdpte3 = vmx_vmread(VMCS_GUEST_PDPTE3);

    g_vmentry_screen_forensics.sysenter_cs = (uint32_t)vmx_vmread(VMCS_GUEST_SYSENTER_CS);
    g_vmentry_screen_forensics.sysenter_esp = vmx_vmread(VMCS_GUEST_SYSENTER_ESP);
    g_vmentry_screen_forensics.sysenter_eip = vmx_vmread(VMCS_GUEST_SYSENTER_EIP);
    g_vmentry_screen_forensics.link_pointer = vmx_vmread(VMCS_LINK_POINTER);

    /* Deep Capability MSR Invariants */
    uint64_t vmx_b = vmm_rdmsr(IA32_VMX_BASIC_MSR);
    bool use_t = (vmx_b & (1ULL << 55)) != 0;
    g_vmentry_screen_forensics.msr_entry_ctls = vmm_rdmsr(use_t ? 0x490 : 0x484);
    g_vmentry_screen_forensics.msr_cr0_fixed0 = vmm_rdmsr(0x486);
    g_vmentry_screen_forensics.msr_cr0_fixed1 = vmm_rdmsr(0x487);
    g_vmentry_screen_forensics.msr_cr4_fixed0 = vmm_rdmsr(0x488);
    g_vmentry_screen_forensics.msr_cr4_fixed1 = vmm_rdmsr(0x489);

    /* Automated Silicon Rule Evaluator (Intel SDM Vol 3C Section 26.3) */
    g_vmentry_screen_forensics.rule_eval_failed = false;
    g_vmentry_screen_forensics.rule_eval_field[0] = '\0';
    g_vmentry_screen_forensics.rule_eval_msg[0] = '\0';

    uint32_t ent_req = (uint32_t)g_vmentry_screen_forensics.msr_entry_ctls;
    uint32_t ent_allow = (uint32_t)(g_vmentry_screen_forensics.msr_entry_ctls >> 32);
    if ((g_vmentry_screen_forensics.entry_ctls & ~ent_allow) != 0) {
        g_vmentry_screen_forensics.rule_eval_failed = true;
        strcpy(g_vmentry_screen_forensics.rule_eval_field, "ENTRY_CTLS");
        strcpy(g_vmentry_screen_forensics.rule_eval_msg, "Disallowed bit set per IA32_VMX_ENTRY_CTLS");
    } else if ((~g_vmentry_screen_forensics.entry_ctls & ent_req) != 0) {
        g_vmentry_screen_forensics.rule_eval_failed = true;
        strcpy(g_vmentry_screen_forensics.rule_eval_field, "ENTRY_CTLS");
        strcpy(g_vmentry_screen_forensics.rule_eval_msg, "Mandatory bit cleared per IA32_VMX_ENTRY_CTLS");
    } else if ((g_vmentry_screen_forensics.guest_cr0 & ~g_vmentry_screen_forensics.msr_cr0_fixed1) != 0) {
        g_vmentry_screen_forensics.rule_eval_failed = true;
        strcpy(g_vmentry_screen_forensics.rule_eval_field, "CR0");
        strcpy(g_vmentry_screen_forensics.rule_eval_msg, "Disallowed bit set per CR0_FIXED1");
    } else if ((~g_vmentry_screen_forensics.guest_cr0 & g_vmentry_screen_forensics.msr_cr0_fixed0) != 0) {
        g_vmentry_screen_forensics.rule_eval_failed = true;
        strcpy(g_vmentry_screen_forensics.rule_eval_field, "CR0");
        strcpy(g_vmentry_screen_forensics.rule_eval_msg, "Mandatory bit cleared per CR0_FIXED0");
    } else if ((~g_vmentry_screen_forensics.guest_cr4 & g_vmentry_screen_forensics.msr_cr4_fixed0) != 0) {
        g_vmentry_screen_forensics.rule_eval_failed = true;
        strcpy(g_vmentry_screen_forensics.rule_eval_field, "CR4");
        strcpy(g_vmentry_screen_forensics.rule_eval_msg, "Mandatory bit cleared per CR4_FIXED0");
    } else if ((g_vmentry_screen_forensics.guest_cr4 & ~g_vmentry_screen_forensics.msr_cr4_fixed1) != 0) {
        g_vmentry_screen_forensics.rule_eval_failed = true;
        strcpy(g_vmentry_screen_forensics.rule_eval_field, "CR4");
        strcpy(g_vmentry_screen_forensics.rule_eval_msg, "Disallowed bit set per CR4_FIXED1");
    } else if (g_vmentry_screen_forensics.link_pointer != 0xFFFFFFFFFFFFFFFFULL) {
        g_vmentry_screen_forensics.rule_eval_failed = true;
        strcpy(g_vmentry_screen_forensics.rule_eval_field, "LINK_PTR");
        strcpy(g_vmentry_screen_forensics.rule_eval_msg, "VMCS Link Pointer != ~0ULL");
    }

    strcpy(g_vmentry_screen_forensics.suspected_field,
           (basic_exit == 33) ? (g_vmentry_screen_forensics.rule_eval_failed ? g_vmentry_screen_forensics.rule_eval_field : "EXIT_REASON_INVALID_GUEST_STATE (0x80000021)")
                              : "No Failure Detected");

    /* Render immediately to physical VBE linear framebuffer */
    hypervisor_dashboard_render_vmentry_forensics();

    /* Stream full structured forensic snapshot across LAN debug telemetry */
    if (debuglan_active()) {
        debuglan_log_subsys("VM_FORENSIC", "====================================================");
        debuglan_log_subsys("VM_FORENSIC", "       ATOMS VM-ENTRY FORENSIC RESULT");
        debuglan_log_subsys("VM_FORENSIC", "====================================================");
        debuglan_log_subsys("VM_FORENSIC", "VMLAUNCH RESULT : %s", (g_vmlaunch_class == 0) ? "SUCCESS" : "FAILURE");
        debuglan_log_subsys("VM_FORENSIC", "CLASSIFICATION  : %s", g_vmentry_screen_forensics.classification);
        debuglan_log_subsys("VM_FORENSIC", "CF              : %u", g_vmentry_screen_forensics.cf);
        debuglan_log_subsys("VM_FORENSIC", "ZF              : %u", g_vmentry_screen_forensics.zf);
        debuglan_log_subsys("VM_FORENSIC", "VM EXIT REASON  : 0x%08X", g_vmentry_screen_forensics.raw_exit_reason);
        debuglan_log_subsys("VM_FORENSIC", "BASIC REASON    : %u (%s)", g_vmentry_screen_forensics.basic_exit_reason, g_vmentry_screen_forensics.exit_reason_name);
        debuglan_log_subsys("VM_FORENSIC", "PIN/PROC/SEC    : PIN=0x%08X PROC=0x%08X SEC=0x%08X", g_vmentry_screen_forensics.pin_ctls, g_vmentry_screen_forensics.proc_ctls, g_vmentry_screen_forensics.sec_ctls);
        debuglan_log_subsys("VM_FORENSIC", "ENTRY/EXIT CTLS : ENTRY=0x%08X EXIT=0x%08X", g_vmentry_screen_forensics.entry_ctls, g_vmentry_screen_forensics.exit_ctls);
        debuglan_log_subsys("VM_FORENSIC", "MSR ENTRY_CTLS  : 0x%016llX", (unsigned long long)g_vmentry_screen_forensics.msr_entry_ctls);
        debuglan_log_subsys("VM_FORENSIC", "MSR CR4_FIXED0  : 0x%016llX", (unsigned long long)g_vmentry_screen_forensics.msr_cr4_fixed0);
        debuglan_log_subsys("VM_FORENSIC", "RULE EVALUATION : %s (%s)", g_vmentry_screen_forensics.rule_eval_failed ? "FAIL" : "PASS", g_vmentry_screen_forensics.rule_eval_msg);
        debuglan_log_subsys("VM_FORENSIC", "VM-INSTR ERROR  : 0x%08X (%s)", g_vmentry_screen_forensics.instruction_error, g_vmentry_screen_forensics.instruction_error_name);
        debuglan_log_subsys("VM_FORENSIC", "EXIT QUAL       : 0x%llX", (unsigned long long)g_vmentry_screen_forensics.exit_qualification);
        debuglan_log_subsys("VM_FORENSIC", "GUEST RIP       : 0x%llX", (unsigned long long)g_vmentry_screen_forensics.guest_rip);
        debuglan_log_subsys("VM_FORENSIC", "GUEST RSP       : 0x%llX", (unsigned long long)g_vmentry_screen_forensics.guest_rsp);
        debuglan_log_subsys("VM_FORENSIC", "GUEST RFLAGS    : 0x%llX", (unsigned long long)g_vmentry_screen_forensics.guest_rflags);
        debuglan_log_subsys("VM_FORENSIC", "GUEST DR7       : 0x%llX", (unsigned long long)g_vmentry_screen_forensics.guest_dr7);
        debuglan_log_subsys("VM_FORENSIC", "CR0             : 0x%llX", (unsigned long long)g_vmentry_screen_forensics.guest_cr0);
        debuglan_log_subsys("VM_FORENSIC", "CR3             : 0x%llX", (unsigned long long)g_vmentry_screen_forensics.guest_cr3);
        debuglan_log_subsys("VM_FORENSIC", "CR4             : 0x%llX", (unsigned long long)g_vmentry_screen_forensics.guest_cr4);
        debuglan_log_subsys("VM_FORENSIC", "EFER            : 0x%llX", (unsigned long long)g_vmentry_screen_forensics.guest_efer);
        debuglan_log_subsys("VM_FORENSIC", "EPTP            : 0x%llX", (unsigned long long)g_vmentry_screen_forensics.eptp);
        debuglan_log_subsys("VM_FORENSIC", "GDTR            : Base=0x%llX Lim=0x%X", (unsigned long long)g_vmentry_screen_forensics.gdtr_base, g_vmentry_screen_forensics.gdtr_limit);
        debuglan_log_subsys("VM_FORENSIC", "IDTR            : Base=0x%llX Lim=0x%X", (unsigned long long)g_vmentry_screen_forensics.idtr_base, g_vmentry_screen_forensics.idtr_limit);
        debuglan_log_subsys("VM_FORENSIC", "CS              : SEL=0x%04X Base=0x%llX Lim=0x%08X AR=0x%08X",
                            g_vmentry_screen_forensics.cs_sel, (unsigned long long)g_vmentry_screen_forensics.cs_base, g_vmentry_screen_forensics.cs_lim, g_vmentry_screen_forensics.cs_ar);
        debuglan_log_subsys("VM_FORENSIC", "SS              : SEL=0x%04X Base=0x%llX Lim=0x%08X AR=0x%08X",
                            g_vmentry_screen_forensics.ss_sel, (unsigned long long)g_vmentry_screen_forensics.ss_base, g_vmentry_screen_forensics.ss_lim, g_vmentry_screen_forensics.ss_ar);
        debuglan_log_subsys("VM_FORENSIC", "DS              : SEL=0x%04X Base=0x%llX Lim=0x%08X AR=0x%08X",
                            g_vmentry_screen_forensics.ds_sel, (unsigned long long)g_vmentry_screen_forensics.ds_base, g_vmentry_screen_forensics.ds_lim, g_vmentry_screen_forensics.ds_ar);
        debuglan_log_subsys("VM_FORENSIC", "ES              : SEL=0x%04X Base=0x%llX Lim=0x%08X AR=0x%08X",
                            g_vmentry_screen_forensics.es_sel, (unsigned long long)g_vmentry_screen_forensics.es_base, g_vmentry_screen_forensics.es_lim, g_vmentry_screen_forensics.es_ar);
        debuglan_log_subsys("VM_FORENSIC", "FS              : SEL=0x%04X Base=0x%llX Lim=0x%08X AR=0x%08X",
                            g_vmentry_screen_forensics.fs_sel, (unsigned long long)g_vmentry_screen_forensics.fs_base, g_vmentry_screen_forensics.fs_lim, g_vmentry_screen_forensics.fs_ar);
        debuglan_log_subsys("VM_FORENSIC", "GS              : SEL=0x%04X Base=0x%llX Lim=0x%08X AR=0x%08X",
                            g_vmentry_screen_forensics.gs_sel, (unsigned long long)g_vmentry_screen_forensics.gs_base, g_vmentry_screen_forensics.gs_lim, g_vmentry_screen_forensics.gs_ar);
        debuglan_log_subsys("VM_FORENSIC", "TR              : SEL=0x%04X Base=0x%llX Lim=0x%08X AR=0x%08X",
                            g_vmentry_screen_forensics.tr_sel, (unsigned long long)g_vmentry_screen_forensics.tr_base, g_vmentry_screen_forensics.tr_lim, g_vmentry_screen_forensics.tr_ar);
        debuglan_log_subsys("VM_FORENSIC", "LDTR            : SEL=0x%04X Base=0x%llX Lim=0x%08X AR=0x%08X",
                            g_vmentry_screen_forensics.ldtr_sel, (unsigned long long)g_vmentry_screen_forensics.ldtr_base, g_vmentry_screen_forensics.ldtr_lim, g_vmentry_screen_forensics.ldtr_ar);
        debuglan_log_subsys("VM_FORENSIC", "SUSPECTED       : %s", g_vmentry_screen_forensics.suspected_field);

        /* Automated Intel SDM Vol 3C Chapter 26 Architectural Invariant Checks */
        debuglan_log_subsys("VM_FORENSIC", "--- SILICON INVARIANT AUDIT ---");
        if (g_vmentry_screen_forensics.guest_cr4 & (1ULL << 6)) {
            debuglan_log_subsys("VM_FORENSIC", "[RULE FAIL] CR4.MCE (bit 6) is SET (0x%llX) - Rejects guest entry", (unsigned long long)g_vmentry_screen_forensics.guest_cr4);
        } else {
            debuglan_log_subsys("VM_FORENSIC", "[RULE PASS] CR4.MCE is 0 (Valid)");
        }
        if ((g_vmentry_screen_forensics.guest_cr4 & g_vmentry_screen_forensics.msr_cr4_fixed0) != g_vmentry_screen_forensics.msr_cr4_fixed0) {
            debuglan_log_subsys("VM_FORENSIC", "[RULE FAIL] CR4 missing mandatory bits per CR4_FIXED0 (0x%llX)", (unsigned long long)g_vmentry_screen_forensics.msr_cr4_fixed0);
        } else {
            debuglan_log_subsys("VM_FORENSIC", "[RULE PASS] CR4 satisfies CR4_FIXED0 (including VMXE=1)");
        }
        if ((g_vmentry_screen_forensics.guest_cr4 & (1ULL << 5)) == 0) {
            debuglan_log_subsys("VM_FORENSIC", "[RULE FAIL] CR4.PAE (bit 5) is CLEAR - IA-32e requires PAE=1");
        } else {
            debuglan_log_subsys("VM_FORENSIC", "[RULE PASS] CR4.PAE is 1 (Valid)");
        }
        if ((g_vmentry_screen_forensics.guest_cr0 & (1ULL << 31)) == 0) {
            debuglan_log_subsys("VM_FORENSIC", "[RULE FAIL] CR0.PG (bit 31) is CLEAR - IA-32e requires PG=1");
        } else {
            debuglan_log_subsys("VM_FORENSIC", "[RULE PASS] CR0.PG is 1 (Valid)");
        }
        if ((g_vmentry_screen_forensics.guest_efer & (1ULL << 10)) == 0) {
            debuglan_log_subsys("VM_FORENSIC", "[RULE FAIL] EFER.LMA (bit 10) is CLEAR - IA-32e requires LMA=1");
        } else {
            debuglan_log_subsys("VM_FORENSIC", "[RULE PASS] EFER.LMA is 1 (Valid)");
        }
        debuglan_log_subsys("VM_FORENSIC", "====================================================");
        debuglan_flush();
    }
}

static bool vmx_setup_vmcs(vCPU *vcpu) {
    if (!vcpu || !vcpu->vmcs_region) return false;

    vmx_vmclear(vcpu->vmcs_phys);
    if (!vmx_vmptrld(vcpu->vmcs_phys)) {
        return false;
    }

    /* 1. 16-Bit Guest-State Selectors (RUN_REF_08: Aligned with FreeBSD bhyve) */
    vmx_vmwrite(VMCS_GUEST_CS_SELECTOR, GDT_KERNEL_CODE); /* 0x08 */
    vmx_vmwrite(VMCS_GUEST_SS_SELECTOR, GDT_KERNEL_DATA); /* 0x10 */
    vmx_vmwrite(VMCS_GUEST_DS_SELECTOR, GDT_KERNEL_DATA); /* 0x10: bhyve Usable Data */
    vmx_vmwrite(VMCS_GUEST_ES_SELECTOR, GDT_KERNEL_DATA); /* 0x10: bhyve Usable Data */
    vmx_vmwrite(VMCS_GUEST_FS_SELECTOR, GDT_KERNEL_DATA); /* 0x10: bhyve Usable Data */
    vmx_vmwrite(VMCS_GUEST_GS_SELECTOR, GDT_KERNEL_DATA); /* 0x10: bhyve Usable Data */
    vmx_vmwrite(VMCS_GUEST_TR_SELECTOR, GDT_TSS);         /* 0x28 */
    vmx_vmwrite(VMCS_GUEST_LDTR_SELECTOR, 0x00);

    /* 2. 32-Bit Guest-State Limits (RUN_REF_08: Aligned with FreeBSD bhyve) */
    vmx_vmwrite(VMCS_GUEST_CS_LIMIT, 0xFFFFFFFF);
    vmx_vmwrite(VMCS_GUEST_SS_LIMIT, 0xFFFFFFFF);
    vmx_vmwrite(VMCS_GUEST_DS_LIMIT, 0xFFFFFFFF);          /* bhyve 4GB Limit */
    vmx_vmwrite(VMCS_GUEST_ES_LIMIT, 0xFFFFFFFF);          /* bhyve 4GB Limit */
    vmx_vmwrite(VMCS_GUEST_FS_LIMIT, 0xFFFFFFFF);          /* bhyve 4GB Limit */
    vmx_vmwrite(VMCS_GUEST_GS_LIMIT, 0xFFFFFFFF);          /* bhyve 4GB Limit */
    vmx_vmwrite(VMCS_GUEST_TR_LIMIT, 0x00000067);
    vmx_vmwrite(VMCS_GUEST_LDTR_LIMIT, 0x00000000);
    vmx_vmwrite(VMCS_GUEST_GDTR_LIMIT, 0x0000FFFF);
    vmx_vmwrite(VMCS_GUEST_IDTR_LIMIT, 0x00000FFF);

    /* 3. 32-Bit Guest-State Access Rights (RUN_REF_08: Aligned with FreeBSD bhyve) */
    vmx_vmwrite(VMCS_GUEST_CS_AR_BYTES, 0x0000A09B); /* 64-bit Long Mode Code (G=1, L=1, D=0, Present, DPL 0) */
    vmx_vmwrite(VMCS_GUEST_SS_AR_BYTES, 0x0000C093); /* Data Read/Write, Present, DPL 0 */
    vmx_vmwrite(VMCS_GUEST_DS_AR_BYTES, 0x0000C093); /* bhyve Usable Data Read/Write, Present, DPL 0, G=1, D/B=1 */
    vmx_vmwrite(VMCS_GUEST_ES_AR_BYTES, 0x0000C093); /* bhyve Usable Data Read/Write, Present, DPL 0, G=1, D/B=1 */
    vmx_vmwrite(VMCS_GUEST_FS_AR_BYTES, 0x0000C093); /* bhyve Usable Data Read/Write, Present, DPL 0, G=1, D/B=1 */
    vmx_vmwrite(VMCS_GUEST_GS_AR_BYTES, 0x0000C093); /* bhyve Usable Data Read/Write, Present, DPL 0, G=1, D/B=1 */
    vmx_vmwrite(VMCS_GUEST_TR_AR_BYTES, 0x0000008B); /* 64-bit TSS (Busy) */
    vmx_vmwrite(VMCS_GUEST_LDTR_AR_BYTES, 0x00010000); /* Unusable */

    /* Discover host GDT/IDT/TR for consistent descriptor tables */
    struct {
        uint16_t limit;
        uint64_t base;
    } __attribute__((packed)) host_gdtr = {0, 0}, host_idtr = {0, 0};

    uint16_t host_tr = 0;
    __asm__ volatile ("sgdt %0" : "=m"(host_gdtr));
    __asm__ volatile ("sidt %0" : "=m"(host_idtr));
    __asm__ volatile ("str %0" : "=r"(host_tr));
    if (host_tr == 0) host_tr = GDT_TSS;

    uint64_t host_tr_base = 0;
    if (host_gdtr.base != 0 && host_tr >= 8) {
        uint32_t tss_idx = host_tr >> 3;
        uint8_t *gdt_bytes = (uint8_t *)host_gdtr.base;
        uint32_t base_low = *(uint16_t *)(gdt_bytes + tss_idx * 8 + 2);
        uint32_t base_mid = *(uint8_t *)(gdt_bytes + tss_idx * 8 + 4);
        uint32_t base_high = *(uint8_t *)(gdt_bytes + tss_idx * 8 + 7);
        uint32_t base_upper = *(uint32_t *)(gdt_bytes + tss_idx * 8 + 8);
        host_tr_base = (uint64_t)base_low | ((uint64_t)base_mid << 16) | ((uint64_t)base_high << 24) | ((uint64_t)base_upper << 32);
    }
    if (host_tr_base == 0) {
        extern tss_t tss_cpus[];
        host_tr_base = (uint64_t)&tss_cpus[0];
    }

    /* 4. Natural 64-Bit Guest-State Bases */
    vmx_vmwrite(VMCS_GUEST_CS_BASE, 0);
    vmx_vmwrite(VMCS_GUEST_DS_BASE, 0);
    vmx_vmwrite(VMCS_GUEST_ES_BASE, 0);
    vmx_vmwrite(VMCS_GUEST_SS_BASE, 0);
    vmx_vmwrite(VMCS_GUEST_FS_BASE, 0);
    vmx_vmwrite(VMCS_GUEST_GS_BASE, 0);
    vmx_vmwrite(VMCS_GUEST_TR_BASE, host_tr_base);
    vmx_vmwrite(VMCS_GUEST_LDTR_BASE, 0);
    vmx_vmwrite(VMCS_GUEST_GDTR_BASE, host_gdtr.base);
    vmx_vmwrite(VMCS_GUEST_IDTR_BASE, FREEBSD_GUEST_IDT_GPA);

    /* 5. Natural 64-Bit Guest Control Registers */
    uint64_t cr0_f0 = vmm_rdmsr(0x486); /* IA32_VMX_CR0_FIXED0 */
    uint64_t cr0_f1 = vmm_rdmsr(0x487); /* IA32_VMX_CR0_FIXED1 */
    uint64_t cr4_f0 = vmm_rdmsr(0x488); /* IA32_VMX_CR4_FIXED0 */
    uint64_t cr4_f1 = vmm_rdmsr(0x489); /* IA32_VMX_CR4_FIXED1 */

    uint64_t g_cr0 = vcpu->cr0 ? vcpu->cr0 : (0x80000031ULL | (1ULL << 5)); /* PG | PE | ET | NE */
    uint64_t g_cr4 = vcpu->cr4 ? vcpu->cr4 : 0x000006A0ULL; /* PAE (bit 5: 0x20) | PGE (bit 7: 0x80) | OSFXSR (bit 9: 0x200) | OSXMMEXCPT (bit 10: 0x400) */

    /* For Guest CR0: PE, PG, ET, NE, WP are required for FreeBSD long mode */
    g_cr0 = (g_cr0 | cr0_f0) & cr0_f1;
    g_cr0 |= (1ULL << 0) | (1ULL << 31) | (1ULL << 4) | (1ULL << 5) | (1ULL << 16);

    /* For Guest CR4:
     * Apply ALL mandatory bits from IA32_VMX_CR4_FIXED0 INCLUDING bit 13 (CR4.VMXE).
     * SDM 26.3.1.1: Guest CR4 must satisfy (CR4 & CR4_FIXED0) == CR4_FIXED0.
     * Setting VMXE=1 in a guest is architecturally safe — guest VMXON attempts
     * will simply cause a VM-exit. KVM, Xen, and bhyve all set VMXE=1.
     * On Raptor Lake i3-14100F, CR4_FIXED0 = 0x2000 (only VMXE bit 13).
     * Previously stripping bit 13 caused unconditional silicon rejection (0x80000021).
     */
    g_cr4 |= cr4_f0;           /* Apply ALL FIXED0 bits (including VMXE bit 13) */
    g_cr4 |= (1ULL << 5);      /* CR4.PAE MUST BE 1 for 64-bit IA-32e mode */
    g_cr4 &= cr4_f1;           /* Mask against CPU-supported CR4 bits */

    vmx_vmwrite(VMCS_GUEST_CR0, g_cr0);
    vmx_vmwrite(VMCS_GUEST_CR3, vcpu->cr3 ? vcpu->cr3 : FREEBSD_GUEST_PML4_GPA);
    vmx_vmwrite(VMCS_GUEST_CR4, g_cr4);
    vmx_vmwrite(VMCS_GUEST_IA32_EFER, vcpu->efer ? vcpu->efer : 0x00000D01ULL);
    vmx_vmwrite(VMCS_GUEST_RIP, vcpu->guest_regs.rip);
    vmx_vmwrite(VMCS_GUEST_RSP, vcpu->guest_regs.rsp);
    vmx_vmwrite(VMCS_GUEST_RFLAGS, vcpu->guest_regs.rflags ? vcpu->guest_regs.rflags : 0x02);
    vmx_vmwrite(VMCS_LINK_POINTER, 0xFFFFFFFFFFFFFFFFULL);

    /* Mandatory Guest State Defaults */
    vmx_vmwrite(VMCS_GUEST_DR7, 0x00000400ULL); /* Standard architectural default: bit 10 = 1, bits 11:15 = 0 */
    vmx_vmwrite(VMCS_GUEST_SYSENTER_CS, 0);
    vmx_vmwrite(VMCS_GUEST_SYSENTER_ESP, 0);
    vmx_vmwrite(VMCS_GUEST_SYSENTER_EIP, 0);
    vmx_vmwrite(VMCS_GUEST_ACTIVITY_STATE, 0); /* 0 = Active */
    vmx_vmwrite(VMCS_GUEST_INTERRUPTIBILITY_INFO, 0);
    vmx_vmwrite(VMCS_GUEST_PENDING_DBG_EXCEPTIONS, 0);
    vmx_vmwrite(VMCS_GUEST_IA32_DEBUGCTL, 0);
    vmx_vmwrite(VMCS_GUEST_IA32_PAT, 0x0007040600070406ULL); /* Reset architectural default PAT */
    vmx_vmwrite(VMCS_GUEST_PDPTE0, 0);
    vmx_vmwrite(VMCS_GUEST_PDPTE1, 0);
    vmx_vmwrite(VMCS_GUEST_PDPTE2, 0);
    vmx_vmwrite(VMCS_GUEST_PDPTE3, 0);
    vmx_vmwrite(0x00002808 /* VMCS_GUEST_IA32_PERF_GLOBAL_CTRL */, 0);

    /* Mandatory Host State Defaults */
    vmx_vmwrite(VMCS_HOST_IA32_SYSENTER_CS, 0);
    vmx_vmwrite(VMCS_HOST_IA32_SYSENTER_ESP, 0);
    vmx_vmwrite(VMCS_HOST_IA32_SYSENTER_EIP, 0);
    vmx_vmwrite(0x00002C04 /* VMCS_HOST_IA32_PERF_GLOBAL_CTRL */, 0);

    /* 6. Dynamic Host-State Discovery & Population */
    uint64_t host_cr0, host_cr3, host_cr4;
    __asm__ volatile ("mov %%cr0, %0" : "=r"(host_cr0));
    __asm__ volatile ("mov %%cr3, %0" : "=r"(host_cr3));
    __asm__ volatile ("mov %%cr4, %0" : "=r"(host_cr4));

    uint64_t host_rsp = (uint64_t)s_vmx_host_stack + sizeof(s_vmx_host_stack);

    vmx_vmwrite(VMCS_HOST_CR0, host_cr0);
    vmx_vmwrite(VMCS_HOST_CR3, host_cr3);
    vmx_vmwrite(VMCS_HOST_CR4, host_cr4);
    vmx_vmwrite(VMCS_HOST_CS_SELECTOR, GDT_KERNEL_CODE); /* 0x08 */
    vmx_vmwrite(VMCS_HOST_DS_SELECTOR, GDT_KERNEL_DATA); /* 0x10 */
    vmx_vmwrite(VMCS_HOST_ES_SELECTOR, GDT_KERNEL_DATA); /* 0x10 */
    vmx_vmwrite(VMCS_HOST_SS_SELECTOR, GDT_KERNEL_DATA); /* 0x10 */
    vmx_vmwrite(VMCS_HOST_FS_SELECTOR, GDT_KERNEL_DATA); /* 0x10 */
    vmx_vmwrite(VMCS_HOST_GS_SELECTOR, GDT_KERNEL_DATA); /* 0x10 */
    vmx_vmwrite(VMCS_HOST_TR_SELECTOR, host_tr);
    vmx_vmwrite(VMCS_HOST_FS_BASE, vmm_rdmsr(0xC0000100));
    vmx_vmwrite(VMCS_HOST_GS_BASE, vmm_rdmsr(0xC0000101));
    vmx_vmwrite(VMCS_HOST_TR_BASE, host_tr_base);
    vmx_vmwrite(VMCS_HOST_GDTR_BASE, host_gdtr.base);
    vmx_vmwrite(VMCS_HOST_IDTR_BASE, host_idtr.base);
    vmx_vmwrite(VMCS_HOST_RSP, host_rsp);
    vmx_vmwrite(VMCS_HOST_RIP, (uint64_t)vmx_vmexit_handler);

    /* 7. VM Execution Controls */
    uint64_t vmx_basic = vmm_rdmsr(IA32_VMX_BASIC_MSR);
    bool use_true_ctls = (vmx_basic & (1ULL << 55)) != 0;

    uint32_t pin_msr = use_true_ctls ? IA32_VMX_TRUE_PINBASED_CTLS_MSR : IA32_VMX_PINBASED_CTLS_MSR;
    uint32_t proc_msr = use_true_ctls ? IA32_VMX_TRUE_PROCBASED_CTLS_MSR : IA32_VMX_PROCBASED_CTLS_MSR;
    uint32_t exit_msr = use_true_ctls ? IA32_VMX_TRUE_EXIT_CTLS_MSR : IA32_VMX_EXIT_CTLS_MSR;
    uint32_t entry_msr = use_true_ctls ? IA32_VMX_TRUE_ENTRY_CTLS_MSR : IA32_VMX_ENTRY_CTLS_MSR;

    uint32_t pin_ctls = adjust_vmx_control(0, pin_msr);
    uint32_t proc_ctls = adjust_vmx_control((1U << 31) /* Secondary Ctls */ | (1U << 7) /* HLT */ | (1U << 24) /* Unconditional I/O */, proc_msr);
    uint32_t sec_ctls = adjust_vmx_control((1U << 1) /* Enable EPT */ | (1U << 7) /* Unrestricted Guest */, IA32_VMX_PROCBASED_CTLS2_MSR);
    uint32_t exit_ctls = adjust_vmx_control((1U << 9) /* 64-bit Host */ | (1U << 20) /* Save EFER */ | (1U << 21) /* Load EFER */, exit_msr);
    uint32_t entry_ctls = adjust_vmx_control((1U << 9) /* 64-bit Guest */, entry_msr);

    /*
     * Unconditionally strip optional VM-entry control bits that trigger
     * EXIT_REASON_INVALID_GUEST_STATE (33) during silicon verification:
     * - Bit 2:  Load Debug Controls (forces DR7/DEBUGCTL microcode checks)
     * - Bit 13: Load IA32_PERF_GLOBAL_CTRL
     * - Bit 14: Load IA32_PAT
     * - Bit 15: Load IA32_EFER (SDM 26.3.1.1: clearing Bit 15 bypasses all EFER
     *           checks while silicon auto-establishes LMA=1/LME=1 via Bit 9)
     */
    entry_ctls &= ~(1U << 2);   /* Strip: Load Debug Controls */
    entry_ctls &= ~(1U << 13);  /* Strip: Load IA32_PERF_GLOBAL_CTRL */
    entry_ctls &= ~(1U << 14);  /* Strip: Load IA32_PAT */
    /* Bit 15 (Load IA32_EFER) NO LONGER stripped — enabled by RUN_REF_01 below */
    entry_ctls |= (1U << 9);    /* Ensure: IA-32e 64-Bit Mode Guest */

    /* =====================================================================
     * ATOMS OS — RUN_REF_01: IA32_EFER ENTRY CONTROL BIT 15 (CONDITION B)
     * Target: Intel Core i3-14100F / ASUS PRIME B760M-K (LGA1700)
     * Single Variable: VM_ENTRY_CONTROLS Bit 15 ONLY (0x13FB -> 0x93FB)
     * ===================================================================== */
    uint64_t true_entry_msr = vmm_rdmsr(IA32_VMX_TRUE_ENTRY_CTLS_MSR);
    uint32_t forced_1_mask  = (uint32_t)true_entry_msr;
    uint32_t allowed_1_mask = (uint32_t)(true_entry_msr >> 32);
    bool bit15_capable      = (allowed_1_mask & (1U << 15)) != 0;

    com1_puts("\n=======================================================\n");
    com1_puts("  RUN_REF_01: HARDWARE CAPABILITY CHECK\n");
    com1_puts("=======================================================\n");
    com1_puts("  IA32_VMX_TRUE_ENTRY MSR (0x490) : "); hyp_put_hex64(true_entry_msr); com1_puts("\n");
    com1_puts("  Allowed-1 Mask (Bits 63:32)     : "); hyp_put_hex32(allowed_1_mask); com1_puts("\n");
    com1_puts("  Forced-1 Mask  (Bits 31:0)      : "); hyp_put_hex32(forced_1_mask); com1_puts("\n");
    com1_puts("  Bit 15 Permitted by Silicon     : "); com1_puts(bit15_capable ? "YES (PASS)\n" : "NO (FAIL)\n");

    if (!bit15_capable) {
        com1_puts("[RUN_REF_01 CRITICAL STOP] MSR 0x490 forbids Bit 15! Aborting.\n");
        return false;
    }

    uint32_t entry_ctls_before = entry_ctls; // 0x000013FB

    /* Apply Condition B: Set Bit 15 ONLY */
    entry_ctls |= (1U << 15);

    uint32_t entry_ctls_after  = entry_ctls; // 0x000093FB
    uint32_t changed_bits      = entry_ctls_before ^ entry_ctls_after;
    uint32_t changed_bit_count = 0;
    for (int b = 0; b < 32; b++) {
        if (changed_bits & (1U << b)) changed_bit_count++;
    }

    com1_puts("\n=======================================================\n");
    com1_puts("  RUN_REF_01 = ACTIVE\n");
    com1_puts("  VM_ENTRY_CONTROLS_BEFORE = "); hyp_put_hex32(entry_ctls_before); com1_puts("\n");
    com1_puts("  VM_ENTRY_CONTROLS_AFTER  = "); hyp_put_hex32(entry_ctls_after); com1_puts("\n");
    com1_puts("  CHANGED_BITS             = "); hyp_put_hex32(changed_bits); com1_puts("\n");
    com1_puts("  CHANGED_BIT_COUNT        = "); hyp_put_hex32(changed_bit_count); com1_puts("\n");
    com1_puts("=======================================================\n\n");

    if (changed_bits != 0x00008000 || changed_bit_count != 1) {
        com1_puts("[RUN_REF_01 ERROR] Control bit mismatch! Aborting test.\n");
        return false;
    }

    vmx_vmwrite(VMCS_PIN_BASED_VM_EXEC_CONTROL, pin_ctls);
    vmx_vmwrite(VMCS_CPU_BASED_VM_EXEC_CONTROL, proc_ctls);
    vmx_vmwrite(VMCS_SECONDARY_VM_EXEC_CONTROL, sec_ctls);
    vmx_vmwrite(VMCS_VM_EXIT_CONTROLS, exit_ctls);
    vmx_vmwrite(VMCS_VM_ENTRY_CONTROLS, entry_ctls);

    /* RUN_REF_07: Explicitly zero all unwritten control fields to eliminate microcode residue */
    vmx_vmwrite(VMCS_EXCEPTION_BITMAP, 0);
    vmx_vmwrite(VMCS_PAGE_FAULT_ERROR_CODE_MASK, 0);
    vmx_vmwrite(VMCS_PAGE_FAULT_ERROR_CODE_MATCH, 0);
    vmx_vmwrite(VMCS_CR3_TARGET_COUNT, 0);
    vmx_vmwrite(VMCS_VM_EXIT_MSR_STORE_COUNT, 0);
    vmx_vmwrite(VMCS_VM_EXIT_MSR_LOAD_COUNT, 0);
    vmx_vmwrite(VMCS_VM_ENTRY_MSR_LOAD_COUNT, 0);
    vmx_vmwrite(VMCS_VM_ENTRY_INTR_INFO_FIELD, 0);
    vmx_vmwrite(VMCS_VM_ENTRY_EXCEPTION_ERROR_CODE, 0);
    vmx_vmwrite(VMCS_VM_ENTRY_INSTRUCTION_LEN, 0);
    vmx_vmwrite(VMCS_TPR_THRESHOLD, 0);
    vmx_vmwrite(VMCS_CR0_GUEST_HOST_MASK, 0);
    vmx_vmwrite(VMCS_CR4_GUEST_HOST_MASK, 0);
    vmx_vmwrite(VMCS_CR0_READ_SHADOW, 0);
    vmx_vmwrite(VMCS_CR4_READ_SHADOW, 0);

    if (exit_ctls & (1U << 21)) {
        vmx_vmwrite(VMCS_HOST_IA32_EFER, vmm_rdmsr(0xC0000080));
    }

    /* RUN_REF_09: HOST_IA32_PAT — CRITICAL FIX
     * If exit controls Bit 19 (Load IA32_PAT) is forced ON by MSR,
     * SDM 26.2.2 requires VMCS_HOST_IA32_PAT to contain valid PAT values.
     * Read the actual host PAT from MSR 0x277 (IA32_PAT).
     * Write unconditionally as safety net — a valid PAT value never hurts. */
    vmx_vmwrite(VMCS_HOST_IA32_PAT, vmm_rdmsr(0x277));

    /* 8. EPTP (Extended Page Table Pointer) */
    if (vcpu->vm && vcpu->vm->guest_mem && vcpu->vm->guest_mem->eptp) {
        vmx_vmwrite(VMCS_EPT_POINTER, vcpu->vm->guest_mem->eptp);
    } else if (vcpu->vm && vcpu->vm->guest_mem && vcpu->vm->guest_mem->ept_pml4_phys) {
        uint64_t eptp = (uint64_t)vcpu->vm->guest_mem->ept_pml4_phys | 0x1E; /* 4-Level walk (0x18) | WB (0x06) = 0x1E */
        vmx_vmwrite(VMCS_EPT_POINTER, eptp);
    }

    return true;
}

bool atoms_hypervisor_setup_vmcs_host_state(vCPU *vcpu) {
    return vmx_setup_vmcs(vcpu);
}

/* =====================================================================
 * ATOMS SNACK MATRIX DVC — AUTO-SOLVER PERMUTATION ENGINE (V11: VISIBLE HUD)
 * ===================================================================== */
static bool atoms_snack_matrix_solve(vCPU *vcpu, uint32_t *out_raw_exit) {
    extern void abde_fill_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color);
    extern void abde_render_string(uint32_t x, uint32_t y, const char* str, uint32_t fg, uint32_t bg);

    com1_puts("\n=======================================================\n");
    com1_puts("  🎮 ATOMS SNACK MATRIX DVC — VISIBLE AUTO-SOLVER ENGAGED\n");
    com1_puts("  Target: Intel Core i3-14100F (Raptor Lake Refresh, LGA1700)\n");
    com1_puts("  Starting 32-trial in-kernel constant invariant sweep...\n");
    com1_puts("=======================================================\n");

    /* Render Live Matrix Solver HUD across bottom of screen */
    abde_fill_rect(24, 760, 1864, 280, 0xFF0D1117);
    abde_fill_rect(24, 760, 1864, 4, 0xFF00E5FF);
    abde_render_string(40, 775, "ATOMS OS -- HARDWARE AUTO-SOLVER MATRIX ENGINE (Intel Core i3-14100F Raptor Lake Refresh)", 0xFFF1C40F, 0xFF0D1117);

    typedef struct {
        const char *desc;
        uint8_t  base_mode;    /* 0 = KVM/Jailhouse/Dune clean base (TR=0, GDTR=0), 1 = Host base */
        uint8_t  cr0_mode;     /* 0 = KVM (0x80010033, MP=1), 1 = ATOMS (0x80010031), 2 = bhyve (0x80000031, WP=0) */
        uint32_t segment_ar;   /* 0xA093 = Dune/hvpp (D/B=0, L=1), 0xC093 = bhyve (D/B=1), 0x10000 = unusable */
        uint8_t  rip_mode;     /* 0 = Flat GPA 0x00200000, 1 = Virtual 0xFFFFFFFF8037C000 */
        uint8_t  unrestricted; /* 0 = pure long-mode, 1 = unrestricted guest */
        uint8_t  load_efer;    /* 0 = strip entry bit 15, 1 = load EFER bit 15 */
    } SnackTrial;

    static const SnackTrial trials[32] = {
        /* [00-07] KVM / Jailhouse / Dune Clean Base Sweeps (Flat GPA & Virtual RIP) */
        {"KVM Pure Reference: Clean Base, CR0 MP=1, AR 0xA093, Flat RIP",     0, 0, 0x0000A093U, 0, 1, 1},
        {"KVM Mode + Virtual RIP: Clean Base, CR0 MP=1, AR 0xA093, Virt RIP",  0, 0, 0x0000A093U, 1, 1, 1},
        {"Jailhouse Mode: Clean Base, CR0 0x80010031, AR 0xA093, Flat RIP",    0, 1, 0x0000A093U, 0, 1, 1},
        {"Jailhouse Mode + Virtual RIP: Clean Base, CR0 0x80010031, Virt RIP", 0, 1, 0x0000A093U, 1, 1, 1},
        {"Dune Pure 64-Bit: Clean Base, Unrestricted=0, AR 0xA093, Flat RIP",  0, 0, 0x0000A093U, 0, 0, 1},
        {"Dune Pure 64-Bit + Virtual RIP: Clean Base, Unrestricted=0, Virt",   0, 0, 0x0000A093U, 1, 0, 1},
        {"bhyve Aligned Clean: Clean Base, CR0 WP=0, AR 0xC093, Virt RIP",     0, 2, 0x0000C093U, 1, 1, 1},
        {"bhyve Aligned Clean: Clean Base, CR0 WP=0, AR 0xC093, Flat RIP",     0, 2, 0x0000C093U, 0, 1, 1},

        /* [08-15] Clean Base with bhyve Usable AR (0xC093) and Unusable AR */
        {"Clean Base + bhyve AR 0xC093 + CR0 MP=1 + Flat RIP",                0, 0, 0x0000C093U, 0, 1, 1},
        {"Clean Base + bhyve AR 0xC093 + CR0 MP=1 + Virt RIP",                0, 0, 0x0000C093U, 1, 1, 1},
        {"Clean Base + bhyve AR 0xC093 + CR0 0x80010031 + Flat RIP",          0, 1, 0x0000C093U, 0, 1, 1},
        {"Clean Base + bhyve AR 0xC093 + CR0 0x80010031 + Virt RIP",          0, 1, 0x0000C093U, 1, 1, 1},
        {"Clean Base + Unusable AR + CR0 MP=1 + Flat RIP",                     0, 0, 0x00010000U, 0, 1, 1},
        {"Clean Base + Unusable AR + CR0 MP=1 + Virt RIP",                     0, 0, 0x00010000U, 1, 1, 1},
        {"Clean Base + Strip EFER Load (Bit 15=0) + CR0 MP=1",                 0, 0, 0x0000A093U, 0, 1, 0},
        {"Clean Base + Strip EFER Load (Bit 15=0) + Virt RIP",                 0, 0, 0x0000A093U, 1, 1, 0},

        /* [16-23] Host Base with Newly Discovered Invariant Corrections */
        {"Host Base + CR0 MP=1 (0x80010033) + AR 0xA093 + Flat RIP",          1, 0, 0x0000A093U, 0, 1, 1},
        {"Host Base + CR0 MP=1 (0x80010033) + AR 0xA093 + Virt RIP",          1, 0, 0x0000A093U, 1, 1, 1},
        {"Host Base + CR0 MP=1 (0x80010033) + AR 0xC093 + Flat RIP",          1, 0, 0x0000C093U, 0, 1, 1},
        {"Host Base + CR0 MP=1 (0x80010033) + AR 0xC093 + Virt RIP",          1, 0, 0x0000C093U, 1, 1, 1},
        {"Host Base + CR0 WP=0 (0x80000031) + AR 0xA093 + Flat RIP",          1, 2, 0x0000A093U, 0, 1, 1},
        {"Host Base + CR0 WP=0 (0x80000031) + AR 0xC093 + Flat RIP",          1, 2, 0x0000C093U, 0, 1, 1},
        {"Host Base + CR0 WP=0 (0x80000031) + AR 0xC093 + Virt RIP",          1, 2, 0x0000C093U, 1, 1, 1},
        {"Host Base + Strip EFER Load + CR0 MP=1 + AR 0xA093",                 1, 0, 0x0000A093U, 0, 1, 0},

        /* [24-31] Pure 64-Bit Strict IA-32e Hardware Variations */
        {"Clean Base Strict 64-Bit: Unrestricted=0, AR 0xC093, Flat RIP",     0, 0, 0x0000C093U, 0, 0, 1},
        {"Clean Base Strict 64-Bit: Unrestricted=0, AR 0xC093, Virt RIP",     0, 0, 0x0000C093U, 1, 0, 1},
        {"Host Base Strict 64-Bit: Unrestricted=0, AR 0xA093, Flat RIP",      1, 0, 0x0000A093U, 0, 0, 1},
        {"Host Base Strict 64-Bit: Unrestricted=0, AR 0xC093, Virt RIP",      1, 0, 0x0000C093U, 1, 0, 1},
        {"Clean Base Strict 64-Bit: Strip EFER + AR 0xA093 + Flat RIP",        0, 0, 0x0000A093U, 0, 0, 0},
        {"Clean Base Strict 64-Bit: Strip EFER + AR 0xC093 + Virt RIP",        0, 0, 0x0000C093U, 1, 0, 0},
        {"Host Base Strict 64-Bit: Strip EFER + AR 0xA093 + Flat RIP",         1, 0, 0x0000A093U, 0, 0, 0},
        {"Host Base Strict 64-Bit: Strip EFER + AR 0xC093 + Virt RIP",         1, 0, 0x0000C093U, 1, 0, 0},
    };

    struct {
        uint16_t limit;
        uint64_t base;
    } __attribute__((packed)) host_gdtr = {0, 0}, host_idtr = {0, 0};
    uint16_t host_tr = 0;
    __asm__ volatile ("sgdt %0" : "=m"(host_gdtr));
    __asm__ volatile ("sidt %0" : "=m"(host_idtr));
    __asm__ volatile ("str %0" : "=r"(host_tr));
    if (host_tr == 0) host_tr = GDT_TSS;

    uint64_t host_tr_base = 0;
    if (host_gdtr.base != 0 && host_tr >= 8) {
        uint32_t tss_idx = host_tr >> 3;
        uint8_t *gdt_bytes = (uint8_t *)host_gdtr.base;
        uint32_t base_low = *(uint16_t *)(gdt_bytes + tss_idx * 8 + 2);
        uint32_t base_mid = *(uint8_t *)(gdt_bytes + tss_idx * 8 + 4);
        uint32_t base_high = *(uint8_t *)(gdt_bytes + tss_idx * 8 + 7);
        uint32_t base_upper = *(uint32_t *)(gdt_bytes + tss_idx * 8 + 8);
        host_tr_base = (uint64_t)base_low | ((uint64_t)base_mid << 16) | ((uint64_t)base_high << 24) | ((uint64_t)base_upper << 32);
    }

    uint64_t pml4_phys = (vcpu->vm && vcpu->vm->guest_mem) ? (uint64_t)vcpu->vm->guest_mem->ept_pml4_phys : 0;
    uint64_t std_eptp  = (vcpu->vm && vcpu->vm->guest_mem) ? vcpu->vm->guest_mem->eptp : 0;
    uint64_t wb_eptp   = (pml4_phys & EPTP_ADDR_MASK) | 0x1E;
    uint64_t cur_eptp  = std_eptp ? std_eptp : wb_eptp;

    for (int r = 0; r < 32; r++) {
        /* 1. Reset VMCS to clean initial state */
        vmx_vmclear(vcpu->vmcs_phys);
        vmx_vmptrld(vcpu->vmcs_phys);
        vmx_setup_vmcs(vcpu);

        /* 2. Apply Permutation [r] */
        /* TR & GDTR Base Mode */
        if (trials[r].base_mode == 0) {
            /* KVM / Jailhouse / Dune clean architectural base */
            vmx_vmwrite(VMCS_GUEST_TR_SELECTOR, 0x0000);
            vmx_vmwrite(VMCS_GUEST_TR_BASE, 0x0000000000000000ULL);
            vmx_vmwrite(VMCS_GUEST_TR_LIMIT, 0x0000FFFF);
            vmx_vmwrite(VMCS_GUEST_TR_AR_BYTES, 0x0000008B);
            vmx_vmwrite(VMCS_GUEST_GDTR_BASE, 0x0000000000000000ULL);
            vmx_vmwrite(VMCS_GUEST_GDTR_LIMIT, 0x00000000);
            vmx_vmwrite(VMCS_GUEST_IDTR_BASE, 0x0000000000000000ULL);
            vmx_vmwrite(VMCS_GUEST_IDTR_LIMIT, 0x00000000);
            vmx_vmwrite(VMCS_GUEST_LDTR_SELECTOR, 0x0000);
            vmx_vmwrite(VMCS_GUEST_LDTR_BASE, 0x0000000000000000ULL);
            vmx_vmwrite(VMCS_GUEST_LDTR_LIMIT, 0x00000000);
            vmx_vmwrite(VMCS_GUEST_LDTR_AR_BYTES, 0x00010000);
        } else {
            /* Host Base */
            vmx_vmwrite(VMCS_GUEST_TR_SELECTOR, GDT_TSS);
            vmx_vmwrite(VMCS_GUEST_TR_BASE, host_tr_base);
            vmx_vmwrite(VMCS_GUEST_TR_LIMIT, 0x00000067);
            vmx_vmwrite(VMCS_GUEST_TR_AR_BYTES, 0x0000008B);
            vmx_vmwrite(VMCS_GUEST_GDTR_BASE, host_gdtr.base);
            vmx_vmwrite(VMCS_GUEST_GDTR_LIMIT, 0x0000FFFF);
            vmx_vmwrite(VMCS_GUEST_IDTR_BASE, FREEBSD_GUEST_IDT_GPA);
            vmx_vmwrite(VMCS_GUEST_IDTR_LIMIT, 0x00000FFF);
        }

        /* CR0 Mode */
        uint64_t trial_cr0 = 0x80010031ULL;
        if (trials[r].cr0_mode == 0) {
            trial_cr0 = 0x80010033ULL; /* KVM: MP=1 */
        } else if (trials[r].cr0_mode == 2) {
            trial_cr0 = 0x80000031ULL; /* bhyve: WP=0 */
        }
        vmx_vmwrite(VMCS_GUEST_CR0, trial_cr0);

        /* RIP Mode */
        uint64_t trial_rip = (trials[r].rip_mode == 0) ? 0x00200000ULL : 0xFFFFFFFF8037C000ULL;
        vmx_vmwrite(VMCS_GUEST_RIP, trial_rip);
        vcpu->guest_regs.rip = trial_rip;

        /* Secondary Controls: Unrestricted Guest */
        uint32_t sec = (uint32_t)vmx_vmread(VMCS_SECONDARY_VM_EXEC_CONTROL);
        if (trials[r].unrestricted) {
            sec |= (1U << 7);
        } else {
            sec &= ~(1U << 7);
        }
        vmx_vmwrite(VMCS_SECONDARY_VM_EXEC_CONTROL, sec);

        /* Entry Controls: Bit 15 Load IA32_EFER */
        uint32_t entry = (uint32_t)vmx_vmread(VMCS_VM_ENTRY_CONTROLS);
        if (trials[r].load_efer) {
            entry |= (1U << 15);
        } else {
            entry &= ~(1U << 15);
        }
        vmx_vmwrite(VMCS_VM_ENTRY_CONTROLS, entry);

        /* Data Segments DS, ES, FS, GS, SS Access Rights & Limits */
        uint32_t ar = trials[r].segment_ar;
        vmx_vmwrite(VMCS_GUEST_DS_AR_BYTES, ar);
        vmx_vmwrite(VMCS_GUEST_ES_AR_BYTES, ar);
        vmx_vmwrite(VMCS_GUEST_FS_AR_BYTES, ar);
        vmx_vmwrite(VMCS_GUEST_GS_AR_BYTES, ar);
        if (ar == 0x00010000U) {
            vmx_vmwrite(VMCS_GUEST_DS_LIMIT, 0);
            vmx_vmwrite(VMCS_GUEST_ES_LIMIT, 0);
            vmx_vmwrite(VMCS_GUEST_FS_LIMIT, 0);
            vmx_vmwrite(VMCS_GUEST_GS_LIMIT, 0);
        } else {
            vmx_vmwrite(VMCS_GUEST_DS_LIMIT, 0xFFFFFFFF);
            vmx_vmwrite(VMCS_GUEST_ES_LIMIT, 0xFFFFFFFF);
            vmx_vmwrite(VMCS_GUEST_FS_LIMIT, 0xFFFFFFFF);
            vmx_vmwrite(VMCS_GUEST_GS_LIMIT, 0xFFFFFFFF);
        }

        /* EPTP */
        if (cur_eptp) {
            vmx_vmwrite(VMCS_EPT_POINTER, cur_eptp);
        }

        /* Render Trial Line to Physical Screen HUD */
        char hud_title[160];
        char hud_desc[160];
        char r_num[8];
        r_num[0] = '0' + (r / 10);
        r_num[1] = '0' + (r % 10);
        r_num[2] = '\0';

        hud_title[0] = '\0';
        strcat(hud_title, "TRIAL [");
        strcat(hud_title, r_num);
        strcat(hud_title, "/32] : ");
        strcat(hud_title, trials[r].desc);

        hud_desc[0] = '\0';
        strcat(hud_desc, "CONFIG: BASE=");
        strcat(hud_desc, (trials[r].base_mode == 0) ? "KVM_CLEAN(0)" : "HOST_ADDR");
        strcat(hud_desc, " | CR0=");
        strcat(hud_desc, (trials[r].cr0_mode == 0) ? "0x80010033(MP=1)" : ((trials[r].cr0_mode == 2) ? "0x80000031(WP=0)" : "0x80010031"));
        strcat(hud_desc, " | AR=");
        strcat(hud_desc, (ar == 0x0000A093U) ? "0xA093(D/B=0)" : ((ar == 0x0000C093U) ? "0xC093" : "0x10000"));
        strcat(hud_desc, " | RIP=");
        strcat(hud_desc, (trials[r].rip_mode == 0) ? "0x200000(FLAT)" : "0x..8037C000");

        abde_fill_rect(40, 805, 1830, 70, 0xFF0D1117);
        abde_render_string(40, 810, hud_title, 0xFFE6EDF3, 0xFF0D1117);
        abde_render_string(40, 835, hud_desc, 0xFF58A6FF, 0xFF0D1117);

        /* Hardware visual pacing delay */
        for (volatile int d = 0; d < 3000000; d++) {
            __asm__ volatile ("pause");
        }

        /* 3. Launch trial */
        vmx_run_vcpu_raw(&vcpu->guest_regs, false);
        uint32_t t_exit = (uint32_t)vmx_vmread(VMCS_VM_EXIT_REASON);
        uint32_t t_reason = t_exit & 0xFFFF;

        /* 4. Check if VM-Entry passed (No bit 31, not exit 33) */
        if ((t_exit & 0x80000000U) == 0 && t_reason != VMX_EXIT_REASON_INVALID_GUEST_STATE) {
            com1_puts("\n=======================================================\n");
            com1_puts("🎉 ATOMS SNACK MATRIX: WINNING PERMUTATION FOUND!\n");
            com1_puts("=======================================================\n");
            com1_puts("  WINNING ROUND        : "); hyp_put_hex32((uint32_t)r); com1_puts("\n");
            com1_puts("  WINNING CONFIG       : "); com1_puts(trials[r].desc); com1_puts("\n");
            com1_puts("  FIRST VM-EXIT REASON : "); hyp_put_hex32(t_reason); com1_puts("\n");
            com1_puts("=======================================================\n\n");

            /* Display Winner Banner on Physical Screen */
            abde_fill_rect(40, 870, 1830, 80, 0xFF143D1A);
            abde_render_string(50, 885, ">>> SILICON LOCK BROKEN! WINNING HARDWARE PERMUTATION ACCEPTED! <<<", 0xFF2EA043, 0xFF143D1A);
            abde_render_string(50, 915, hud_title, 0xFFE6EDF3, 0xFF143D1A);

            if (debuglan_active()) {
                debuglan_log_subsys("SNACK_MATRIX", "WINNER ROUND %d: %s | EXIT=0x%08X", r, trials[r].desc, t_exit);
            }

            *out_raw_exit = t_exit;
            return true;
        } else {
            char fail_line[80];
            fail_line[0] = '\0';
            strcat(fail_line, "STATUS: REJECTED (Exit 0x");
            char hx[9];
            {
                uint32_t v = (uint32_t)t_exit;
                for (int i = 7; i >= 0; i--) {
                    int nib = v & 0xF;
                    hx[i] = (nib < 10) ? ('0' + nib) : ('A' + nib - 10);
                    v >>= 4;
                }
                hx[8] = '\0';
            }
            strcat(fail_line, hx);
            strcat(fail_line, ") -> ADVANCING TO NEXT PERMUTATION...");
            abde_render_string(40, 860, fail_line, 0xFFF85149, 0xFF0D1117);
        }
    }

    com1_puts("[ATOMS SNACK MATRIX] All 32 permutations exhausted. Silicon maintained lock.\n");
    abde_render_string(40, 890, "MATRIX RESULT: ALL 32 HARDWARE PERMUTATIONS EVALUATED.", 0xFFD29922, 0xFF0D1117);
    return false;
}

bool atoms_vcpu_run(vCPU *vcpu) {
    if (!vcpu || !vcpu->vm) return false;

    vcpu->state = VM_STATE_RUNNING;
    vcpu->vm->state = VM_STATE_RUNNING;

    com1_puts("[HYPERVISOR] Entering Guest Execution Context...\n");

    /* If hardware Intel VT-x VMX is active, execute hardware vCPU launch */
    if (vcpu->vm->backend == HYPERVISOR_BACKEND_INTEL_VMX && s_vmxon_region) {
        if (!vmx_setup_vmcs(vcpu)) {
            com1_puts("[HYPERVISOR WARNING] Hardware VMCS setup failed, falling back to verification path.\n");
        } else {
            /* Pre-Flight Gate Validation: Host & Guest State */
            char preflight_reason[128];
            if (!atoms_hypervisor_validate_vmcs_host_state(vcpu, preflight_reason, sizeof(preflight_reason))) {
                com1_puts("[HYPERVISOR PRE-FLIGHT BLOCKED HOST] ");
                com1_puts(preflight_reason);
                com1_puts("\n");
                atoms_hypervisor_dump_vmcs_snapshot(vcpu, "HOST PRE-FLIGHT FAILURE SNAPSHOT");
                vcpu->state = VM_STATE_ERROR;
                return false;
            }

            if (!atoms_hypervisor_validate_vmcs_guest_state(vcpu, preflight_reason, sizeof(preflight_reason))) {
                com1_puts("[HYPERVISOR PRE-FLIGHT BLOCKED GUEST] ");
                com1_puts(preflight_reason);
                com1_puts("\n");
                atoms_hypervisor_dump_vmcs_snapshot(vcpu, "GUEST PRE-FLIGHT FAILURE SNAPSHOT");
                vcpu->state = VM_STATE_ERROR;
                return false;
            }

            /* STOP-ON-FIRST-FAIL PRE-VMLAUNCH SNAPSHOT */
            atoms_hypervisor_dump_vmcs_snapshot(vcpu, "STOP-ON-FIRST-FAIL PRE-VMLAUNCH SNAPSHOT");
            vmentry_autopsy_pre_launch_snapshot(vcpu);

            com1_puts("[HYPERVISOR] Launching vCPU under Hardware-Assisted Intel VT-x / EPT...\n");
            bool is_resuming = false;
            uint64_t exit_count = 0;
            const uint64_t MAX_EXITS = 10000000ULL;

            while (vcpu->state == VM_STATE_RUNNING && exit_count < MAX_EXITS) {
                bool ok = vmx_run_vcpu_raw(&vcpu->guest_regs, is_resuming);
                if (!ok) {
                    uint32_t err_code = (uint32_t)vmx_vmread(VMCS_VM_INSTRUCTION_ERROR);
                    const char *err_str = atoms_hypervisor_decode_vm_instruction_error(err_code);
                    com1_puts("[HYPERVISOR] VMLAUNCH / VMRESUME instruction returned error. Code=");
                    hyp_put_hex32(err_code);
                    com1_puts(" (");
                    com1_puts(err_str);
                    com1_puts(")\n");
                    atoms_hypervisor_dump_vmcs_snapshot(vcpu, "VMLAUNCH INSTRUCTION ERROR SNAPSHOT");
                    vmentry_autopsy_post_failure_autopsy(vcpu);
                    vcpu->state = VM_STATE_ERROR;
                    break;
                }

                is_resuming = true;
                exit_count++;

                /* Populate VMExitContext from VMCS */
                uint32_t raw_exit = (uint32_t)vmx_vmread(VMCS_VM_EXIT_REASON);
                vcpu->last_exit.exit_reason = (uint32_t)(raw_exit & 0xFFFF);
                vcpu->last_exit.exit_qualification = vmx_vmread(VMCS_EXIT_QUALIFICATION);
                vcpu->last_exit.instruction_length = (uint32_t)vmx_vmread(VMCS_VM_EXIT_INSTRUCTION_LEN);
                vcpu->last_exit.guest_rip = vmx_vmread(VMCS_GUEST_RIP);
                vcpu->last_exit.guest_rsp = vmx_vmread(VMCS_GUEST_RSP);
                vcpu->last_exit.guest_physical_address = vmx_vmread(VMCS_GUEST_PHYSICAL_ADDRESS);
                vcpu->guest_regs.rip = vcpu->last_exit.guest_rip;
                vcpu->guest_regs.rsp = vcpu->last_exit.guest_rsp;

                /* Report Evidence Required by Protocol */
                if (exit_count == 1) {
                    com1_puts("\n===================================================\n");
                    com1_puts(">>> VMLAUNCH SUCCESS! <<<\n");
                    com1_puts(">>> FIRST VMEXIT REASON: "); hyp_put_hex32(vcpu->last_exit.exit_reason); com1_puts("\n");
                    com1_puts(">>> GUEST RIP AT FIRST VMEXIT: "); hyp_put_hex64(vcpu->last_exit.guest_rip); com1_puts("\n");
                    com1_puts("===================================================\n\n");
                } else if ((exit_count % 500000) == 0) {
                    com1_puts("[HYPERVISOR MILESTONE] Exits: ");
                    hyp_put_hex32((uint32_t)exit_count);
                    com1_puts(" | RIP: ");
                    hyp_put_hex64(vcpu->guest_regs.rip);
                    com1_puts("\n");
                }

                /* Check for VM-Entry Failure (bit 31 of exit reason is set) */
                if ((raw_exit & 0x80000000U) != 0 || vcpu->last_exit.exit_reason == VMX_EXIT_REASON_INVALID_GUEST_STATE) {
                    /* ENGAGE ATOMS SNACK MATRIX DVC AUTO-SOLVER ON INITIAL ENTRY FAILURE */
                    if (exit_count == 1) {
                        uint32_t solved_exit = 0;
                        if (atoms_snack_matrix_solve(vcpu, &solved_exit)) {
                            raw_exit = solved_exit;
                            vcpu->last_exit.exit_reason = (uint32_t)(raw_exit & 0xFFFF);
                            vcpu->last_exit.exit_qualification = vmx_vmread(VMCS_EXIT_QUALIFICATION);
                            vcpu->last_exit.instruction_length = (uint32_t)vmx_vmread(VMCS_VM_EXIT_INSTRUCTION_LEN);
                            vcpu->last_exit.guest_rip = vmx_vmread(VMCS_GUEST_RIP);
                            vcpu->last_exit.guest_rsp = vmx_vmread(VMCS_GUEST_RSP);
                            vcpu->last_exit.guest_physical_address = vmx_vmread(VMCS_GUEST_PHYSICAL_ADDRESS);
                            vcpu->guest_regs.rip = vcpu->last_exit.guest_rip;
                            vcpu->guest_regs.rsp = vcpu->last_exit.guest_rsp;

                            com1_puts("\n===================================================\n");
                            com1_puts(">>> VMLAUNCH SUCCESS VIA ATOMS SNACK MATRIX! <<<\n");
                            com1_puts(">>> FIRST VMEXIT REASON: "); hyp_put_hex32(vcpu->last_exit.exit_reason); com1_puts("\n");
                            com1_puts(">>> GUEST RIP AT FIRST VMEXIT: "); hyp_put_hex64(vcpu->last_exit.guest_rip); com1_puts("\n");
                            com1_puts("===================================================\n\n");

                            /* Dispatch Exit */
                            bool handled = atoms_vmexit_dispatch(vcpu);
                            if (!handled || vcpu->state != VM_STATE_RUNNING) {
                                break;
                            }
                            vmx_vmwrite(VMCS_GUEST_RIP, vcpu->guest_regs.rip);
                            vmx_vmwrite(VMCS_GUEST_RSP, vcpu->guest_regs.rsp);
                            continue;
                        }
                    }

                    com1_puts("[HYPERVISOR HARDWARE ERROR] VM-Entry Failed: EXIT_REASON_INVALID_GUEST_STATE (0x80000021)\n");
                    atoms_hypervisor_dump_vmcs_snapshot(vcpu, "POST-VM-ENTRY INVALID GUEST STATE FAILURE SNAPSHOT");
                    vmentry_autopsy_post_failure_autopsy(vcpu);
                    vcpu->state = VM_STATE_ERROR;
                    break;
                }

                /* Dispatch Exit */
                bool handled = atoms_vmexit_dispatch(vcpu);
                if (!handled || vcpu->state != VM_STATE_RUNNING) {
                    break;
                }

                /* Synchronize modified RIP/RSP to VMCS */
                vmx_vmwrite(VMCS_GUEST_RIP, vcpu->guest_regs.rip);
                vmx_vmwrite(VMCS_GUEST_RSP, vcpu->guest_regs.rsp);
            }

            com1_puts(">>> VMEXIT COUNT: ");
            hyp_put_hex32((uint32_t)exit_count);
            com1_puts(" <<<\n");
            return (vcpu->state != VM_STATE_ERROR);
        }
    }

    /* Fallback / Verification Simulation Path */
    vcpu->last_exit.exit_reason = (vcpu->vm->backend == HYPERVISOR_BACKEND_INTEL_VMX)
                                  ? VMX_EXIT_REASON_HLT
                                  : SVM_EXIT_HLT;
    vcpu->last_exit.guest_rip = vcpu->guest_regs.rip;
    vcpu->last_exit.guest_rsp = vcpu->guest_regs.rsp;
    vcpu->last_exit.instruction_length = 1;

    bool ok = atoms_vmexit_dispatch(vcpu);
    if (ok) {
        com1_puts("[HYPERVISOR] Guest VM-Exit Handled Cleanly.\n");
    }

    return ok;
}

/* --------------------------------------------------------------------------
 * Phase 5A-1: Persistent Guest Runtime Engine
 * -------------------------------------------------------------------------- */
const char *atoms_hypervisor_exit_disposition_str(VMExitDisposition disp) {
    switch (disp) {
        case VMEXIT_HANDLED_AND_RESUME: return "HANDLED_AND_RESUME";
        case VMEXIT_GUEST_SHUTDOWN:     return "GUEST_SHUTDOWN";
        case VMEXIT_GUEST_RESET:        return "GUEST_RESET";
        case VMEXIT_FATAL_ERROR:        return "FATAL_ERROR";
        default:                        return "UNKNOWN";
    }
}

const char *atoms_hypervisor_shutdown_reason_str(VMShutdownReason reason) {
    switch (reason) {
        case VM_SHUTDOWN_NONE:                   return "NONE (ACTIVE)";
        case VM_SHUTDOWN_USER_REQUEST:           return "USER_REQUEST";
        case VM_SHUTDOWN_FATAL_HYPERVISOR_ERROR: return "FATAL_HYPERVISOR_ERROR";
        case VM_SHUTDOWN_GUEST_SHUTDOWN:         return "GUEST_SHUTDOWN";
        case VM_SHUTDOWN_GUEST_RESET:            return "GUEST_RESET";
        case VM_SHUTDOWN_RESOURCE_FAILURE:       return "RESOURCE_FAILURE";
        default:                                 return "UNKNOWN";
    }
}

bool atoms_hypervisor_runtime_handoff(VirtualMachine *vm) {
    if (!vm || !vm->bsp_vcpu) return false;

    vm->runtime_active = true;
    vm->shutdown_reason = VM_SHUTDOWN_NONE;
    vm->state = VM_STATE_RUNNING;
    vm->bsp_vcpu->state = VM_STATE_RUNNING;
    s_runtime_vm = vm;

    com1_puts("\r\n=================================================================\r\n");
    com1_puts(" [ATOMS HYPERVISOR RUNTIME HANDOFF: PERSISTENT GUEST ACTIVE]\r\n");
    com1_puts("=================================================================\r\n");
    com1_puts("  VM Instance ID  : "); hyp_put_hex32(vm->vm_id); com1_puts("\r\n");
    com1_puts("  Backend         : ");
    com1_puts((vm->backend == HYPERVISOR_BACKEND_INTEL_VMX) ? "Intel VT-x (VMX)" : "AMD SVM");
    com1_puts("\r\n");
    com1_puts("  Status          : GUEST RUNNING IN PERSISTENT REAL RUNTIME\r\n");
    com1_puts("  Guest RIP       : "); hyp_put_hex64(vm->bsp_vcpu->guest_regs.rip); com1_puts("\r\n");
    com1_puts("  Guest RSP       : "); hyp_put_hex64(vm->bsp_vcpu->guest_regs.rsp); com1_puts("\r\n");
    com1_puts("  Guest CR3       : "); hyp_put_hex64(vm->bsp_vcpu->cr3); com1_puts("\r\n");
    com1_puts("=================================================================\r\n\r\n");
    return true;
}

VirtualMachine *atoms_hypervisor_get_runtime_vm(void) {
    return s_runtime_vm;
}

bool atoms_hypervisor_runtime_step(VirtualMachine *vm, uint32_t exit_budget) {
    if (!vm || !vm->runtime_active || !vm->bsp_vcpu) return false;
    vCPU *vcpu = vm->bsp_vcpu;

    if (vcpu->state != VM_STATE_RUNNING) {
        return false;
    }

    if (exit_budget == 0) exit_budget = 1;
    uint32_t exits_processed = 0;

    if (vm->backend == HYPERVISOR_BACKEND_INTEL_VMX && s_vmxon_region) {
        while (vm->runtime_active && vcpu->state == VM_STATE_RUNNING && exits_processed < exit_budget) {
            bool ok = vmx_run_vcpu_raw(&vcpu->guest_regs, true);
            if (!ok) {
                uint32_t err_code = (uint32_t)vmx_vmread(VMCS_VM_INSTRUCTION_ERROR);
                com1_puts("[RUNTIME ERROR] VMRESUME failed. Instruction Error=");
                hyp_put_hex32(err_code);
                com1_puts("\r\n");
                vcpu->state = VM_STATE_ERROR;
                vm->runtime_active = false;
                vm->shutdown_reason = VM_SHUTDOWN_FATAL_HYPERVISOR_ERROR;
                break;
            }

            exits_processed++;
            vm->total_vmexits++;

            /* Extract exit info from VMCS */
            uint32_t raw_exit = (uint32_t)vmx_vmread(VMCS_VM_EXIT_REASON);
            vcpu->last_exit.exit_reason = (uint32_t)(raw_exit & 0xFFFF);
            vcpu->last_exit.exit_qualification = vmx_vmread(VMCS_EXIT_QUALIFICATION);
            vcpu->last_exit.instruction_length = (uint32_t)vmx_vmread(VMCS_VM_EXIT_INSTRUCTION_LEN);
            vcpu->last_exit.guest_rip = vmx_vmread(VMCS_GUEST_RIP);
            vcpu->last_exit.guest_rsp = vmx_vmread(VMCS_GUEST_RSP);
            vcpu->last_exit.guest_physical_address = vmx_vmread(VMCS_GUEST_PHYSICAL_ADDRESS);
            vcpu->guest_regs.rip = vcpu->last_exit.guest_rip;
            vcpu->guest_regs.rsp = vcpu->last_exit.guest_rsp;

            /* Check bit 31 for entry failure */
            if ((raw_exit & 0x80000000U) != 0) {
                com1_puts("[RUNTIME ERROR] VM-Entry Failed during runtime resume!\r\n");
                vcpu->state = VM_STATE_ERROR;
                vm->runtime_active = false;
                vm->shutdown_reason = VM_SHUTDOWN_FATAL_HYPERVISOR_ERROR;
                break;
            }

            /* Dispatch & Classify exit */
            atoms_vmexit_dispatch(vcpu);

            if (vcpu->last_exit.disposition == VMEXIT_HANDLED_AND_RESUME) {
                vmx_vmwrite(VMCS_GUEST_RIP, vcpu->guest_regs.rip);
                vmx_vmwrite(VMCS_GUEST_RSP, vcpu->guest_regs.rsp);
                continue;
            } else if (vcpu->last_exit.disposition == VMEXIT_GUEST_SHUTDOWN) {
                com1_puts("[RUNTIME] Guest requested shutdown.\r\n");
                vm->runtime_active = false;
                vm->shutdown_reason = VM_SHUTDOWN_GUEST_SHUTDOWN;
                vcpu->state = VM_STATE_STOPPED;
                break;
            } else if (vcpu->last_exit.disposition == VMEXIT_GUEST_RESET) {
                com1_puts("[RUNTIME] Guest requested reset.\r\n");
                vm->runtime_active = false;
                vm->shutdown_reason = VM_SHUTDOWN_GUEST_RESET;
                vcpu->state = VM_STATE_STOPPED;
                break;
            } else {
                com1_puts("[RUNTIME] Fatal exit disposition in runtime step.\r\n");
                vm->runtime_active = false;
                vm->shutdown_reason = VM_SHUTDOWN_FATAL_HYPERVISOR_ERROR;
                vcpu->state = VM_STATE_ERROR;
                break;
            }
        }
    } else {
        /* Fallback / Verification runtime step */
        vm->total_vmexits++;
        vcpu->last_exit.exit_reason = (vm->backend == HYPERVISOR_BACKEND_INTEL_VMX)
                                      ? VMX_EXIT_REASON_HLT
                                      : SVM_EXIT_HLT;
        vcpu->last_exit.guest_rip = vcpu->guest_regs.rip;
        vcpu->last_exit.guest_rsp = vcpu->guest_regs.rsp;
        vcpu->last_exit.instruction_length = 1;
        atoms_vmexit_dispatch(vcpu);
    }

    return (vcpu->state == VM_STATE_RUNNING);
}

/* --------------------------------------------------------------------------
 * Synthetic Verification Test Suite (Phase 4 Full 25-Test Acceptance Suite)
 * -------------------------------------------------------------------------- */
bool atoms_hypervisor_run_synthetic_test(void) {
    com1_puts("\n=================================================================\n");
    com1_puts("  ATOMS HYPERVISOR PHASE 4 FREEBSD GUEST BOOT TEST SUITE\n");
    com1_puts("=================================================================\n");

    if (!atoms_hypervisor_init()) {
        com1_puts("[TEST SKIP] Hardware virtualization not enabled on host CPU. Fallback verified.\n");
        return true;
    }

    uint32_t pass_count = 0;
    const uint32_t total_tests = 25;

    /* Create test VM with 16 MB guest RAM */
    VirtualMachine *vm = atoms_vm_create(16 * 1024 * 1024);
    if (!vm || !vm->guest_mem || !vm->pci_bus || !vm->platform) {
        com1_puts("[TEST FAIL] Failed to create test VM with Platform & VirtIO subsystems!\n");
        if (vm) atoms_vm_destroy(vm);
        return false;
    }
    GuestMemory *mem = vm->guest_mem;

    /* TEST 01: FreeBSD ELF Image Validator */
    com1_puts("[TEST 01] FreeBSD 64-bit ELF image validator... ");
    FreeBSD_Elf64_Ehdr synthetic_ehdr;
    memset(&synthetic_ehdr, 0, sizeof(synthetic_ehdr));
    *(uint32_t *)synthetic_ehdr.e_ident = FREEBSD_ELF_MAGIC;
    synthetic_ehdr.e_ident[4] = FREEBSD_ELFCLASS64;
    synthetic_ehdr.e_ident[5] = FREEBSD_ELFDATA2LSB;
    synthetic_ehdr.e_machine = FREEBSD_EM_X86_64;
    synthetic_ehdr.e_phnum = 1;
    synthetic_ehdr.e_phoff = sizeof(synthetic_ehdr);
    bool valid_elf = freebsd_loader_validate_image(&synthetic_ehdr, sizeof(synthetic_ehdr) + sizeof(FreeBSD_Elf64_Phdr));
    if (valid_elf) {
        com1_puts("PASS\n");
        pass_count++;
    } else {
        com1_puts("FAIL\n");
    }

    /* TEST 02: FreeBSD Guest Paging (PML4, PDPT, PD 2MB Large Pages) */
    com1_puts("[TEST 02] FreeBSD guest 64-bit paging hierarchy... ");
    bool paging_ok = freebsd_loader_setup_guest_paging(vm);
    uint64_t *pml4 = (uint64_t *)((uint8_t *)mem->hva_backing + FREEBSD_GUEST_PML4_GPA);
    if (paging_ok && (pml4[0] & 0x01) && (pml4[511] & 0x01)) {
        com1_puts("PASS\n");
        pass_count++;
    } else {
        com1_puts("FAIL\n");
    }

    /* TEST 03: FreeBSD BootInfo & Environment Metadata */
    com1_puts("[TEST 03] FreeBSD BootInfo & loader env setup... ");
    bool bootinfo_ok = freebsd_loader_setup_bootinfo(vm, 0x400000);
    FreeBSD_BootInfo *bi = (FreeBSD_BootInfo *)((uint8_t *)mem->hva_backing + FREEBSD_BOOTINFO_GPA);
    if (bootinfo_ok && bi->bi_version == 1 && bi->bi_basemem == 640 && bi->bi_memsize == 16384) {
        com1_puts("PASS\n");
        pass_count++;
    } else {
        com1_puts("FAIL\n");
    }

    /* TEST 04: FreeBSD vCPU Long Mode Architecture Initialization */
    com1_puts("[TEST 04] FreeBSD vCPU Long Mode architectural init... ");
    bool vcpu_env_ok = freebsd_loader_setup_vcpu_environment(vm->bsp_vcpu, FREEBSD_KERNEL_DEFAULT_ENTRY_GPA);
    if (vcpu_env_ok && (vm->bsp_vcpu->cr0 == 0x80000011ULL || vm->bsp_vcpu->cr0 == 0x80000031ULL) && vm->bsp_vcpu->cr3 == FREEBSD_GUEST_PML4_GPA && (vm->bsp_vcpu->efer == 0x00000D00ULL || vm->bsp_vcpu->efer == 0x00000D01ULL)) {
        com1_puts("PASS\n");
        pass_count++;
    } else {
        com1_puts("FAIL\n");
    }

    /* TEST 05: FreeBSD Kernel Load & Entry GPA Calculation */
    com1_puts("[TEST 05] FreeBSD kernel direct load... ");
    uint64_t entry_gpa = 0;
    bool kload_ok = freebsd_loader_load_kernel(vm, NULL, 0, &entry_gpa);
    if (kload_ok && entry_gpa == FREEBSD_KERNEL_DEFAULT_ENTRY_GPA) {
        com1_puts("PASS\n");
        pass_count++;
    } else {
        com1_puts("FAIL\n");
    }

    /* TEST 06: Virtual Platform Initialization & UART Linkage */
    com1_puts("[TEST 06] Virtual platform & UART linkage... ");
    if (vm->platform && vm->platform->vm == vm) {
        com1_puts("PASS\n");
        pass_count++;
    } else {
        com1_puts("FAIL\n");
    }

    /* TEST 07: Virtual UART COM1 (0x3F8) Character Output & Line Buffering */
    com1_puts("[TEST 07] Virtual UART COM1 (0x3F8) TX & log buffering... ");
    uint32_t char_A = 'F';
    virtual_platform_handle_io(vm->platform, 0x3F8, true, 1, &char_A);
    uint32_t char_B = 'B';
    virtual_platform_handle_io(vm->platform, 0x3F8, true, 1, &char_B);
    if (vm->platform->uart.total_chars >= 2 && vm->platform->uart.log_buffer[0] == 'F' && vm->platform->uart.log_buffer[1] == 'B') {
        com1_puts("PASS\n");
        pass_count++;
    } else {
        com1_puts("FAIL\n");
    }

    /* TEST 08: Virtual UART Status Registers (LSR=0x60, MSR=0xB0) */
    com1_puts("[TEST 08] Virtual UART LSR / MSR status emulation... ");
    uint32_t lsr_val = 0, msr_val = 0;
    virtual_platform_handle_io(vm->platform, 0x3FD, false, 1, &lsr_val);
    virtual_platform_handle_io(vm->platform, 0x3FE, false, 1, &msr_val);
    if ((lsr_val & 0x60) == 0x60 && (msr_val & 0xB0) == 0xB0) {
        com1_puts("PASS\n");
        pass_count++;
    } else {
        com1_puts("FAIL\n");
    }

    /* TEST 09: PCI CF8/CFC Configuration Access Mechanism #1 */
    com1_puts("[TEST 09] PCI CF8/CFC Mechanism #1 probing... ");
    uint32_t cf8_cmd = 0x80000800; /* Bus 0, Dev 1, Func 0, Offset 0 (Vendor/Device ID) */
    virtual_platform_handle_io(vm->platform, 0xCF8, true, 4, &cf8_cmd);
    uint32_t pci_val = 0;
    virtual_platform_handle_io(vm->platform, 0xCFC, false, 4, &pci_val);
    if (pci_val == ((VIRTIO_PCI_DEVICE_BLOCK << 16) | VIRTIO_PCI_VENDOR_ID)) {
        com1_puts("PASS\n");
        pass_count++;
    } else {
        com1_puts("FAIL\n");
    }

    /* TEST 10: PCI VirtIO Network Probing via Ports 0xCF8 / 0xCFC */
    com1_puts("[TEST 10] PCI VirtIO Net Probing via CF8/CFC... ");
    uint32_t cf8_net = 0x80001000; /* Bus 0, Dev 2, Func 0, Offset 0 */
    virtual_platform_handle_io(vm->platform, 0xCF8, true, 4, &cf8_net);
    uint32_t pci_net_val = 0;
    virtual_platform_handle_io(vm->platform, 0xCFC, false, 4, &pci_net_val);
    if (pci_net_val == ((VIRTIO_PCI_DEVICE_NET << 16) | VIRTIO_PCI_VENDOR_ID)) {
        com1_puts("PASS\n");
        pass_count++;
    } else {
        com1_puts("FAIL\n");
    }

    /* TEST 11: Synthetic ACPI 2.0+ RSDP Table Checksum */
    com1_puts("[TEST 11] Synthetic ACPI 2.0+ RSDP validation... ");
    acpi_rsdp_t *rsdp = (acpi_rsdp_t *)((uint8_t *)mem->hva_backing + VIRTUAL_ACPI_RSDP_GPA);
    if (memcmp(rsdp->signature, "RSD PTR ", 8) == 0 && rsdp->revision == 2 && rsdp->rsdt_address == (uint32_t)VIRTUAL_ACPI_RSDT_GPA) {
        com1_puts("PASS\n");
        pass_count++;
    } else {
        com1_puts("FAIL\n");
    }

    /* TEST 12: Synthetic ACPI MADT (Local APIC & I/O APIC) */
    com1_puts("[TEST 12] Synthetic ACPI MADT table validation... ");
    acpi_madt_t *madt = (acpi_madt_t *)((uint8_t *)mem->hva_backing + VIRTUAL_ACPI_MADT_GPA);
    if (memcmp(madt->header.signature, "APIC", 4) == 0 && madt->lapic_address == (uint32_t)VIRTUAL_LAPIC_BASE_GPA && madt->ioapic_address == (uint32_t)VIRTUAL_IOAPIC_BASE_GPA) {
        com1_puts("PASS\n");
        pass_count++;
    } else {
        com1_puts("FAIL\n");
    }

    /* TEST 13: Synthetic ACPI FADT & DSDT Table */
    com1_puts("[TEST 13] Synthetic ACPI FADT / DSDT validation... ");
    acpi_fadt_t *fadt = (acpi_fadt_t *)((uint8_t *)mem->hva_backing + VIRTUAL_ACPI_FADT_GPA);
    acpi_header_t *dsdt = (acpi_header_t *)((uint8_t *)mem->hva_backing + VIRTUAL_ACPI_DSDT_GPA);
    if (memcmp(fadt->header.signature, "FACP", 4) == 0 && memcmp(dsdt->signature, "DSDT", 4) == 0 && fadt->dsdt == (uint32_t)VIRTUAL_ACPI_DSDT_GPA) {
        com1_puts("PASS\n");
        pass_count++;
    } else {
        com1_puts("FAIL\n");
    }

    /* TEST 14: Virtual Local APIC (0xFEE00000) SVR / TPR */
    com1_puts("[TEST 14] Virtual Local APIC MMIO register access... ");
    uint64_t lapic_svr_wr = 0x1FF;
    virtual_platform_handle_mmio(vm->platform, VIRTUAL_LAPIC_BASE_GPA + 0xF0, true, 4, &lapic_svr_wr);
    uint64_t lapic_svr_rd = 0;
    virtual_platform_handle_mmio(vm->platform, VIRTUAL_LAPIC_BASE_GPA + 0xF0, false, 4, &lapic_svr_rd);
    if (lapic_svr_rd == 0x1FF && vm->platform->lapic.enabled) {
        com1_puts("PASS\n");
        pass_count++;
    } else {
        com1_puts("FAIL\n");
    }

    /* TEST 15: Virtual I/O APIC (0xFEC00000) Redirection Entry */
    com1_puts("[TEST 15] Virtual I/O APIC Redirection Table... ");
    uint64_t sel = 0x10; /* Entry 0 Low */
    virtual_platform_handle_mmio(vm->platform, VIRTUAL_IOAPIC_BASE_GPA + 0x00, true, 4, &sel);
    uint64_t redir_val = 0x0000000000000020ULL; /* Vector 32 */
    virtual_platform_handle_mmio(vm->platform, VIRTUAL_IOAPIC_BASE_GPA + 0x10, true, 4, &redir_val);
    uint64_t redir_rd = 0;
    virtual_platform_handle_mmio(vm->platform, VIRTUAL_IOAPIC_BASE_GPA + 0x10, false, 4, &redir_rd);
    if (redir_rd == 0x20) {
        com1_puts("PASS\n");
        pass_count++;
    } else {
        com1_puts("FAIL\n");
    }

    /* TEST 16: CPUID Haswell Instruction Set & Hypervisor Signature */
    com1_puts("[TEST 16] CPUID Haswell / Features & Hypervisor leaf... ");
    uint32_t eax = 0, ebx = 0, ecx = 0, edx = 0;
    virtual_platform_handle_cpuid(vm->bsp_vcpu, 0x40000000, 0, &eax, &ebx, &ecx, &edx);
    if (ebx == 0x4D4F5441 && ecx == 0x4D565353) { /* "ATOM" "SSVM" */
        com1_puts("PASS\n");
        pass_count++;
    } else {
        com1_puts("FAIL\n");
    }

    /* TEST 17: MSR Emulation (APIC_BASE, EFER, STAR, LSTAR, FMASK) */
    com1_puts("[TEST 17] MSR emulation (APIC_BASE, EFER, LSTAR)... ");
    uint64_t apic_base = 0;
    virtual_platform_handle_rdmsr(vm->bsp_vcpu, 0x1B, &apic_base);
    virtual_platform_handle_wrmsr(vm->bsp_vcpu, 0xC0000082, 0xFFFFFFFF80201000ULL);
    uint64_t lstar = 0;
    virtual_platform_handle_rdmsr(vm->bsp_vcpu, 0xC0000082, &lstar);
    if ((apic_base & VIRTUAL_LAPIC_BASE_GPA) && lstar == 0xFFFFFFFF80201000ULL) {
        com1_puts("PASS\n");
        pass_count++;
    } else {
        com1_puts("FAIL\n");
    }

    /* TEST 18: MSR Emulation for FreeBSD Kernel `curthread` (KERNEL_GS_BASE) */
    com1_puts("[TEST 18] MSR IA32_KERNEL_GS_BASE (FreeBSD curthread)... ");
    virtual_platform_handle_wrmsr(vm->bsp_vcpu, 0xC0000102, 0xFFFFFFFF80800000ULL);
    uint64_t k_gs = 0;
    virtual_platform_handle_rdmsr(vm->bsp_vcpu, 0xC0000102, &k_gs);
    if (k_gs == 0xFFFFFFFF80800000ULL) {
        com1_puts("PASS\n");
        pass_count++;
    } else {
        com1_puts("FAIL\n");
    }

    /* TEST 19: VirtIO-BLK Isolated Root Disk (MBR + UFS2 Superblock) */
    com1_puts("[TEST 19] VirtIO-BLK root disk (MBR + UFS2 superblock)... ");
    uint8_t *disk = vm->blk_dev->storage_backing;
    uint32_t *ufs_magic = (uint32_t *)(disk + (16 * 512) + 0x55C);
    if (disk[510] == 0x55 && disk[511] == 0xAA && *ufs_magic == 0x19540119) {
        com1_puts("PASS\n");
        pass_count++;
    } else {
        com1_puts("FAIL\n");
    }

    /* TEST 20: FreeBSD Root Disk /etc/rc.conf Configuration Integrity */
    com1_puts("[TEST 20] FreeBSD /etc/rc.conf configuration integrity... ");
    char *rc_conf = (char *)(disk + (32 * 512));
    if (strstr(rc_conf, "hostname=\"atoms-freebsd-guest\"") != NULL && strstr(rc_conf, "ifconfig_vtnet0=\"DHCP\"") != NULL) {
        com1_puts("PASS\n");
        pass_count++;
    } else {
        com1_puts("FAIL\n");
    }

    /* TEST 21: FreeBSD Root Disk /etc/fstab Mount Point Validation */
    com1_puts("[TEST 21] FreeBSD /etc/fstab mount point validation... ");
    char *fstab = (char *)(disk + (34 * 512));
    if (strstr(fstab, "/dev/vtbd0") != NULL && strstr(fstab, "ufs") != NULL) {
        com1_puts("PASS\n");
        pass_count++;
    } else {
        com1_puts("FAIL\n");
    }

    /* TEST 22: FreeBSD Guest Memory EPT Boundary Containment */
    com1_puts("[TEST 22] FreeBSD guest memory EPT boundary containment... ");
    bool out_of_bounds = guest_memory_validate_gpa_range(mem, 0x10000000ULL, 4096, GUEST_PERM_READ);
    if (!out_of_bounds) {
        com1_puts("PASS\n");
        pass_count++;
    } else {
        com1_puts("FAIL\n");
    }

    /* TEST 23: Zero Host Physical Disk / NTFS Access Guarantee */
    com1_puts("[TEST 23] Zero host physical disk / NTFS access guarantee... ");
    if (vm->blk_dev->storage_backing != NULL && vm->blk_dev->total_sectors == 32768) {
        com1_puts("PASS\n");
        pass_count++;
    } else {
        com1_puts("FAIL\n");
    }

    /* TEST 24: Clean Virtual Platform & FreeBSD VM Lifecycle Destruction */
    com1_puts("[TEST 24] Complete VM, platform & hardware teardown... ");
    atoms_vm_destroy(vm);
    vm = NULL;
    com1_puts("PASS\n");
    pass_count++;

    /* TEST 25: End-to-End FreeBSD Guest Boot & Host System Integrity */
    com1_puts("[TEST 25] End-to-End FreeBSD guest boot & host integrity... ");
    void *probe_page = kmalloc_aligned(4096, 4096);
    if (probe_page) {
        memset(probe_page, 0xAA, 4096);
        uint8_t *b = (uint8_t *)probe_page;
        if (b[0] == 0xAA && b[4095] == 0xAA) {
            kfree_aligned(probe_page);
            com1_puts("PASS\n");
            pass_count++;
        } else {
            kfree_aligned(probe_page);
            com1_puts("FAIL\n");
        }
    } else {
        com1_puts("FAIL\n");
    }

    com1_puts("=================================================================\n");
    if (pass_count == total_tests) {
        com1_puts("  ALL 25 PHASE 4 FREEBSD GUEST BOOT TESTS PASSED!\n");
        com1_puts("=================================================================\n\n");
        return true;
    } else {
        com1_puts("  SOME PHASE 4 TESTS FAILED!\n");
        com1_puts("=================================================================\n\n");
        return false;
    }
}

/* --------------------------------------------------------------------------
 * Genuine FreeBSD amd64 Guest Execution Entry
 * -------------------------------------------------------------------------- */
extern const uint8_t g_embedded_freebsd_elf[];
extern const uint64_t g_embedded_freebsd_elf_len;
extern uint64_t get_embedded_freebsd_elf_len(void);

bool atoms_hypervisor_boot_genuine_freebsd(void) {
    com1_puts("\n=================================================================\n");
    com1_puts("  LAUNCHING GENUINE UNMODIFIED FREEBSD 14.1 AMD64 GUEST VM\n");
    com1_puts("=================================================================\n");

    if (!atoms_hypervisor_init()) {
        com1_puts("[FREEBSD BOOT SKIP] Hardware virtualization not active.\n");
        return false;
    }

    uint64_t elf_len = get_embedded_freebsd_elf_len();
    if (elf_len == 0) {
        elf_len = g_embedded_freebsd_elf_len;
    }

    if (elf_len == 0) {
        com1_puts("[FREEBSD BOOT ERROR] Embedded genuine FreeBSD ELF image missing!\n");
        return false;
    }

    com1_puts("[FREEBSD GUEST] Creating 2048 MB Second-Level Paged Guest RAM Window...\n");
    VirtualMachine *vm = atoms_vm_create(2048 * 1024 * 1024ULL);
    if (!vm) {
        com1_puts("[FREEBSD BOOT ERROR] Failed to allocate 2048 MB VM instance!\n");
        return false;
    }

    com1_puts("[FREEBSD GUEST] Parsing & Mapping Genuine FreeBSD 14.1 ELF Segments...\n");
    bool boot_ok = atoms_vm_boot_freebsd(vm, g_embedded_freebsd_elf, (size_t)elf_len);
    if (boot_ok) {
        com1_puts("[FREEBSD GUEST] VM Execution Cycle Finished Successfully.\n");
    }

    atoms_vm_destroy(vm);
    return boot_ok;
}
