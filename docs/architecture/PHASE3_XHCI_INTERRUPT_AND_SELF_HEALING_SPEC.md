# ATOMS OS — PHASE 3: xHCI INTERRUPT MIGRATION & BSME SELF-HEALING ARCHITECTURE

**Title:** Enterprise-Grade xHCI Event Handling & Autonomous Input Guardian  
**Target Silicon:** Intel Core i3-14100F (Raptor Lake Refresh, B760M-K) & Haswell H81  
**Architectural References:**  
- Linux Kernel: `drivers/usb/host/xhci-pci.c`, `drivers/usb/host/xhci-ring.c` (`xhci_irq()`)  
- Windows NT: `USBXHCI.SYS` (Interrupter 0 DPC / ISR Pipeline)  
- xHCI Specification 1.2: Section 4.17 (Interrupters), Section 5.5.2 (Interrupter Registers)  

---

## 1. Executive Vision

Currently, ATOMS OS services xHCI events via cooperative software polling (`xhci_poll()`). While polling achieves sub-millisecond responsiveness during active user interaction, it incurs two fundamental architectural limitations:
1. **CPU Quantum Waste:** The CPU burns millions of instruction cycles continuously checking the Event Ring Dequeue Pointer (`ERDP`) even when no USB transfer is pending.
2. **Starvation Under Heavy Load:** If any task or display routine holds the CPU for $> 5\text{ms}$, incoming USB packets accumulate in hardware transfer rings, creating perceived input jitter.

**Phase 3 Goal:** Migrate from busy-polling to an **asynchronous hardware interrupt pipeline** using PCI MSI / MSI-X and Local APIC delivery, integrated with the **BSME (Autonomous Self-Healing Guardian)** for automatic endpoint recovery and hot-plug reconnects.

---

## 2. Hardware Architecture Comparison

```
+--------------------------------------------------------------------------+
| WINDOWS NT 11 (USBXHCI.SYS)                                              |
| Hardware Completion TRB -> PCI MSI-X -> APIC Vector -> ISR -> DPC Worker |
+--------------------------------------------------------------------------+
| LINUX (drivers/usb/host/xhci-ring.c)                                     |
| Hardware Completion TRB -> PCI MSI-X -> xhci_irq() -> tasklet / kthread  |
+--------------------------------------------------------------------------+
| ATOMS OS PHASE 3 ARCHITECTURE                                            |
| Hardware Completion TRB -> PCI MSI-X -> Local APIC (0x40) ->             |
| xhci_isr() [Clear EINT/IP/EHB] -> UDIM/HRE Nano-Ring Dispatch            |
+--------------------------------------------------------------------------+
```

---

## 3. Detailed Component Design

### A. PCI MSI / MSI-X Capability Discovery
The PCI configuration space of the xHCI host controller (`0x0C:0x03:0x30`) is scanned for capability structures starting from offset `0x34`:
1. **MSI-X (`Capability ID 0x11`):**
   - Table Size: $N$ interrupter vectors.
   - Table Offset & BIR (Base Address Register Indicator).
   - Message Address Register: Programmed with Local APIC destination (`0xFEE00000 | (apic_id << 12)`).
   - Message Data Register: Vector `0x40` (Delivery Mode Fixed, Edge Triggered).
2. **MSI Fallback (`Capability ID 0x05`):**
   - For legacy Haswell Lynx Point controllers lacking MSI-X: Program 64-bit Address `0xFEE00000` and Data `0x0040`.

### B. xHCI Interrupter 0 Programming
xHCI Runtime Base (`RTSOFF`) offset `0x20` exposes Interrupter 0 registers:
1. **`IMAN` (Interrupter Management, Offset 0x00):**
   - Bit 0: `IP` (Interrupt Pending - RW1C).
   - Bit 1: `IE` (Interrupt Enable). Set to 1 to assert physical bus line.
2. **`IMOD` (Interrupt Moderation, Offset 0x04):**
   - Programmed with `1000` (250 microseconds moderation interval = 4 kHz throttle).
   - Prevents interrupt storms during 8K polling gaming mice while ensuring $< 0.25\text{ms}$ dispatch latency.
3. **`USBCMD` (Operational Register 0x00):**
   - Set Bit 2: `INTE` (Interrupter Enable). Must be 1 to enable hardware interrupt delivery.

### C. Zero-Stall Interrupt Service Routine (`xhci_isr`)
When hardware completes a packet:
1. **Validate Event:** Inspect `USBSTS` Bit 3 (`EINT`). If not set, return false (shared IRQ safe).
2. **Clear Controller Flags:**
   - Write `USBSTS.EINT = 1` (RW1C).
   - Write `IMAN.IP = 1` (RW1C).
3. **Drain Ring:** Harvest pending TRBs from `g_xhci_event_ring`.
4. **Advance ERDP:** Write updated Dequeue Pointer with Bit 3 (`EHB = 1`) to clear hardware busy gate.
5. **Local APIC EOI:** Write `0` to APIC EOI register (`0xFEE000B0`).

---

## 4. BSME: Autonomous Self-Healing Guardian

If a physical USB cable experiences electrostatic discharge (ESD) or high-DPI transfer overflow:
1. **Completion Code Validation:** Detects `CC_STALL_ERROR (6)` or `CC_TRB_ERROR (5)`.
2. **Stall Recovery Pipeline:**
   - Issues `TRB_TYPE_RESET_ENDPOINT` to xHCI Command Ring.
   - Issues `TRB_TYPE_SET_TR_DEQUEUE` to restore endpoint ring alignment.
   - Clears USB halt state via Control Endpoint EP0 (`CLEAR_FEATURE(ENDPOINT_HALT)`).
3. **Hot-Plug Re-Bind:** If a device is unplugged and re-plugged on physical root ports, UDIM invalidates the old Slot context and binds the new slot without requiring an OS reboot.
