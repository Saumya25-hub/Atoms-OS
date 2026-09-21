# ATOMS OS — Lightweight Native Micro-Hypervisor
## Phase 1: VMX/SVM CPU Virtualization & Micro-Hypervisor Core Architecture

---

## 1. Executive Summary & Design Philosophy

The **ATOMS Micro-Hypervisor** is an integrated Ring 0 virtualization subsystem within the native **BOS Kernel**. Its purpose is to provide hardware-assisted virtualization to host isolated guest operating system environments (specifically targeted for an isolated, headless FreeBSD guest container to run modern web browsers such as Chromium/Brave/Chrome) without sacrificing the independence, security, or native execution model of ATOMS OS.

```text
+-----------------------------------------------------------------------+
|                              ATOMS OS                                 |
|  (BWE Desktop / BCM Compositor / Native Ring 3 C/C++/Java Userspace)  |
+-----------------------------------------------------------------------+
                                  │
                                  ▼
+-----------------------------------------------------------------------+
|                             BOS Kernel                                |
|  (PMM, VMM, Scheduler, VFS/BOFS, AHME, DGL, Network, Audio, USB)     |
+-----------------------------------------------------------------------+
                                  │
                                  ▼
+-----------------------------------------------------------------------+
|                    ATOMS Native Micro-Hypervisor                      |
|  - CPU Virtualization Detection (VMX / SVM)                           |
|  - Intel VMX Root Operation & VMCS Abstraction                        |
|  - AMD SVM Operation & VMCB Abstraction                               |
|  - Unified vCPU & Virtual Machine Lifecycle Manager                   |
|  - Central VM-Exit Dispatcher & Instruction Emulation                 |
+-----------------------------------------------------------------------+
                                  │
                                  ▼
+-----------------------------------------------------------------------+
|                 Isolated Guest Container (Future Phases)              |
|  [Phase 2: EPT/NPT Paging] ➔ [Phase 3: VirtIO Devices] ➔ [FreeBSD]   |
+-----------------------------------------------------------------------+
```

### Core Invariants & Boundaries
1. **ATOMS OS Independence**: ATOMS OS is NOT Linux, NOT Linux-based, and NOT FreeBSD-based. The native kernel is and remains the **BOS Kernel**.
2. **Guest Isolation**: The guest OS (FreeBSD) runs entirely within a hardware-confined virtual machine (VMX non-root operation / SVM guest mode). It has zero access to host physical memory or hardware control registers outside its allocated partition.
3. **Phase 1 Boundary**: Focuses strictly on CPU virtualization capability detection, hypervisor host mode initialization, VMCS/VMCB abstraction structures, vCPU/VM lifecycle management, the central VM-exit dispatcher, and synthetic self-tests. Zero EPT/NPT, zero VirtIO devices, zero guest OS image loading, and zero browser code are included in this phase.

---

## 2. Hardware Virtualization Architecture

The hypervisor abstracts vendor-specific CPU virtualization instructions behind a unified hardware abstraction layer (HAL).

```text
                     +───────────────────────────────────+
                     |       Unified Hypervisor API      |
                     |  atoms_hypervisor_init()          |
                     |  atoms_vm_create() / run()        |
                     +───────────────────────────────────+
                                       │
                    ┌──────────────────┴──────────────────┐
                    ▼                                     ▼
        +───────────────────────+             +───────────────────────+
        |   Intel VT-x / VMX    |             |        AMD SVM        |
        | - CPUID.1:ECX.VMX[5]  |             | - CPUID.80000001:SVM  |
        | - IA32_FEATURE_CONTROL|             | - EFER.SVME (Bit 12)  |
        | - CR4.VMXE (Bit 13)   |             | - VM_HSAVE_PA MSR     |
        | - VMXON / VMXOFF      |             | - VMRUN / VMSAVE      |
        | - VMPTRLD / VMWRITE   |             | - VMLOAD / VMMCALL    |
        | - VMLAUNCH / VMRESUME |             | - VMCB Control/State  |
        +───────────────────────+             +───────────────────────+
```

### 2.1 Intel VT-x (VMX) Implementation
- **Capability Detection**: Evaluates `CPUID.(EAX=1):ECX.VMX[bit 5]`.
- **Feature Control Configuration**: Inspects `IA32_FEATURE_CONTROL_MSR` (`0x3A`). Verifies lock bit (bit 0) and VMXON outside SMX enable (bit 2). Enables bits if unlocked.
- **VMX Root Activation**: Sets `CR4.VMXE` (bit 13), aligns `CR0` and `CR4` against fixed MSRs (`IA32_VMX_CR0_FIXED0/1`, `IA32_VMX_CR4_FIXED0/1`), allocates a page-aligned 4KB VMXON region populated with the VMCS revision ID from `IA32_VMX_BASIC_MSR`, and executes `__vmx_on()`.
- **VMCS Management**: Allocates 4KB page-aligned VMCS regions per vCPU, configures 16-bit, 64-bit, 32-bit, and natural-width VMCS fields via `vmx_vmwrite()`, loads via `vmx_vmptrld()`, launches via `__vmx_vmlaunch()`, and resumes via `__vmx_vmresume()`.

### 2.2 AMD SVM Implementation
- **Capability Detection**: Evaluates `CPUID.(EAX=0x80000001):ECX.SVM[bit 2]`.
- **Host Save Activation**: Sets `EFER.SVME` (`MSR 0xC0000080`, bit 12), allocates a page-aligned 4KB Host Save Area, and writes its physical address into `VM_HSAVE_PA_MSR` (`0xC0010117`).
- **VMCB Management**: Allocates a 4KB page-aligned `vmcb_t` struct containing the control area (intercept vectors, guest ASID, Vintr controls) and state save area (guest segment registers, CR0/CR3/CR4/EFER, RIP, RSP, RAX). Executes guest loop via `__svm_vmrun()`.

---

## 3. Data Structures & Abstractions

### 3.1 vCPU Descriptor (`vcpu_t`)
Represents an individual virtual processor instance:
```c
typedef struct vcpu {
    uint32_t                vcpu_id;
    struct virtual_machine* vm;
    vcpu_state_t            state;              /* CREATED, INITIALIZED, RUNNING, PAUSED, BLOCKED, SHUTDOWN */
    guest_registers_t       regs;               /* GPR state: RAX, RBX, RCX, RDX, RSI, RDI, RBP, R8-R15, CR2 */
    uint64_t                guest_rip;
    uint64_t                guest_rsp;
    uint64_t                guest_rflags;
    uint64_t                guest_cr0;
    uint64_t                guest_cr3;
    uint64_t                guest_cr4;
    uint64_t                guest_efer;
    guest_segment_t         cs, ss, ds, es, fs, gs, tr, ldtr;
    guest_dtable_t          gdtr, idtr;
    bool                    interrupts_enabled;
    uint64_t                exit_count;
    uint64_t                last_exit_reason;
    void*                   arch_data;          /* Pointer to vmx_vcpu_data_t or svm_vcpu_data_t */
} vcpu_t;
```

### 3.2 Virtual Machine Descriptor (`virtual_machine_t`)
Represents an isolated VM container:
```c
typedef struct virtual_machine {
    uint32_t            vm_id;
    char                name[ATOMS_VM_MAX_NAME_LEN];
    vm_state_t          state;                  /* CREATED, INITIALIZED, RUNNING, STOPPED, ERROR */
    virt_type_t         virt_type;              /* VIRT_TYPE_INTEL_VMX or VIRT_TYPE_AMD_SVM */
    uint32_t            vcpu_count;
    vcpu_t              vcpus[ATOMS_VM_MAX_VCPUS];
    uint64_t            guest_memory_size;
    uint64_t            guest_physical_base;
    void*               ept_root;               /* Reserved for Phase 2 */
    uint64_t            total_exits;
    bool                is_active;
} virtual_machine_t;
```

---

## 4. Virtual Machine Lifecycle

The hypervisor manages VM lifecycle through strict state-machine transitions:

```text
    +---------------+
    |  UNALLOCATED  |
    +---------------+
            │
            ▼ atoms_vm_create()
    +---------------+
    |    CREATED    |
    +---------------+
            │
            ▼ atoms_vm_initialize()
    +---------------+
    |  INITIALIZED  | ◄────────┐
    +---------------+          │
            │                  │
            ▼ atoms_vm_run()   │ atoms_vm_stop()
    +---------------+          │
    |    RUNNING    | ─────────┘
    +---------------+
            │
            ▼ atoms_vm_destroy()
    +---------------+
    |   DESTROYED   |
    +---------------+
```

1. **`atoms_vm_create(name, vcpu_count, mem_size)`**: Allocates VM slot, validates vCPU limits and hardware availability, initializes vCPU structures.
2. **`atoms_vm_initialize(vm)`**: Allocates and binds VMCS/VMCB structures, initializes default 64-bit Flat Mode host/guest segment states, sets initial reset vector (`RIP = 0xFFF0`, `RSP = 0x7C00`, `RFLAGS = 0x02`).
3. **`atoms_vm_run(vm, vcpu_id)`**: Executes vCPU execution loop, enters hardware guest mode (`VMLAUNCH`/`VMRESUME` or `VMRUN`), captures VM-exits, and dispatches to handler.
4. **`atoms_vm_stop(vm)`**: Halts active vCPU execution loops, saves final CPU context, transitions state to `VM_STATE_STOPPED`.
5. **`atoms_vm_destroy(vm)`**: Releases allocated VMCS/VMCB and hypervisor structures, frees vCPU instances, clears VM descriptor table.

---

## 5. VM-Exit Dispatcher & Emulation Core

The central dispatcher handles all exits from guest mode and decides whether to resume, emulate, or halt the vCPU:

```c
vmexit_action_t atoms_vmexit_dispatch(vcpu_t* vcpu, const vmexit_context_t* context);
```

| Exit Reason / Code | Description | Handler Action |
|---|---|---|
| **`VMEXIT_REASON_HLT`** | Guest executed `HLT` instruction | Advances `RIP` past instruction, yields or transitions vCPU state |
| **`VMEXIT_REASON_CPUID`** | Guest executed `CPUID` instruction | Evaluates `RAX`/`RCX`, emulates virtual CPU features, advances `RIP` |
| **`VMEXIT_REASON_VMCALL`** | Hypercall / guest hypervisor service | Dispatches guest hypercall request, sets return status in `RAX`, advances `RIP` |
| **`VMEXIT_REASON_IO_INSTRUCTION`** | Guest port I/O access | Emulates I/O port read/write, advances `RIP` |
| **`VMEXIT_REASON_CR_ACCESS`** | Control register move (`CR0/CR3/CR4/CR8`) | Updates virtual CR shadow, advances `RIP` |
| **`VMEXIT_REASON_TRIPLE_FAULT`** | Guest triple fault exception | Logs diagnostic trace, halts vCPU, transitions state to `VCPU_STATE_SHUTDOWN` |
| **Unknown / Unhandled** | Unexpected hardware exit code | Records diagnostic telemetry, aborts vCPU loop |

---

## 6. Phase 1 Synthetic Test Verification

A self-contained synthetic test suite verifies the micro-hypervisor core during kernel initialization (`atoms_hypervisor_run_synthetic_test()`):

1. **Test 1 — CPU Virtualization Capability**: Verifies hardware support detection or reports host virtualization presence.
2. **Test 2 — Hypervisor Subsystem Initialization**: Validates VMX/SVM host setup, MSR configuration, and root operation enablement.
3. **Test 3 — VM Creation & Allocation**: Creates a test virtual machine (`"synthetic_guest"`, 1 vCPU, 16MB RAM).
4. **Test 4 — VM & vCPU Initialization**: Binds architecture control structures, initializes host/guest state descriptors.
5. **Test 5 — VM-Exit Dispatcher Emulation**: Simulates synthetic VM-exits (`HLT`, `CPUID`, `VMCALL`) and validates instruction pointer stepping (`RIP += inst_len`) and return actions (`VMEXIT_ACTION_RESUME` / `VMEXIT_ACTION_HANDLED`).
6. **Test 6 — VM Lifecycle Clean Teardown**: Gracefully destroys the test VM and reclaims resources.

---

## 7. Phase Roadmap & Subsystem Boundaries

```text
[Phase 1] ➔ VMX/SVM Micro-Hypervisor Core & VMCS/VMCB Abstraction (COMPLETED)
    │
[Phase 2] ➔ Extended / Nested Page Tables (EPT / NPT) Paging Subsystem
    │
[Phase 3] ➔ VirtIO Block, Network, Console, & Input Device Virtualization
    │
[Phase 4] ➔ Lightweight FreeBSD Guest Kernel Bootloader & ELF Loader
    │
[Phase 5] ➔ FreeBSD Userspace Minimal Rootfs & IPC Bridge
    │
[Phase 6] ➔ Headless Chromium / Brave Engine Integration & BCM Compositor Surface Sharing
```

*Phase 1 implements the CPU virtualization foundation only. Memory virtualization (EPT/NPT) and I/O virtualization (VirtIO) will be implemented in Phases 2 and 3 respectively.*
