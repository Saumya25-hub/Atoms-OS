# EMERGENCY CERTIFICATION PLAN — ATOMS OS

**Document ID:** `EMERGENCY_CERTIFICATION_PLAN.md`  
**Classification:** Post-Remediation Verification & Hardware Certification Protocol  

---

## 1. Automated Verification Suite (QEMU & VMware)

### Test Suite 1: Timer ISR Duration Benchmark
- **Test:** Log entry/exit timestamp for `timer_tick_handler()`.
- **Pass Criteria:** `timer_tick_handler()` execution time must be **< 100 microseconds** under heavy multi-window load.

### Test Suite 2: Multi-App Stress Reproduction (Explorer + Terminal + Calculator)
- **Test:** Run automated QMP script `scratch/reproduce_calc_crash.py`:
  1. Boot OS and log in.
  2. Launch Explorer.
  3. Launch Terminal.
  4. Launch Calculator.
  5. Perform calculation sequence: `2` $\to$ `+` $\to$ `2` $\to$ `=`.
  6. Rapidly click multiple buttons and evaluate complex formulas.
- **Pass Criteria:** Zero hangs, zero dropped frames, zero vCPU shutdown states, correct calculation result displayed ("4").

### Test Suite 3: 5-Application Concurrency Stress Test
- **Test:** Open Explorer, Terminal, Calculator, Settings, and Notes simultaneously. Move windows, overlap dirty rectangles, and type in textboxes.
- **Pass Criteria:** Stable 60 FPS compositor execution without watchdog timeouts or kernel faults.

---

## 2. Physical Hardware Certification (Haswell LGA1150 H81)

Once QEMU and VMware automated verification passes 100%:
1. Deploy new build image via PXE TFTP Boot (`BOOTX64.EFI`).
2. Boot Haswell H81 physical test bench.
3. Open Explorer, Terminal, Calculator.
4. Execute `2 + 2 =` and stress-test buttons via physical USB mouse and keyboard.
5. Verify physical machine remains powered on, responsive, and stable.
