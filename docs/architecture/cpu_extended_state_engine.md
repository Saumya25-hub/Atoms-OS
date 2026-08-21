# ATOMS CPU EXTENDED STATE ENGINE ARCHITECTURE

## 1. Overview
The **ATOMS CPU Extended State Engine** provides robust, architectural management of x86_64 processor extended states (x87 FPU, MMX, SSE, SSE2-4, AVX, and extended register contexts) across user and kernel tasks.

Inspired by the engineering principles of Linux and Windows NT, but built with ATOMS simplicity and lightweight performance, the engine dynamically selects the safest and most capable hardware backend (`XSAVE`/`XRSTOR` or `FXSAVE`/`FXRSTOR`), guarantees strict 64-byte alignment, clones known-good clean initial templates, and centralizes context switching.

---

## 2. Architecture & Backend Selection

```text
                       [ CPU Boot / Features Discovery ]
                                       │
                         CPUID Leaf 1, Leaf 7, Leaf 0xD
                                       │
                 ┌─────────────────────┴─────────────────────┐
                 │                                           │
         [ XSAVE Supported ]                        [ FXSR Supported ]
                 │                                           │
         Enable CR4.OSXSAVE                          Enable CR4.OSFXSR
         Set XCR0 (x87, SSE, AVX)                    Enable CR4.OSXMMEXCPT
         Backend: CPU_EXT_BACKEND_XSAVE              Backend: CPU_EXT_BACKEND_FXSAVE
         Alignment: 64 Bytes                         Alignment: 64 Bytes
         Size: Dynamic from CPUID (>= 576 B)         Size: 512 Bytes
                 │                                           │
                 └─────────────────────┬─────────────────────┘
                                       │
                    [ Clean Template Initialization ]
                    - fninit
                    - ldmxcsr (0x1F80 - All Exceptions Masked)
                    - Baseline xsave / fxsave into Clean Template
```

---

## 3. Data Structures & Task Lifecycle

### Task Extended Context
```c
typedef struct TaskExtendedContext {
    uint32_t magic;          /* 0x41544558 ("ATEX") */
    CpuExtBackend backend;   /* CPU_EXT_BACKEND_XSAVE or FXSAVE */
    uint32_t size;           /* Buffer size allocated in bytes */
    uint32_t alignment;      /* Guaranteed alignment (64 bytes) */
    bool is_initialized;     /* True if state buffer holds valid architectural state */
    uint8_t *buffer;         /* 64-byte aligned memory storage */
} TaskExtendedContext;
```

### Invariants
1. **Magic Guarantee**: `task->extended_state` must always start with `CPU_EXT_MAGIC` (`0x41544558`).
2. **Strict Alignment**: `buffer` is strictly 64-byte aligned (`((uintptr_t)buffer & 0x3F) == 0`).
3. **Known-Good State**: Newly spawned tasks copy `clean_template`, preventing uninitialized or invalid MXCSR reserved bits from reaching the CPU.
4. **Ownership**: `g_cpu_ext_engine.current_owner` records the active task whose FPU/SSE state is currently resident in hardware registers.

---

## 4. Centralized Context Switch Flow

```text
                            OLD TASK
                               │
                cpu_extended_state_save(old_task)
                (Validates magic, alignment, executes XSAVE/FXSAVE)
                               │
                tss_set_kernel_stack(new_task->stack + size)
                               │
                vmm_switch_address_space(new_task->pml4)
                               │
                cpu_extended_state_restore(new_task)
                (Validates magic, alignment, executes XRSTOR/FXRSTOR)
                               │
                            NEW TASK
```

---

## 5. Files Changed & APIs

### Files Modified
- [`arch/x86_64/cpu/cpu_features.h`](file:///d:/Signatures_OS/arch/x86_64/cpu/cpu_features.h)
- [`arch/x86_64/cpu/cpu_features.c`](file:///d:/Signatures_OS/arch/x86_64/cpu/cpu_features.c)
- [`kernel/core/cpu/cpu_state.h`](file:///d:/Signatures_OS/kernel/core/cpu/cpu_state.h)
- [`kernel/core/cpu/cpu_state.c`](file:///d:/Signatures_OS/kernel/core/cpu/cpu_state.c)
- [`kernel/core/scheduler/src/scheduler.c`](file:///d:/Signatures_OS/kernel/core/scheduler/src/scheduler.c)
- [`kernel/core/process/src/process.c`](file:///d:/Signatures_OS/kernel/core/process/src/process.c)

### Primary APIs
- `void cpu_extended_state_engine_init(void)`: Detects features, arms control registers, and allocates clean template.
- `bool cpu_extended_state_init_task(Task *task)`: Allocates `TaskExtendedContext` and 64-byte aligned buffer, initializing it from template.
- `void cpu_extended_state_free_task(Task *task)`: Frees context and memory buffer.
- `void cpu_extended_state_save(Task *task)`: Saves extended state using active backend.
- `void cpu_extended_state_restore(Task *task)`: Restores extended state using active backend.
- `void cpu_extended_state_context_switch(Task *old_task, Task *new_task)`: Unified switch helper.

---

## 6. Certification & Hardware Test Matrix

| Platform / Test Profile | Configuration | Verdict |
| :--- | :--- | :--- |
| **Intel H81 Motherboard** | Haswell Core i3 LGA1150, 8 GB RAM, Native UEFI | `PASS ✅` |
| **ASUS B750M-K** | Hardware Target 2 | `PASS ✅` |
| **QEMU x86_64** | Pure UEFI (`OVMF.fd`) | `PASS ✅` |
| **VMware Workstation** | VMMouse Backdoor & Virtual UEFI | `PASS ✅` |
| **Extended Desktop Session** | Continuous Mouse Movement & Idle | `PASS ✅` |
| **Multitasking User Processes** | Multiple Ring 3 apps, repeated yield/switch | `PASS ✅` |
