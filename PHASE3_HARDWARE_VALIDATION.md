# ATOMS OS — PHASE 3 HARDWARE VALIDATION GUIDE

**Document ID:** `HARDWARE_VALIDATION_PHASE3`  
**Target Hardware:** ASUS PRIME B760M-K (Intel Core i3-14100F, Raptor Lake Refresh) & Intel Core i3-4130 (Haswell H81)  
**Verification Binary:** `build\atoms_uefi_test.img` (or PXE `build\BOOTX64.EFI` + `build\kernel.bin`)  
**Status:** Pre-flight Certified. Ready for Physical Bare-Metal Validation.  
**Date:** November 2, 2026  

---

## 1. Physical Hardware Test Checklist

When testing on physical ASUS PRIME B760M-K or Haswell H81:

### A. Live Mouse Inspector (Press F8 on Keyboard)
1. Verify Top Header:
   - "ATOMS PHYSICAL HARDWARE FORENSICS -- LIVE USB HID MOUSE INSPECTOR"
2. Check `[PHASE 3 MSI-X & BSME]` Section in the Left Panel:
   - **Interrupt Mode:** Expected `MSI-X (ACTIVE)` (or `MSI (ACTIVE)` on Haswell).
   - **Hardware IRQs:** Value must actively increment as mouse is moved or clicked.
   - **BSME Recoveries:** Baseline `0`.
3. Check `[DEVICE & PACKET TELEMETRY]` Section:
   - Total HID Packets incrementing smoothly.
   - Mouse Routed (OK) incrementing.
   - Zero packets misrouted to Keyboard.
4. Check `[LIVE CURSOR ACCUMULATOR]`:
   - Accumulated X and Y changing proportionally with movement.
   - Left, Right, and Middle buttons toggle between `UP` and `PRESSED`.

---

## 2. Forensic Stress Questions (Pass/Fail)

| # | Forensic Question | Expected Bare-Metal Verdict |
| :- | :--- | :--- |
| **Q1** | Does the mouse pointer glide continuously without 3-4 second micro-stutters? | **PASS** |
| **Q2** | Does the hardware interrupt counter increment during pointer movement? | **PASS** |
| **Q3** | Does unplugging and re-plugging the mouse cable restore pointer functionality within 1 second? | **PASS** |
| **Q4** | Does rapid clicking or simultaneous keyboard typing cause any dropped packets? | **PASS** |

---

## 3. Physical Test Verdict Recording

- **ASUS PRIME B760M-K Result:** `[ PENDING PHYSICAL FLASH / PXE TEST ]`
- **Haswell H81 Result:** `[ PENDING PHYSICAL FLASH / PXE TEST ]`
