# ATOMS OS — ARCHITECTURE PATCH PLAN
## Case: Frozen Spinner "[/] SILICON ACTIVE (LIVE)" and Unresponsive USB Keyboard (ASUS B760M-K)
### Governing Rule: RULE 0 — Mandatory Phase Isolation (TASK 2: ARCHITECT TEAM)

**Input:** `FORENSIC_REPORT.md`  
**Status:** TASK 2 COMPLETE — Architecture Patch Plan Formulated (NO CODE IMPLEMENTATION)  
**Target Hardware:** ASUS PRIME B760M-K (Intel Core i3-14100F, Raptor Lake, 8 Cores)  

---

## 1. Scope of Modification

The patch strictly encompasses 4 files:
1. `drivers/timer/pit/pit.c`
2. `kernel/core/timer/src/timer.c`
3. `kernel/debug/hypervisor_dashboard/hypervisor_dashboard.c`
4. `kernel/core/hypervisor/src/hypervisor.c`

**HARD RULE Compliance:**
- ❌ Refactor random files: Strictly NO.
- ❌ Touch unrelated subsystems: Strictly NO.
- ❌ Rewrite architecture: Strictly NO.
- ❌ Touch `kernel.c`: Not required and NOT modified.
- ❌ Rename APIs: Strictly NO.

---

## 2. Detailed Modification Plan

### Modification 1: PCH Bus Delay for PIT Divisor Programming
- **File:** `drivers/timer/pit/pit.c`
- **Function:** `pit_set_frequency(uint32_t frequency)`
- **Modification:**
  - Define static inline `io_wait()` sending dummy byte to POST port 0x80 (`io_out8(0x80, 0)`), matching `drivers/interrupt/pic/pic.c`.
  - Insert `io_wait()` after the command byte write to `PIT_COMMAND_PORT`.
  - Insert `io_wait()` after the LSB write to `PIT_CHANNEL0_DATA`.
  - Insert `io_wait()` after the MSB write to `PIT_CHANNEL0_DATA`.
- **Reason:** Raptor Lake CPUs operating at 4.7 GHz execute back-to-back outb instructions faster than the LPC/eSPI bridge can latch. Without settling delay, the divisor writes are dropped, leaving the PIT running at its 18.2 Hz default.
- **Expected Result:** PIT IRQ 0 interrupt fires at calibrated 1000 Hz, making 1 tick = 1 ms.
- **Risk:** ZERO. Standard PC architecture practice used by Linux, FreeBSD, and existing ATOMS PIC driver.
- **Rollback:** Remove `io_wait()` calls.

### Modification 2: Interrupt-Driven xHCI Event Ring Polling
- **File:** `kernel/core/timer/src/timer.c`
- **Function:** `timer_tick_handler(registers_t *regs)`
- **Modification:**
  - Declare `extern void xhci_poll(void);`.
  - Call `xhci_poll();` during each timer tick before or alongside `BRE_DispatchPending()`.
- **Reason:** USB keyboard transfers and NumLock toggle events must be drained even if user threads are sleeping or the hypervisor is stepping. `xhci_poll()` is already reentrancy-safe via `__sync_lock_test_and_set(&s_xhci_poll_active, 1)`.
- **Expected Result:** USB HID packets are drained and dispatched immediately upon arrival. NumLock LED state is toggled in real time; `F1`–`F6` keypresses are registered instantly.
- **Risk:** VERY LOW. `xhci_poll()` already handles locking and returns immediately if active on another CPU or reentered.
- **Rollback:** Remove `xhci_poll()` call from `timer_tick_handler()`.

### Modification 3: Bounding Guest vCPU Exit Budget and Decoupling Spinner
- **File:** `kernel/debug/hypervisor_dashboard/hypervisor_dashboard.c`
- **Function:** `hypervisor_dashboard_step(void)`
- **Modification:**
  - Reduce exit budget argument in `atoms_hypervisor_runtime_step(rt_vm, 50)` from 50 to 5.
  - Change spinner calculation from `s_spin[(now_ticks / 125) % 4]` to frame-driven `s_spin[(s_dash_ticks / 4) % 4]`.
- **Reason:** 50 exits per frame caused guest execution to monopolize CPU 0 for hundreds of milliseconds. Bounding to 5 exits preserves a consistent 30+ FPS UI refresh. Decoupling the spinner from tick arithmetic guarantees smooth visual rotation whenever the UI render loop is alive.
- **Expected Result:** Smooth, continuous spinner rotation `[| / - \]`; no frame stuttering.
- **Risk:** LOW. Guest execution progresses gracefully in 5-exit increments every ~30ms frame.
- **Rollback:** Restore budget to 50 and tick-based formula.

### Modification 4: Silencing COM1 UART Flooding on CPUID Exits
- **File:** `kernel/core/hypervisor/src/hypervisor.c`
- **Function:** `atoms_vmexit_handle_cpuid(vCPU *vcpu)`
- **Modification:**
  - In `atoms_vmexit_handle_cpuid`, cap verbose serial logging from 60 exits to 5 exits (`s_cpuid_log_count < 5`).
- **Reason:** At 115200 baud, printing 15 serial lines per exit blocks CPU 0 for 22ms per exit. Bounding this eliminates serial starvation.
- **Expected Result:** Serial port remains available without blocking guest execution or scheduler telemetry.
- **Risk:** ZERO. Only affects diagnostic UART logging.
- **Rollback:** Restore threshold to 60.

---

## 3. Verification & Acceptance Criteria (For Task 4)

1. **Compilation:** Clean compilation with zero warnings/errors.
2. **QEMU Pre-Flight:** Pure UEFI boot in QEMU, verifying clean UI rendering, active spinner rotation, and zero regressions.
3. **Physical Hardware Boot:** PXE serving to ASUS PRIME B760M-K.
4. **Interactive Verification:**
   - Top spinner `[| / - \]` continuously rotates.
   - Physical NumLock key toggles physical keyboard LED on and off.
   - `F1`–`F6` keys switch dashboard debug pages seamlessly.
   - `build/atoms_live_kernel.log` reflects `g_ui_perf_hid_reports > 0` on keypress.
