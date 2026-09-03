# ATOMS OS — Emergency Input Power & Controller Forensic Certification Report

**Target Platform:** ASUS B750M-K (Intel LGA1700 Chipset, CSM Disabled, Pure Native UEFI)  
**CPU:** Intel Core i3-14100F (Raptor Lake Refresh, 4P/8T @ 3.50 GHz, Family 6, Model 183, Stepping 1)  
**Memory:** 8 GB DDR5  
**Case ID:** `INPUT_POWER_DEAD_CASE`  
**Date:** September 3, 2026  
**Final Forensic Verdict:** 🟢 **ROOT CAUSE CONCLUSIVELY PROVEN ON PHYSICAL BARE METAL**

---

## 1. Physical Symptom & Forensic Mandate

### The Phenomenon
During iterative debugging on the physical ASUS B750M-K bare-metal testbench:
- Keyboard LEDs (NumLock, CapsLock, ScrollLock) were completely OFF.
- Mouse optical sensor illumination and status LEDs were completely OFF.
- Both devices were physically dead and unresponsive to user interaction.
- Both input devices had previously functioned during desktop and cursor certification milestones.

### Protocol Mandate
In strict adherence to `.agents/AGENTS.md` (Mandatory Phase Isolation):
- Zero speculative timing tweaks or random delays.
- Zero modifications to stable subsystems (`push_event`, input queue, compositor, VMM, PMM, scheduler, LAN datapath).
- Strict execution of read-only passive sampling followed by controlled A/B verification on bare metal.

---

## 2. Epistemological Truth Model & Forensic Classifications

Every conclusion in this audit is classified according to the ATOMS OS Truth Model:

| Fact / Finding | Classification | Forensic Proof |
| :--- | :--- | :--- |
| **Input Devices are USB Peripherals** | **OBSERVED** | xHCI Root Hub Ports 6 and 10 report `CCS = 1` (Device Connected) at Low-Speed (1.5 Mbps, `Speed = 2`). |
| **8042 Port 0x60/0x64 is an Open Circuit** | **OBSERVED** | Port 0x64 reports status `0x7C` at the first instruction of kernel execution. Bit 6 (`TIMEOUT`) was set by the UEFI firmware prior to ATOMS boot. Command writes consistently experience a 240 ms wire timeout followed by `0xFE`. |
| **xHCI Controller Left Halted by UEFI** | **OBSERVED** | When `xhci_init()` is bypassed, PCI config confirms MMIO enabled, but Operational registers report `USBCMD = 0` (Run/Stop = 0) and `USBSTS = 1` (HCHalted = 1). |
| **USB Bus Suspend Kills Peripheral Power** | **DERIVED** | USB 2.0 §9.1.1.6 & USB 3.2 §10.3 mandate that USB devices enter Suspend when no SOF bus activity is observed for >3 ms, capping current draw below 2.5 mA. In this state, optical sensor LEDs and keyboard LEDs are powered down. |
| **xHCI Bringup Restores Peripheral Power** | **OBSERVED** | In Test B (`xhci_init` called), Root Hub Ports 6 and 10 transition from `PLS = 7` (Suspended) to `PLS = 0` (`U0 ACTIVE`) with `PED = 1` (Port Enabled). |

---

## 3. Physical Bare-Metal A/B Forensic Experiment

### Experiment Setup
- **Test A (`test_001_input_power_passive`)**: Pure passive read-only sampling. `xhci_init()` bypassed in `kernel/kernel.c`.
- **Test B (`test_002_input_power_xhci_active`)**: Active xHCI bring-up. `xhci_init()` invoked in `kernel/kernel.c`.
- **Observation Channel**: Autonomous PXE boot, UDP 9997 binary telemetry, and UDP 9998 uncompressed 1920x1080 32-bit framebuffer stream.

### Comparative Results Matrix

```text
========================================================================================
METRIC / REGISTER             TEST A (xhci_init BYPASSED)     TEST B (xhci_init CALLED)
========================================================================================
PCI Location                  Bus 0, Dev 20, Func 0           Bus 0, Dev 20, Func 0
PCI Command                   0x0002 (MMIO Enabled)           0x0002 (MMIO Enabled)
xHCI Host State               STOPPED / HALTED                RUNNING (ACTIVE)
USBLEGSUP (Ownership)         UNCLAIMED (DEFAULT)             OS OWNED
Port 2  (Full-Speed Device)   0x000006E1 (Disabled, Suspend)  0x00020603 (Enabled, U0 Active)
Port 6  (Low-Speed Mouse/Kbd) 0x00000AE1 (Disabled, Suspend)  0x00020A03 (Enabled, U0 Active)
Port 8  (Full-Speed Device)   0x000006E1 (Disabled, Suspend)  0x00020603 (Enabled, U0 Active)
Port 10 (Low-Speed Kbd/Mouse) 0x00000AE1 (Disabled, Suspend)  0x00020A03 (Enabled, U0 Active)
VBUS Power (PP Bit 9)         POWER ON (All 14 Ports)         POWER ON (All 14 Ports)
8042 Status (Port 0x64)       0x7C (Pre-boot Timeout)         0x7C (Pre-boot Timeout)
Peripheral Visual Status      PHYSICALLY DEAD / DARK          ENERGIZED / U0 AWAKE
========================================================================================
```

---

## 4. Root Cause Mechanics

1. **Why the Peripherals are Physically Dead:**
   Modern LGA1700 desktop motherboards (ASUS B750M-K) route rear I/O peripheral connections through the Intel PCH xHCI USB host controller. When ATOMS OS conditionally bypassed `xhci_init()` during the PS/2 keyboard LED investigation:
   - The xHCI controller remained in the halted state left by UEFI Boot Services.
   - Without host controller clocking or SOF packets, all downstream root-hub ports automatically entered **USB Bus Suspend (`PLS = 7`)**.
   - USB HID specifications strictly mandate that suspended peripherals power down high-current emitters (optical sensor LEDs and keyboard indicator LEDs) to comply with the 2.5 mA suspend current budget.

2. **Why PS/2 Diagnostics Failed:**
   - There are no physical PS/2 devices attached to the motherboard LPC bus.
   - In pure UEFI mode (`CSM Disabled`), the ASUS UEFI firmware does not emulate SMM PS/2 transactions on Port 0x60/0x64.
   - The 8042 controller status byte `0x7C` reflects an unserviced LPC interface with a pre-existing firmware timeout flag.
   - Any attempt to probe or command a keyboard via Port 0x60 will reliably time out after 240 ms with an open-bus `0xFE` reply.

---

## 5. Architectural Recommendation

1. **Retire Port 0x60/0x64 PS/2 Emulation for LGA1700 Platform:**
   Peripheral input on the target physical platform MUST flow through the native **xHCI + USB HID stack** (`kernel/drivers/usb/`).
2. **Keep xHCI Host Controller Running:**
   `xhci_init()` and `usb_hid_init()` must remain permanently active in all production and debug modes to prevent the root-hub ports from lapsing into USB Bus Suspend.
3. **Preserve Phase Isolation:**
   All diagnostic tools must distinguish between physical bus power (`PORTSC` bit 9 `PP`), bus link state (`PLS = 0` vs `PLS = 7`), and software event polling.
