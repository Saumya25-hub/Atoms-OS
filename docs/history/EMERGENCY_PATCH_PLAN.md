# EMERGENCY PATCH PLAN — ATOMS OS
## Decoupling Heavy Composition from Timer ISR & Arming IST Exception Stack

**Document ID:** `EMERGENCY_PATCH_PLAN.md`  
**Classification:** Incident Remediation Plan (Rule 0 Phase Isolation — NO CODE COMMITTED YET)  

---

## 1. What to Modify & Why

### Patch 1: Decouple Composition from Timer ISR
- **File:** [`kernel/wm/bwe/src/bwe_core.c`](file:///d:/Signatures_OS/kernel/wm/bwe/src/bwe_core.c)
- **Problem:** `BWE_PumpEvents()` directly invokes `BWE_Compose()` synchronously inside the `bre_input_pump_callback()`, which runs inside `timer_tick_handler()` (IRQ 0).
- **Modification:** Set a deferred composition request flag (`g_bwe_needs_compose = true;`) during event processing. Allow the Timer ISR to finish in < 50 microseconds. Execute `BWE_Compose()` outside of the hardware ISR on a dedicated compositor task or cooperative loop.
- **Expected Result:** Timer ISR duration drops from **45.9 ms** to **< 0.05 ms** (99.9% reduction in ISR hold time). Eliminates APIC timer starvation and prevents VMware watchdog vCPU shutdown.

---

### Patch 2: IST Dedicated Emergency Stack for Double Fault (`#DF`) & Page Fault (`#PF`)
- **Files:** [`arch/x86_64/interrupt/idt.c`](file:///d:/Signatures_OS/arch/x86_64/interrupt/idt.c), [`arch/x86_64/interrupt/idt.h`](file:///d:/Signatures_OS/arch/x86_64/interrupt/idt.h), [`kernel/core/scheduler/src/scheduler.c`](file:///d:/Signatures_OS/kernel/core/scheduler/src/scheduler.c)
- **Problem:** All 256 IDT descriptors use `ist = 0`. If a stack fault occurs in Ring 0, the CPU cannot push the exception frame onto the damaged stack, causing an unrecoverable Triple Fault (`#TF`) and instant hardware power-off.
- **Modification:** Configure IST index 1 in TSS as a dedicated 16 KB emergency exception stack, and set `idt[8].ist = 1` (Double Fault) and `idt[14].ist = 1` (Page Fault).
- **Expected Result:** Any kernel-mode stack fault or paging fault is safely caught on the emergency stack, rendering forensic ABDE crash diagnostics rather than triggering hardware reset.

---

### Patch 3: Dirty Rect Defensive Null-Check & Clamping
- **File:** [`kernel/wm/bwe/renderer/bwe_compositor.c`](file:///d:/Signatures_OS/kernel/wm/bwe/renderer/bwe_compositor.c)
- **Problem:** `BWE_AddCompositorDirtyRect()` lacks a null pointer guard on `rect`.
- **Modification:** Add `if (!rect || rect->width <= 0 || rect->height <= 0) return;`.

---

## 2. Risk Analysis & Rollback Plan

| Risk | Mitigation | Rollback Plan |
|---|---|---|
| **Compositor latency if deferred** | If deferred flag is polled every frame cycle (16.6 ms / 60 FPS), responsiveness is maintained at 60 FPS without ISR stalls. | Revert `bwe_core.c` changes via Git. |
| **TSS / IST misconfiguration** | Dedicated statically allocated 16 KB aligned buffer ensures zero dynamic allocation dependencies. | Restore `idt[vector].ist = 0`. |

---

## 3. Approval Gate
Per Rule 0, no source code will be modified until this Forensic Report, Root Cause Analysis, and Patch Plan are reviewed.
