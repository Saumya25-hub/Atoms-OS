# ATOMS OS — MASTER ARCHITECTURE PATCH PLAN (v2)
## Stage B: Forensic Stabilization & Phase 5/6/7 Hardening

**Date:** 2026-10-23  
**Status:** TASK 2 COMPLETE (Architectural Plan Formulated — NO CODE MODIFIED)  
**Input Document:** [FORENSIC_REPORT.md](file:///d:/Signatures_OS/FORENSIC_REPORT.md)  
**Governing Rule:** RULE 0 — Mandatory Phase Isolation (Investigate ➔ Plan ➔ Patch ➔ Certify)  

---

### 1. Architectural Scope & Whitelisted Files

Under strict RULE 0, modifications are permitted ONLY to the following files:
1. `kernel/debug/abde/abde.h` (Add `silent_mode` flag, `diag_mute()`, `diag_unmute()`)
2. `kernel/debug/abde/abde.c` (Decouple `diag_render()` from all synchronous `diag_set_*` setters)
3. `kernel/debug/abde/abde_renderer.c` (Bounds-checked buffer formatting, stack safety, silent mode exit guard)
4. `kernel/core/interrupt/src/exception.c` (Decouple exception dispatch from synchronous framebuffer render)
5. `kernel/kernel.c` (Mute legacy ABDE dashboard when entering multi-core scheduler and hypervisor runtime)

No other files or kernel subsystems may be modified.

---

### 2. Component Design Specifications

#### A. Decouple Diagnostic State from Framebuffer Rendering (`abde.c`)
* **Problem**: Currently, every call to `diag_set_step()`, `diag_set_pass()`, `diag_set_fail()`, `diag_set_running()`, `diag_set_fault()`, etc. synchronously invokes `diag_render()`. This forces every caller (including boot steps, interrupt handlers, and scheduler paths) to execute 720+ bytes of graphics rendering on whatever stack it happens to be using.
* **Architecture Fix**:
  - Remove all synchronous `diag_render()` invocations from `diag_set_*` functions.
  - Setters strictly update the memory fields of `g_abde` in $O(1)$ time with memory barriers:
    ```c
    void diag_set_step(const char *step_name) {
        abde_strcpy(g_abde.last_event, g_abde.current_step, ABDE_MAX_STEP_LEN);
        abde_strcpy(g_abde.current_step, step_name, ABDE_MAX_STEP_LEN);
    }
    ```
  - Framebuffer rendering is strictly triggered by:
    1. Dedicated boot progression checkpoints prior to scheduler initialization.
    2. Read-only presentation in `system_telemetry_thread()`.
* **Expected Result**: Caller stack depth is reduced by 720+ bytes. Zero framebuffer access during interrupts or lock-sensitive operations.

#### B. Buffer Hardening & Stack Safety in ABDE Renderer (`abde_renderer.c`)
* **Problem**: In `diag_render()`, local stack buffers (`surv_buf[48]`, `num_tmp[24]`, `hb_buf[64]`) are populated using unbounded pointer traversals (`while (*p) p++; while (*q) *p++ = *q++;`), creating a severe risk of stack corruption if integers or strings exceed buffer limits.
* **Architecture Fix**:
  - Replace raw pointer pointer math with explicit bounded formatting helpers.
  - Check buffer capacity before every byte write:
    ```c
    // Bounded survival heartbeat formatting
    char surv_buf[48];
    abde_strcpy(surv_buf, "SURVIVAL HEARTBEAT: ", sizeof(surv_buf));
    abde_format_dec_append(surv_buf, sizeof(surv_buf), g_survival_heartbeat_count);
    ```
  - Add entry guard: `if (g_abde.silent_mode || !g_abde.framebuffer) return;`.
* **Expected Result**: Provable absence of stack frame overwrite. Retq always pops a valid canonical caller return address.

#### C. Exception Dispatch Stabilization (`exception.c`)
* **Problem**: When a kernel fault occurs, `exception_dispatch()` calls `diag_set_fail("IDT")` and `diag_set_fault(name, formatted)`, which previously triggered synchronous `diag_render()` calls directly on the interrupt stack.
* **Architecture Fix**:
  - Keep telemetry recording in memory (`g_abde.error_code`, `g_abde.fault_detail`).
  - Do not render graphics inside `exception_dispatch()`.
  - Let `atoms_screenshot_capture_sync()` capture the exact pre-panic screen state and stream it to the LAN receiver without modifying the display.
* **Expected Result**: Interrupt and fault handlers operate within strict, bounded stack budgets without triggering secondary `#GP` faults.

#### D. ABDE Mute Gate for Phase 5 & Compositor Runtime (`kernel.c` & `abde.h`)
* **Problem**: Once the system brings up the hypervisor dashboard (Page 4 / Page 5 Danger Monitor) or the BCM desktop compositor, legacy ABDE V2.5 blue diagnostic boxes must never be painted on top of the active display.
* **Architecture Fix**:
  - Add `void diag_set_silent_mode(bool silent);` in `abde.h` and `abde.c`.
  - In `kernel_main()`: Once Stage 1 boot completes and the system enters Level 5 and the hypervisor dashboard, invoke `diag_set_silent_mode(true)`.
* **Expected Result**: Clean visual separation between boot diagnostics and runtime dashboards. Zero UI clobbering.

---

### 3. Risk Analysis & Safety Boundaries

| Planned Modification | Potential Failure Mode | Mitigation Strategy |
| :--- | :--- | :--- |
| **Remove `diag_render()` from setters** | Visual boot table may not update until explicit render call. | Call explicit `diag_render()` at major boot milestone boundaries during Stage 0/1 boot, while the system is single-threaded on BSP boot stack. |
| **Buffer hardening in renderer** | String truncation if buffer is too small. | Allocate safe, static bounded sizes (e.g. 64 bytes for a 20-character label + 20-digit integer) with guaranteed null termination. |
| **Mute ABDE in runtime** | Loss of diagnostic text if an early panic occurs post-boot. | All diagnostics continue to be mirrored in full to COM1 serial (`com1_puts`) and UDP LAN debug packets (`debuglan_log_subsys`). |

---

### 4. Rollback Plan

If any regression occurs during compilation or QEMU pre-flight validation:
1. Revert `kernel/debug/abde/abde.c`, `abde.h`, `abde_renderer.c`.
2. Revert `kernel/core/interrupt/src/exception.c` and `kernel/kernel.c`.
3. Restore clean working baseline using `git checkout`.

---

**ARCHITECT TEAM SIGNOFF:** `PATCH_PLAN.md` is approved. Ready for handoff to **TASK 3 — PATCH TEAM**.
