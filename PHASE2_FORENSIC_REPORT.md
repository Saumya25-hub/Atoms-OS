# ATOMS OS — FORENSIC INVESTIGATION REPORT (TASK 1)

**Phase:** Phase 2 — HRE (Universal HID 1.11 Bytecode Parser Engine)  
**Date:** 2026-11-02  
**Author:** ATOMS OS Forensic Group  
**Rule 0 Compliance:** Investigation & Evidence Only (NO CODE MODIFICATIONS)  

---

## 1. Executive Summary

Phase 1 successfully resolved the immediate bare-metal hardware failure on **ASUS PRIME B760M-K (Intel Core i3-14100F)** by deploying the **UDIM (Unified Device & Interface Manager)** and the **Anti-Misrouting Gate**. This stopped the physical mouse from typing keyboard letters and restored cursor tracking.

However, an in-depth forensic audit of the ATOMS OS input stack reveals a critical architectural limitation that restricts the operating system from supporting universal USB HID devices:
1. **Mouse Report Descriptors are never fetched:** In `kernel/drivers/usb/class/usb_hid.c:863`, `GET_DESCRIPTOR (USB_DESC_HID_REPORT)` is strictly gated behind `if (interface_desc->bInterfaceProtocol == 1)` (Keyboards only). Mice are never queried for their Report Descriptors.
2. **Hardcoded Packet Offsets:** In `usb_hid.c:640-720`, mouse reports are decoded using fixed array indices (`report[0]`, `report[1]`, `report[2]`, `report[3]`). The decoder guesses the field layout based solely on packet byte length (`length == 3`, `4`, `5`, or `6`).
3. **Gaming & High-DPI Mouse Incompatibility:** High-performance gaming mice (Logitech G502, Razer DeathAdder, Corsair), high-polling (1K–8K Hz) mice, and multi-button devices utilize custom Report IDs, 12-bit/16-bit coordinate fields, and multi-byte button masks. In the current engine, a Report ID byte shifts the entire packet, causing left clicks to become stuck or deltas to be interpreted as buttons.

---

## 2. Forensic Evidence & Root Cause Analysis

### Evidence A: Mouse Report Descriptors Are Omitted Entirely
In `kernel/drivers/usb/class/usb_hid.c`, lines 862–895:
```c
// If keyboard, query HID descriptor and Report Descriptor, then send initial LED report
if (interface_desc->bInterfaceProtocol == 1) {
    uint8_t* p = (uint8_t*)interface_desc + interface_desc->bLength;
    uint8_t* p_end = (uint8_t*)config_desc_buffer + total_length;
    while (p < p_end) {
        ...
        if (h->bDescriptorType == 0x21 && h->bLength >= sizeof(USBHIDDescriptorHeader)) {
            // GET_DESCRIPTOR(USB_DESC_HID_REPORT) executed ONLY FOR KEYBOARDS!
            ...
        }
    }
}
```
**Finding:** When an optical or gaming mouse binds (`bInterfaceProtocol == 2` or `0`), the driver never requests `GET_DESCRIPTOR (HID Report)`. It has zero metadata regarding the device's actual packet structure, Report IDs, or bit offsets.

---

### Evidence B: Hardcoded Heuristic Offsets
In `kernel/drivers/usb/class/usb_hid.c`, lines 640–690:
```c
if (length == 3) {
    buttons = report[0];
    dx = (int)(int8_t)report[1];
    dy = (int)(int8_t)report[2];
} else if (length == 4) {
    if (report[0] <= 0x07) {
        buttons = report[0];
        dx = (int)(int8_t)report[1];
        dy = (int)(int8_t)report[2];
        scroll = (int)(int8_t)report[3];
    } else {
        buttons = report[1];
        dx = (int)(int8_t)report[2];
        dy = (int)(int8_t)report[3];
    }
}
```
**Finding:**
1. A 4-byte report from a mouse with 5 buttons (where button byte can be up to `0x1F`) fails the `report[0] <= 0x07` test and is misparsed as a Report ID packet.
2. A mouse utilizing Report ID 1 with a 3-byte payload produces a 4-byte packet `[01, btn, dx, dy]`. The current logic treats `report[0] = 0x01` as buttons (Left Button Pressed) and shifts all coordinate deltas.
3. 16-bit displacements (common in high-DPI sensors $>1600$ DPI) are completely mangled when truncated to `(int8_t)`.

---

## 3. Industry Comparison: Linux & Windows NT Implementations

### Linux Kernel (`drivers/hid/hid-core.c`)
- **Item-Based Bytecode Parser:** Linux implements a state machine parsing 1-byte, 2-byte, and 4-byte items:
  - **Main Items:** `Input` (0x80), `Output` (0x90), `Feature` (0xB0), `Collection` (0xA0), `End Collection` (0xC0).
  - **Global Items:** `Usage Page` (0x04), `Logical Minimum` (0x14), `Logical Maximum` (0x24), `Report Size` (0x74), `Report ID` (0x84), `Report Count` (0x94).
  - **Local Items:** `Usage` (0x08), `Usage Minimum` (0x18), `Usage Maximum` (0x28).
- **Field Descriptors:** Linux builds `struct hid_field` for every declared input variable, recording its exact bit offset in the report bitstream.
- **Dynamic Extraction:** In `hid_input_field()`, values are extracted using bitwise shifts and sign-extension based on `Logical Minimum` and `Report Size`.

### Windows NT (`HIDPARSE.SYS` / `HIDCLASS.SYS`)
- During PnP device start (`IRP_MN_START_DEVICE`), `HIDUSB.SYS` queries the report descriptor.
- `HIDPARSE.SYS` converts the bytecode into a parsed collection tree (`HIDP_COLLECTION_DESC`).
- `MOUHID.SYS` queries `HidP_GetUsageValue()` to extract X, Y, and wheel deltas and `HidP_GetUsages()` for button arrays.

---

## 4. Affected Subsystems & File Inventory

1. `kernel/drivers/usb/class/usb_hid.c`:
   - Fetching of Report Descriptors for both Keyboard and Mouse interfaces.
   - Integration of the HRE bytecode parser.
   - Dynamic report extraction replacing hardcoded array lookups.
2. `kernel/drivers/usb/class/hid_parser.h` / `hid_parser.c`:
   - Standalone, cleanly testable HID 1.11 bytecode item parser and bitfield extraction engine.

---

## 5. Risk Assessment

| Risk Category | Severity | Mitigation Strategy |
|---|---|---|
| **Malformed Report Descriptor** | High | Strict bounds checking on parser pointer `i + bSize <= len`; safe fallback to Boot Protocol if parsing fails |
| **Non-Boot Report ID Mismatch** | Medium | Support multiple Report IDs in a single device; index field map by `report_id` |
| **High Polling Rate CPU Load** | Low | Parse descriptor *once* at device bind time; runtime packet decoding requires only precomputed bit shifts |
| **Regression on Standard Mice** | Zero | If a mouse does not supply a valid Report Descriptor, the existing Phase 1 Boot Protocol fallback remains fully active |

---

## 6. Suspected Fix & Architecture Recommendation

1. Implement **HRE (Universal HID 1.11 Bytecode Parser Engine)**:
   - State machine tracking Global items (`Usage Page`, `Logical Min/Max`, `Report Size`, `Report Count`, `Report ID`), Local items (`Usage`, `Usage Min/Max`), and Main items (`Input`, `Collection`).
   - Output structured Field Map:
     - Button bit offsets (Button 1, 2, 3, 4, 5...)
     - X displacement (bit offset, bit size, signed flag, min/max)
     - Y displacement (bit offset, bit size, signed flag, min/max)
     - Wheel displacement (bit offset, bit size, signed flag)
2. Implement **Dynamic Bitfield Extractor**:
   - `hre_extract_bits(data, bit_offset, bit_size, is_signed)`
3. Query and parse Report Descriptor for all HID interfaces during `usb_hid_bind()`.
4. Route packets through HRE when a parsed field map is active; fall back to Phase 1 Boot Protocol otherwise.

**Task 1 Verdict:** Forensic Investigation complete. Ready for Task 2 Architecture Plan.
