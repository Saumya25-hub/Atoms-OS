# ATOMS OS — PATCH REPORT
## Case: Frozen Spinner "[/] SILICON ACTIVE (LIVE)" and Unresponsive USB Keyboard (ASUS B760M-K)
### Governing Rule: RULE 0 — Mandatory Phase Isolation (TASK 3: PATCH TEAM)

**Input:** `FORENSIC_REPORT.md`, `PATCH_PLAN.md`  
**Status:** TASK 3 COMPLETE — Targeted Implementation Complete  
**Target Hardware:** ASUS PRIME B760M-K (Intel Core i3-14100F, Raptor Lake, 8 Cores)  

---

## 1. Summary of Changes

In strict accordance with `PATCH_PLAN.md`, only the 4 approved files were modified:

### 1. `drivers/timer/pit/pit.c`
- **Function Changed:** `pit_set_frequency(uint32_t frequency)`
- **Lines Changed:** Added static inline `io_wait()` (port 0x80 delay) and invoked `io_wait()` after command write (port 0x43), LSB write (port 0x40), and MSB write (port 0x40).
- **Result:** Resolves modern PCH eSPI/LPC bus latching race condition at 4.7 GHz, enabling accurate 1000 Hz PIT timer ticks.

### 2. `kernel/core/timer/src/timer.c`
- **Function Changed:** `timer_tick_handler(registers_t *regs)`
- **Lines Changed:** Added `extern void xhci_poll(void); xhci_poll();` inside the hardware timer tick handler.
- **Result:** Drains USB xHCI event rings and processes HID reports at interrupt time on every tick, guaranteeing instant NumLock LED updates and physical key event queuing regardless of thread state or hypervisor execution.

### 3. `kernel/debug/hypervisor_dashboard/hypervisor_dashboard.c`
- **Function Changed:** `hypervisor_dashboard_step(void)`
- **Lines Changed:**
  - Bounded guest vCPU execution: `atoms_hypervisor_runtime_step(rt_vm, 5)` (reduced from 50 to 5 exits).
  - Decoupled spinner animation: `char spin_char = s_spin[(s_dash_ticks / 4) % 4];` (rotates per 4 UI render frames).
- **Result:** Eliminates dashboard render stalls and ensures the top spinner rotates visibly and smoothly whenever the UI render loop is alive.

### 4. `kernel/core/hypervisor/src/hypervisor.c`
- **Functions Changed:** `atoms_vmexit_dispatch(vCPU *vcpu)` (VMX and SVM CPUID exit handlers)
- **Lines Changed:** Bounded `s_cpuid_log_count` and `s_svm_cpuid_log_count` to 5 exits max.
- **Result:** Eliminates COM1 UART baud rate serial port blocking during guest VM execution.
