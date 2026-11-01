# ATOMS OS — PHASE 3 PATCH PLAN

**Document ID:** `PATCH_PLAN_PHASE3_XHCI_INTERRUPT_AND_SELF_HEALING`  
**Input:** `PHASE3_FORENSIC_REPORT.md`  
**Target Hardware:** Intel Core i3-14100F (ASUS B760M-K) & Intel Core i3-4130 (Haswell H81)  
**Protocol Phase:** TASK 2 — ARCHITECT TEAM (No Code Modifications)  
**Date:** November 2, 2026  

---

## 1. What to Modify & Why

### Component 1: Hardware Interrupter & Command Definitions
- **File:** `kernel/drivers/usb/host/xhci/xhci.h`
- **Why:** Provide symbolic constants for `USBCMD.INTE` (Bit 2), `USBSTS.EINT` (Bit 3), `IMAN.IP` (Bit 0), `IMAN.IE` (Bit 1), and `ERDP.EHB` (Bit 3). Declare function prototypes for `xhci_isr`, `xhci_set_tr_dequeue_ptr`, and BSME telemetry variables.

### Component 2: TRB Set TR Dequeue Pointer Command
- **File:** `kernel/drivers/usb/host/xhci/xhci_cmd.c`
- **Why:** When an endpoint stalls or babble occurs, the xHCI controller transitions the endpoint context to the Halted state. Clearing the halt requires issuing `TRB_SET_TR_DEQUEUE_PTR_CMD` (Command 16) to re-align the controller's hardware dequeue pointer with software's transfer ring state.

### Component 3: PCI MSI-X / MSI Enablement & Zero-Stall ISR
- **File:** `kernel/drivers/usb/host/xhci/xhci.c`
- **Why:** 
  1. Scan PCI capabilities of the xHCI controller for Capability `0x11` (MSI-X) and `0x05` (MSI).
  2. Program Target Message Address to BSP Local APIC (`0xFEE00000`) and Message Data to Vector `0x40`.
  3. Program Interrupt Moderation Register (`IMOD`) to `1000` (250 microseconds moderation).
  4. Enable `USBCMD.INTE` (Bit 2) so the host controller asserts MSI/MSI-X upon event completion.
  5. Register `xhci_isr` with `isr_register_handler(0x40, xhci_isr)`.
  6. Within `xhci_isr`, atomically clear `USBSTS.EINT`, `IMAN.IP`, drain the event ring, write `ERDP` with `EHB = 1`, and write Local APIC EOI (`0xFEE000B0`).

### Component 4: BSME Autonomous Stall Recovery Guardian
- **File:** `kernel/drivers/usb/host/xhci/xhci_transfer.c`
- **Why:** In `xhci_handle_transfer_event`, intercept non-success completion codes (`CC_STALL_ERROR = 6`, `CC_BABBLE_DETECTED_ERROR = 7`, `CC_TRB_ERROR = 5`). Execute the 4-step recovery sequence:
  1. `xhci_reset_endpoint(slot_id, dci)`.
  2. `xhci_set_tr_dequeue_ptr(slot_id, dci, dequeue_phys, ring->cycle)`.
  3. EP0 `CLEAR_FEATURE(ENDPOINT_HALT)`.
  4. Immediately requeue the endpoint's report buffer so user input never freezes.

### Component 5: Live Diagnostic Telemetry Integration
- **File:** `kernel/debug/mouse_debug.c`
- **Why:** Present real-time telemetry on the F8 Live USB HID Mouse Inspector showing:
  - Interrupt Delivery Mode: `MSI-X (Active)` / `MSI (Active)` / `Polled (Cooperative)`.
  - Real-time Hardware IRQ counter (`g_xhci_interrupts`).
  - BSME Autonomous Recoveries counter (`g_bsme_stall_recoveries`).

---

## 2. Expected Results

1. **Continuous 1000 Hz Fluid Pointer Movement:** Hardware interrupts deliver mouse packets directly to the pointer engine without waiting for CPU cooperative polling intervals.
2. **Zero Dropped Packets During High Load:** Full-screen dashboard redraws or disk operations will no longer cause input starvation or micro-stutters.
3. **Resilience Against Cable Noise & Disconnects:** If a mouse is unplugged and re-plugged or encounters electrostatic noise, BSME clears the endpoint halt automatically within $<10\text{ms}$ without kernel panic.
4. **Clean Verification on QEMU and Bare-Metal:** System compiles cleanly with zero unresolved symbols and boots seamlessly in pure UEFI mode.

---

## 3. Risk Assessment & Safety Guards

1. **Local APIC EOI Safety:** If an interrupt arrives while `0xFEE00000` is unmapped or inaccessible, a page fault could occur.  
   *Guard:* Local APIC physical address `0xFEE00000` is mapped in kernel page tables with `PAGE_CACHE_DISABLE`. A valid pointer check guards the EOI write.
2. **Reentrancy Protection:** If `xhci_poll` is invoked while `xhci_isr` is executing, ring state corruption could occur.  
   *Guard:* Implement an atomic spin-lock/busy flag `g_xhci_event_processing` in `xhci.c` ensuring single-threaded event ring consumption.
3. **Graceful Fallback:** If the platform does not support MSI/MSI-X (e.g. legacy QEMU configuration), the driver logs the fallback and continues running in cooperative polling mode without failure.

---

## 4. Rollback Plan

If unexpected behavior occurs during test execution:
1. Revert to stable checkpoint commit `17dcd9d3753a473f324d9c72ec1cbb6611586e35` (Tag `v2.2-fluid-mouse-stable`).
2. Rebuild with `powershell -ExecutionPolicy Bypass -File .\build.ps1`.
3. Verify baseline functionality in QEMU.
