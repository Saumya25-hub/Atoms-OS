# ATOMS OS — PHASE 3 FORENSIC REPORT

**Document ID:** `FORENSIC_REPORT_PHASE3_XHCI_INTERRUPT_AND_SELF_HEALING`  
**Target Hardware:** Intel Core i3-14100F (Raptor Lake Refresh, ASUS B760M-K) & Intel Core i3-4130 (Haswell H81)  
**Target Architecture:** x86_64 Long Mode, Pure UEFI Boot, xHCI 1.2 USB Host Controller  
**Protocol Phase:** TASK 1 — FORENSIC TEAM (No Code Modifications)  
**Date:** November 2, 2026  

---

## 1. Executive Summary

Physical testing on real ASUS PRIME B760M-K hardware confirmed that the mouse movement and button clicks function properly with Phase 1 UDIM (Unified Device & Interface Manager) and Phase 2 HRE (Universal HID 1.11 Bytecode Parser Engine). However, two fundamental architectural limitations remain in the current USB subsystem:
1. **Cooperative Software Polling Latency:** The xHCI event ring is serviced exclusively via periodic software polling (`xhci_poll()`). If any kernel subsystem (such as full-screen rendering, memory compaction, or storage I/O) executes for $>2\text{ms}$, incoming mouse and keyboard packets accumulate in the hardware transfer rings. This causes micro-stutter and increased click-to-photon latency.
2. **Absence of Autonomous Self-Healing (BSME):** When an endpoint experiences an electrical glitch, electrostatic discharge (ESD), or transaction stall (`CC_STALL_ERROR = 6`, `CC_BABBLE_DETECTED_ERROR = 7`), `xhci_handle_transfer_event` unconditionally drops the packet and halts TRB replenishment. The affected endpoint remains permanently frozen until an OS reboot.

---

## 2. Forensic Evidence & Root Cause Analysis

### A. Hardware Interrupt Suppression (`USBCMD.INTE == 0`)
- **Location:** `kernel/drivers/usb/host/xhci/xhci.c:300`
- **Evidence:** During xHCI initialization:
  ```c
  *usbcmd |= 1; // Set Run/Stop
  ```
  In xHCI Specification 1.2 (Section 5.4.1), Bit 0 is `R/S` (Run/Stop) and Bit 2 is `INTE` (Interrupter Enable). Although Interrupter 0 `IMAN.IE` (Bit 1) is enabled at line 296, `USBCMD.INTE` remains `0`.
- **Root Cause:** As specified in xHCI Section 4.17, if `USBCMD.INTE` is 0, the host controller hardware suppresses all physical interrupt line assertions and PCI MSI/MSI-X upstream memory write cycles across all interrupters.

### B. PCI MSI / MSI-X Capability Inactive
- **Location:** `kernel/drivers/usb/host/xhci/xhci.c:134-137`
- **Evidence:** The PCI initialization sequence enables PCI Bus Mastering and Memory Space:
  ```c
  pci_enable_memory_space(xhci_dev);
  pci_enable_bus_mastering(xhci_dev);
  ```
  However, it does not query PCI Capability ID `0x11` (MSI-X) or `0x05` (MSI) via the PCI Capability Pointer (`offset 0x34`).
- **Root Cause:** Without programming the Message Address Register (`0xFEE00000 | (lapic_id << 12)`) and Message Data Register (Vector `0x40`), the xHCI controller cannot deliver MSI/MSI-X interrupts to the Intel Local APIC.

### C. Drop-and-Die Behavior on Endpoint Stall
- **Location:** `kernel/drivers/usb/host/xhci/xhci_transfer.c:433-435`
- **Evidence:**
  ```c
  if (completion_code != 1 && completion_code != 13) {
      return;
  }
  ```
- **Root Cause:** If an interrupt endpoint experiences a transfer error (Completion Code 5 = TRB Error, Code 6 = Stall Error, Code 7 = Babble Detected), the transfer handler executes an early `return`. It does NOT issue `TRB_RESET_ENDPOINT_CMD`, does NOT advance or synchronize the transfer ring dequeue pointer, and does NOT requeue an interrupt TRB. Consequently, the endpoint enters a halted state, freezing input from that physical device permanently.

### D. Lack of Interrupt Moderation (IMOD)
- **Location:** `kernel/drivers/usb/host/xhci/xhci.c:293`
- **Evidence:**
  ```c
  *imod = 0; // Zero Interrupt Moderation
  ```
- **Root Cause:** Setting `IMOD = 0` causes an immediate hardware interrupt for every completed packet. For high-polling gaming mice (1000 Hz, 4000 Hz, 8000 Hz), this causes interrupt storms that saturate the CPU pipeline. Moderating to 250 µs (`IMOD = 1000`) caps interrupt frequency at 4 kHz while ensuring sub-millisecond mouse response.

---

## 3. Files Involved

1. `kernel/drivers/usb/host/xhci/xhci.h` (Hardware register bit definitions, MSI/MSI-X constants, command TRB types).
2. `kernel/drivers/usb/host/xhci/xhci.c` (PCI MSI/MSI-X capability programming, `USBCMD.INTE` enable, `xhci_isr` handler, Local APIC EOI, `IMOD` tuning).
3. `kernel/drivers/usb/host/xhci/xhci_cmd.c` (Implementation of `xhci_set_tr_dequeue_ptr` command).
4. `kernel/drivers/usb/host/xhci/xhci_transfer.c` (BSME stall recovery integration in `xhci_handle_transfer_event`).
5. `kernel/drivers/usb/class/usb_hid.c` (UDIM device context re-binding and recovery dispatch).
6. `kernel/debug/mouse_debug.c` (Live telemetry tracking for MSI/MSI-X status, interrupt count, and BSME stall recoveries).

---

## 4. Risk Analysis

| Risk | Severity | Mitigation Strategy |
| :--- | :--- | :--- |
| Interrupt Reentrancy / Race Condition with `xhci_poll` | HIGH | Implement an atomic event processing lock or cycle-state check ensuring TRB dequeue pointer advance is strictly single-threaded. |
| Missing Local APIC EOI leading to CPU interrupt lockup | CRITICAL | Always issue Local APIC EOI write (`*(volatile uint32_t*)0xFEE000B0 = 0`) at the exit of `xhci_isr`, accompanied by clearing `USBSTS.EINT` and `IMAN.IP`. |
| Legacy Platform Incompatibility (No MSI-X support) | MEDIUM | Implement a 2-tier fallback: Try MSI-X -> Try standard MSI -> Fall back to cooperative polling without kernel panic. |
| Infinite Loop during Endpoint Reset | MEDIUM | Bound endpoint reset retries to a maximum of 3 attempts with timeout guards. |

---

## 5. Suspected Fix (NO CODE)

1. **Scan PCI Capabilities:** Iterate through the PCI configuration capability list for the xHCI controller. Discover MSI-X (0x11) or MSI (0x05).
2. **Program MSI/MSI-X:** Configure Message Address to `0xFEE00000` (BSP Local APIC) and Message Data to Vector `0x40`. Unmask vector and enable capability.
3. **Configure Interrupter 0:** Set `IMOD` to 1000 (250 µs moderation). Set `IMAN.IE = 1`. Set `USBCMD.INTE = 1` (Bit 2) alongside `USBCMD.RS = 1`.
4. **Register ISR:** Wire vector `0x40` in the IDT to `xhci_isr`. Inside `xhci_isr`, clear `USBSTS.EINT`, clear `IMAN.IP`, drain event ring, update `ERDP` with `EHB = 1`, and issue Local APIC EOI.
5. **BSME Stall Recovery Pipeline:** In `xhci_handle_transfer_event`, detect `CC_STALL_ERROR`, `CC_BABBLE_DETECTED_ERROR`, and `CC_TRB_ERROR`. Execute endpoint reset, reset transfer ring dequeue pointer, clear halt condition via EP0 control transfer, and requeue the interrupt buffer.
