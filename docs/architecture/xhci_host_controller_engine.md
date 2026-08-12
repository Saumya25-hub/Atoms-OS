# ATOMS OS — xHCI Host Controller Engine & PCI Power Handoff Specification

## 1. Executive Overview

The **xHCI (eXtensible Host Controller Interface) Engine** serves as the primary hardware controller interface for all USB 1.1, 2.0, 3.0, and 3.1 peripherals in ATOMS OS. On modern x86_64 motherboards (such as Intel Haswell H81, Skylake H110, AMD B450, and QEMU/VMware virtual chipsets), discrete 8042 PS/2 microcontrollers are deprecated or emulated via System Management Mode (SMM).

To ensure that 5V USB VBUS power, RGB LEDs, and interrupt endpoints remain 100% active post UEFI `ExitBootServices()`, ATOMS OS implements a hardware-level **BIOS-to-OS Ownership Handoff (`USBLEGSUP`)** protocol and PCI D0 Power State enforcement.

---

## 2. xHCI Architecture & Memory Registers

```
+-----------------------------------------------------------------------+
|                       PCI CONFIGURATION SPACE                         |
| Class: 0x0C (Bus), Subclass: 0x03 (Serial), ProgIF: 0x30 (xHCI)       |
| BAR0 / BAR1: 64-bit Physical MMIO Base Address                       |
+-----------------------------------------------------------------------+
                                   |
                                   v
+-----------------------------------------------------------------------+
|                    MMIO CAPABILITY REGISTERS                          |
| 0x00: CAPLENGTH (Cap Length) / HCIVERSION (Interface Version)         |
| 0x04: HCSPARAMS1 (Max Slots, Max Interrupters, Max Ports)             |
| 0x08: HCSPARAMS2 (Scratchpad Buffers)                                 |
| 0x10: HCCPARAMS1 (Extended Capabilities Pointer: EECP Bits [31:16])  |
+-----------------------------------------------------------------------+
                                   |
                                   v
+-----------------------------------------------------------------------+
|                 xHCI EXTENDED CAPABILITIES (USBLEGSUP)                |
| Offset = EECP * 4                                                     |
| DWORD 0: CapID = 0x01 (Legacy Support)                                |
|   - Bit 16: BIOS Owned Semaphore                                      |
|   - Bit 24: OS Owned Semaphore                                        |
| DWORD 1: LegSup Control / Status (SMI Enablers)                      |
+-----------------------------------------------------------------------+
```

---

## 3. BIOS-to-OS Ownership Handoff (`USBLEGSUP`) Protocol

When a motherboard boots via UEFI, BIOS firmware claims initial ownership of the xHCI host controller to handle legacy USB keyboard emulation during Aptio BIOS Setup.

### Handoff Sequence Algorithm:
1. **Locate EECP**: Read `HCCPARAMS1` (Capability Register offset `0x10`). Extract `EECP` field (`(HCCPARAMS1 >> 16) & 0xFFFF`).
2. **Traverse Extended List**: While `EECP != 0`:
   - Calculate MMIO offset: `ext_cap = mmio_base + (eecp << 2)`.
   - Read `cap_id = ext_cap[0] & 0xFF`.
   - If `cap_id == 1` (`USB Legacy Support / USBLEGSUP`), proceed to handoff.
   - Otherwise, advance `eecp = (ext_cap[0] >> 8) & 0xFF`.
3. **Claim OS Ownership**:
   - Write `OS_OWNED_SEMAPHORE` (`1 << 24`) to `ext_cap[0]`.
4. **Poll BIOS Release (Non-Blocking Timeout)**:
   - Poll `ext_cap[0]` for `BIOS_OWNED_SEMAPHORE` (`1 << 16`) to become `0`.
   - Timeout: 50 milliseconds (50,000 iterations with 1µs delay).
5. **Disable SMM SMI Traps**:
   - Clear `LegSup Control / Status` (DWORD 1) SMI generation bits (`0xE0000000`) to prevent SMM system pauses during OS interrupt transfers.

---

## 4. Intel Haswell H81 xHCI Port Multiplexing (`XUSB2PR` / `USB3_PSSEN`)

On Intel Haswell PCH chipsets (H81/B85/Z87), physical USB ports are routed through dual PCH multiplexers between EHCI (USB 2.0) and xHCI (USB 3.0).

```
                      +-------------------+
                      | Physical USB Port |
                      +-------------------+
                                |
             +------------------+------------------+
             |                                     |
             v                                     v
   +-------------------+                 +-------------------+
   | EHCI Controller   |                 | xHCI Controller   |
   | (Legacy USB 2.0)  |                 | (Native USB 3.0)  |
   +-------------------+                 +-------------------+
             ^                                     ^
             |                                     |
      [ XUSB2PR = 0 ]                       [ XUSB2PR = 0xFFFFFFFF ]
```

By writing `0xFFFFFFFF` to PCI Register `0xD0` (`XUSB2PR`) and `0xD8` (`USB3_PSSEN`), ATOMS OS programmatically forces all physical motherboard ports to the xHCI host controller, overriding BIOS "Smart Auto" power cuts.

---

## 5. Non-Blocking System Resilience Policy

In compliance with ATOMS OS Core Rules:
- **No Infinite Loops**: All hardware register polling loops MUST include a strict iteration timeout.
- **Fail-Safe Handoff**: If BIOS fails to clear `BIOS_OWNED_SEMAPHORE` within 50ms, the xHCI engine forces controller reset (`HCRST`) and proceeds without hanging the system.
