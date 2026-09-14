# CHROMIUM / ATOMS OS EXCEPTION & FAULT MODEL SPECIFICATION

**Document ID:** ATOMS-CHROMIUM-EXC-001  
**Phase:** 16-B (Exception & Fault Isolation Architecture)  
**Standard:** Phase Isolation, Zero-Kernel-Panic for Ring 3, Forensic Telemetry  
**Date:** 2026-09-07  
**Status:** ARCHITECTURE VERIFIED  

---

## 1. Architectural Philosophy

Chromium's multi-process model is designed fundamentally as an exception and crash containment boundary. Web content execution (untrusted HTML, CSS, JavaScript, WebAssembly) runs entirely inside sandboxed **Renderer Processes**. If a renderer process suffers a segmentation fault, out-of-bounds array access, null pointer dereference, or illegal instruction, the failure must be strictly contained within that single process.

```text
                           ┌────────────────────────────────┐
                           │         Browser Process        │
                           │     (PID 300 - UI / Master)    │
                           └───────────────┬────────────────┘
                                           │ Mojo IPC
                  ┌────────────────────────┴────────────────────────┐
                  ▼                                                 ▼
     ┌────────────────────────┐                        ┌────────────────────────┐
     │   Renderer Process A   │                        │   Renderer Process B   │
     │       (PID 301)        │                        │       (PID 302)        │
     ├────────────────────────┤                        ├────────────────────────┤
     │  Page Fault (#PF: 14)  │                        │  Normal Web Execution  │
     │      [CRASHED]         │                        │      [REMAINS ALIVE]   │
     └───────────┬────────────┘                        └───────────┬────────────┘
                 ▼                                                 ▼
     ┌────────────────────────┐                        ┌────────────────────────┐
     │ Kernel Fault Handler   │                        │ Continued Execution    │
     │ Ring 3 Trap & Contain  │                        │ Unaffected by PID 301  │
     └───────────┬────────────┘                        └────────────────────────┘
                 ▼
     ┌────────────────────────┐
     │ Reclaim PID 301 Memory │
     │ Notify Browser via IPC │
     │ Kernel 100% Stable     │
     └────────────────────────┘
```

---

## 2. ATOMS Native Exception Handling Architecture

ATOMS OS handles processor exceptions through its 64-bit Interrupt Descriptor Table (IDT) configured with Interrupt Service Routines (ISRs) for vectors 0 through 31 in `kernel/core/interrupt/src/exception.c`.

### 2.1 Fault Dispatching Logic (`exception_dispatch`)

When the CPU encounters an exception, it pushes the interrupt stack frame (`SS`, `RSP`, `RFLAGS`, `CS`, `RIP`, `Error Code`, `Vector Number`) onto the kernel TSS RSP0 stack:

1. **Ring Determination:**  
   The dispatcher inspects `regs->cs & 0x03`:
   - `CPL == 0`: Kernel-mode fault. Indicates a fatal internal bug; enters kernel panic loop with ABDE diagnostic assertion.
   - `CPL == 3`: Usermode fault. Indicates a misbehaving or crashed Ring 3 application (e.g. Renderer Process).

2. **Forensic Telemetry Capture:**  
   For all exceptions, the kernel captures:
   - Exception vector name (e.g., `#PF Page Fault`, `#GP General Protection Fault`, `#UD Invalid Opcode`)
   - Faulting Instruction Pointer (`RIP`)
   - Faulting Stack Pointer (`RSP`)
   - Faulting Memory Address (`CR2` for Page Faults)
   - Architectural Error Code (`regs->err_code`, decode of Present, Write, User bits)
   - Full 4-level PML4/PDPE/PDE/PTE hardware pagewalk dumped to COM1 serial.

3. **Ring 3 Fault Containment Protocol:**
   ```c
   if ((regs->cs & 0x03) == 0x03) {
       com1_puts("[USERMODE FAULT CONTAINMENT] Terminating faulting Ring 3 process.\r\n");
       Task *cur = scheduler_current_task();
       if (cur) {
           if (cur->owner_pid) {
               ATOMS_Process_Terminate(cur->owner_pid, -(int32_t)regs->int_no);
           }
           scheduler_terminate_task(cur);
       }
       scheduler_on_tick();
       Task *next = scheduler_current_task();
       uint64_t next_rsp = next ? context_restore_state(next) : 0;
       return next_rsp;
   }
   ```

4. **Resource Reclamation:**  
   - All tasks owned by the faulting PID are terminated and reaped from the scheduler runqueues.
   - Any GUI surfaces created by the PID are closed (`BOS_CloseSurfacesByPID(pid)`).
   - The process PCB transitions to `ATOMS_PROC_STATE_ZOMBIE` if a parent exists, preserving the exit code (`-(int32_t)regs->int_no`) so the parent can query it via `waitpid()`.
   - The scheduler context switches immediately to the next ready task without idling or stalling.

---

## 3. Exception Matrix & Chromium Mapping

| Vector | Exception Type | Usermode Cause | ATOMS Containment Behavior | Chromium Base / POSIX Mapping |
|:---:|:---|:---|:---|:---|
| **0** | `#DE Divide Error` | Division by zero in JavaScript/C++ | Process terminated with code `-0`; scheduler switches to next task. | Mapped to `SIGFPE` $\rightarrow$ `TERMINATION_STATUS_PROCESS_CRASHED` |
| **6** | `#UD Invalid Opcode` | Unsupported SSE/AVX instruction or corrupted code page | Process terminated with code `-6`; code page invalidated. | Mapped to `SIGILL` $\rightarrow$ `TERMINATION_STATUS_PROCESS_CRASHED` |
| **13** | `#GP General Protection` | Non-canonical pointer dereference, segment violation, Ring 0 MSR write | Process terminated with code `-13`; kernel registers scrubbed. | Mapped to `SIGSEGV` $\rightarrow$ `TERMINATION_STATUS_PROCESS_CRASHED` |
| **14** | `#PF Page Fault` | Null pointer dereference, write to read-only page, access to unmapped page | Process terminated with code `-14`; CR2 logged to COM1. | Mapped to `SIGSEGV` $\rightarrow$ `TERMINATION_STATUS_PROCESS_CRASHED` |
| **N/A**| Syscall Violation | User pointer $< 0x40000000$ or $\ge 0x80000000$, or unmapped user address | Syscall rejected immediately with `SYSCALL_BAD_ADDRESS` without terminating process. | Returns `EFAULT` |

---

## 4. Multi-Process Isolation Verification Protocol

To verify that child crashes do not cascade across the browser hierarchy:

1. **Independent Page Tables:**  
   Browser (Parent) has its own PML4; Renderer A has PML4_A; Renderer B has PML4_B. Address modifications in PML4_A have zero effect on PML4_B or Parent PML4.

2. **Mojo Channel Robustness:**  
   When Renderer A faults and terminates, its IPC channel endpoint is detected as closed/hung up. The Browser process receives an endpoint error callback, unregisters Renderer A, and logs a child termination status (`TERMINATION_STATUS_PROCESS_CRASHED`).

3. **Renderer B Invariance:**  
   Renderer B continues servicing DOM and JavaScript events over its dedicated Mojo channel with zero interruption, zero dropped packets, and zero latency degradation.

4. **Kernel Stability:**  
   The BOS Kernel reports 0 leaked pages, 0 corrupted locks, and continues scheduling tasks across all CPU cores.
