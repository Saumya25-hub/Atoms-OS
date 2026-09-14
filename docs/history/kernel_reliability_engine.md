# ATOMS Kernel Reliability, Fault Containment & Context Safety Engine (KRFC)

## Architectural Specification V1.0

---

## 1. System Overview & Core Principles

The **ATOMS Kernel Reliability, Fault Containment & Context Safety Engine (KRFC)** establishes a production-grade reliability baseline modeled after x86-64 architectural guarantees and modern OS kernel (Linux/Windows NT) reliability invariants:

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                             USER SPACE (Ring 3)                             │
│       Desktop Shell (PID 200)      Apps & Tools       Test Runners         │
└───────────────────────┬─────────────────────────────────────┬───────────────┘
                        │ SYSCALL (IA32_LSTAR)                │ FAULT (#PF/#GP)
                        ▼                                     ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                        KERNEL BOUNDARY BARRIER (Ring 0)                     │
│  ┌───────────────────────────────┐   ┌───────────────────────────────────┐  │
│  │   Syscall Validation Engine   │   │   User Fault Containment Unit     │  │
│  │   - Ptr bounds [0x40M..0x80M) │   │   - Ring 3 #PF/#GP termination    │  │
│  │   - Frame sanitization        │   │   - Kernel surviving execution    │  │
│  └───────────────┬───────────────┘   └───────────────────┬───────────────┘  │
│                  ▼                                       ▼                  │
│  ┌───────────────────────────────────────────────────────────────────────┐  │
│  │               Central IRQ State Engine (irq_save / restore)           │  │
│  │               - Preserves IF flags across all kernel critical regions │  │
│  │               - Prohibits raw unmasked STI in ISRs & drivers          │  │
│  └───────────────────────────────────────┬───────────────────────────────┘  │
│                                          ▼                                  │
│  ┌───────────────────────────────────────────────────────────────────────┐  │
│  │               Task & Stack Guard Watchdog Monitor                     │  │
│  │               - 32KB guarded kernel stack + canary verification       │  │
│  │               - Separation: SyscallFrame ≠ IRQFrame ≠ Context         │  │
│  │               - Input coalescing & rate control                       │  │
│  └───────────────────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 2. Subsystem Architectures

### Subsystem 1: Central Interrupt-State Engine (`irq_flags_t`, `irq_save`, `irq_restore`)
- **Semantics**:
  - `irq_save()` pushes RFLAGS to stack, pops into a 64-bit integer, and executes `cli`.
  - `irq_restore(flags)` pushes the saved flags and executes `popfq`, returning RFLAGS (and specifically `IF`) to its exact prior state.
  - **Hard Invariant**: Zero raw `sti` instructions inside interrupt service routines, driver event pumps, or queue handlers.

### Subsystem 2: Frame Separation & Privilege Transition
- **Frame Isolation Matrix**:
  1. `ATOMS_SyscallFrame` (112 bytes): Allocated explicitly on `syscall_entry` at `TSS.rsp0 - 48 - 112`.
  2. `registers_t` (176 bytes): Pushed dynamically on interrupt/exception entry.
  3. `Context` (176 bytes): ABI-coupled with `registers_t` for scheduler context switches.
- **Atomic User Return**:
  - Validates `user_rip` in `[0x40000000, 0x80000000)` and `user_rsp` in `[0x40000000, 0x80000000]`.
  - Enforces `CS = 0x23` and `SS = 0x1B`.
  - Sanitizes `RFLAGS` ($0\text{x}202 \mid (\text{rflags} \ \& \ 0\text{xCD5})$).

### Subsystem 3: User-Mode Fault Containment Unit
- When an unhandled `#PF`, `#GP`, `#UD`, or `#DE` occurs in Ring 3 (`CS & 3 == 3`):
  1. The kernel does **NOT** panic or halt.
  2. The faulting thread/process is safely terminated via `ATOMS_Process_Terminate(pid, -status)`.
  3. A diagnostic forensic record is captured to serial COM1.
  4. The scheduler picks the next runnable task (or `idle_task`), keeping the desktop, compositor, and kernel fully alive.

### Subsystem 4: Syscall Pointer & Memory Bounds Validator
- Validates all user buffer pointers passed via syscall registers (`rdi`, `rsi`, `rdx`, `r10`, `r8`, `r9`).
- Rejects any pointer residing in supervisor space ($< 0\text{x}40000000$ or $\ge 0\text{x}80000000$) with `SYSCALL_BAD_ADDRESS` ($-4$).

### Subsystem 5: Input Coalescing & Presentation Rate Controller
- In high-frequency interrupt environments (e.g. VMware PS/2 mouse generating 300–600 IRQ 12 interrupts/second):
  - Mouse motion events are coalesced in the process queue.
  - Invalidation is decoupled from ISR dispatch, preventing uncontrolled composition loops while maintaining instant responsiveness.

### Subsystem 6: Stack Integrity & Task Watchdog
- Every task stack is allocated with 32KB space and a 64-bit guard canary at the stack base.
- Scheduler verifies stack bounds on every context switch. If stack corruption is detected, the watchdog isolates the faulted task.
