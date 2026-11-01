# ATOMS OS — IMPERIAL ARCHITECTURAL SPECIFICATION
## Phase 2: HRE (Universal HID 1.11 Bytecode Parser Engine)

**Subsystem:** Human Interface & Pointer Architecture  
**Author:** ATOMS OS Core Architecture Group  
**Target:** Universal USB HID Compatibility (Generic, Multi-button, Gaming, High-DPI 8K mice)  
**Historical Models:** Linux Kernel (`drivers/hid/hid-core.c`, `hid-input.c`) & Windows NT (`HIDCLASS.SYS`, `HIDPARSE.SYS`, `MOUHID.SYS`)  
**Date:** 2026-11-02  
**Status:** ARCHITECTURAL SPECIFICATION APPROVED  

---

## 1. The Engineering Problem

Operating systems that rely on hardcoded packet offsets (e.g. `report[0]` for buttons, `report[1]` for dX, `report[2]` for dY) work only for basic 3-byte legacy Boot Protocol mice.

When connecting modern hardware:
1. **Gaming Mice (Logitech G502, Razer Basilisk, Corsair):**
   - Transmit 8-byte, 12-byte, or 16-byte packets.
   - Use dynamic Report IDs: Byte 0 is `Report ID` (e.g. `0x01` or `0x02`), not buttons!
   - Under hardcoded drivers, `report[0] = 0x01` is interpreted as **Left Button Clicked**, causing the left click to become permanently stuck down.
2. **High-DPI Sensors (1,600 to 30,000 DPI):**
   - Displacements exceed $[-127, 127]$. Sensors send 12-bit or 16-bit signed two's-complement coordinates ($dX, dY \in [-32768, 32767]$).
   - Casting `report[1]` to `(int8_t)` truncates the high bits, causing high-speed movement to invert direction or jerk violently.
3. **Multi-Button Ergonomic Mice:**
   - Feature 5 to 16 buttons (Left, Right, Middle, Back, Forward, DPI Shift, Profile, Macro).
   - Button bits span 2 or more bytes, followed by padding bits. Fixed-offset decoders ignore all buttons past Button 3.

---

## 2. Comparative Analysis: Linux vs. Windows NT vs. ATOMS OS HRE

```
+-----------------------------------------------------------------------------+
|                               USB WIRE (xHCI)                               |
|                  Raw Interrupt IN Packet Stream (Variable Length)           |
+--------------------------------------+--------------------------------------+
                                       |
        +------------------------------+------------------------------+
        |                                                             |
        v                                                             v
+-------------------------------+                             +-------------------------------+
|     LINUX KERNEL PIPELINE     |                             |     WINDOWS NT PIPELINE       |
+-------------------------------+                             +-------------------------------+
| 1. drivers/hid/usbhid/        |                             | 1. HIDUSB.SYS                 |
|    hid-core.c                 |                             |    USB HID Minidriver         |
| 2. hid_parse_report():        |                             | 2. HIDPARSE.SYS               |
|    Bytecode Parser            |                             |    Parses Report Descriptor   |
| 3. struct hid_field           |                             | 3. PHIDP_PREPARSED_DATA       |
|    Bit offset & size mapping  |                             |    Opaque collection tree     |
| 4. hid-input.c:               |                             | 4. MOUHID.SYS / MOUCLASS.SYS  |
|    Dynamic bit extraction     |                             |    HidP_GetUsageValue()       |
| 5. evdev / input_event        |                             | 5. MOUSE_INPUT_DATA / Win32k  |
+-------------------------------+                             +-------------------------------+
                                       |
                                       v
        +-------------------------------------------------------------+
        |                 ATOMS OS HRE ARCHITECTURE                   |
        +-------------------------------------------------------------+
        | 1. UDIM (Unified Device & Interface Manager)                |
        |    Per-endpoint DMA buffers, isolated interface contexts     |
        | 2. HRE Bytecode Parser (kernel/drivers/usb/class/hid_parser)|
        |    Zero-heap bounded state machine (Main, Global, Local)     |
        | 3. hid_mouse_field_map_t                                    |
        |    Bit-exact descriptor map for X, Y, Wheel, Buttons (1..16)|
        | 4. Dynamic Bitfield Extractor                               |
        |    hid_extract_field(): unaligned bit extraction + sign ext |
        | 5. HIDA Subsystem -> BSPE Cursor Presenter                  |
        +-------------------------------------------------------------+
```

### Detailed Structural Mapping:

| Capability | Linux (`hid-core.c`) | Windows NT (`HIDPARSE.SYS`) | ATOMS OS HRE V2 |
|---|---|---|---|
| **Memory Allocation** | Dynamic `kmalloc` per field | Kernel NonPagedPool allocation | Bounded static per-endpoint map (`hid_mouse_field_map_t`) — **Zero Heap Allocation** |
| **Parsing Time** | Device probe (`hid_probe`) | Device start (`IRP_MN_START_DEVICE`) | Device bind (`usb_hid_bind`) |
| **Report ID Handling** | Indexed in `hid_device->report_enum` | Indexed per `ReportID` in collection | Direct validation against `map->report_id` |
| **Coordinate Range** | Scaled via `input_absinfo` or raw `EV_REL` | Sign-extended 32-bit integer | Sign-extended 32-bit integer with full dynamic range |
| **Buttons** | Linux Keycodes (`BTN_MOUSE`, `BTN_SIDE`, etc.) | Bitmask in `MOUSE_INPUT_DATA.Buttons` | Hardware-neutral bitmask in `hida_push_relative` |

---

## 3. HID 1.11 Item Parser Mechanics

HID Report Descriptors consist of variable-length bytecode items:

### Item Prefix Format (1 byte):
```
  Bit:   7   6   5   4   3   2   1   0
       +---------------+-------+-------+
       |   Item Tag    | Type  | Size  |
       +---------------+-------+-------+
```
- **Size (bits 0..1):**
  - `0`: 0 bytes of data
  - `1`: 1 byte of data
  - `2`: 2 bytes of data
  - `3`: 4 bytes of data
- **Type (bits 2..3):**
  - `0`: **Main Item** (`Input`, `Output`, `Feature`, `Collection`, `End Collection`)
  - `1`: **Global Item** (`Usage Page`, `Logical Min`, `Logical Max`, `Report Size`, `Report ID`, `Report Count`, `Push`, `Pop`)
  - `2`: **Local Item** (`Usage`, `Usage Min`, `Usage Max`)

### Parser State Tracking in HRE:
1. When `Usage Page == 0x01` (Generic Desktop):
   - `Usage == 0x30` $\rightarrow$ X Axis displacement.
   - `Usage == 0x31` $\rightarrow$ Y Axis displacement.
   - `Usage == 0x38` $\rightarrow$ Vertical Scroll Wheel.
2. When `Usage Page == 0x09` (Buttons):
   - `Usage Min == 1, Usage Max == N` $\rightarrow$ Sequential button bits $1 \dots N$.
3. When `Input` item is encountered:
   - For each declared usage, the parser advances the bit offset:
     $$\text{Bit Offset}_{\text{new}} = \text{Bit Offset}_{\text{current}} + (\text{Report Size} \times \text{Report Count})$$
   - Records the exact bit offset and bit size for X, Y, Wheel, and Buttons.

---

## 4. Universal Bitfield Extraction Engine

Extracting arbitrary unaligned bitfields from a byte stream requires handling:
1. **Arbitrary bit offsets:** The field may start on any bit (e.g. Bit 3).
2. **Multi-byte spans:** A 16-bit value starting on Bit 4 spans 3 bytes (`byte[0]`, `byte[1]`, `byte[2]`).
3. **Sign Extension:** If the field is declared as signed (`Logical Minimum < 0`):
   ```c
   if (is_signed && (value & (1 << (bit_size - 1)))) {
       value |= ~((1 << bit_size) - 1); // Two's complement sign-extension to 32 bits
   }
   ```

---

## 5. Architectural Quality Guarantees

1. **Zero Heap Allocation:** No dynamic `kmalloc` or `malloc` during packet processing. High-speed 8K Hz polling runs with deterministic sub-microsecond latency.
2. **Buffer Overrun Immunity:** Parser strictly enforces `i + bSize <= len` on every bytecode token.
3. **Seamless Fallback:** If a legacy device lacks a Report Descriptor or fails bytecode validation, ATOMS OS defaults automatically to the Phase 1 Boot Protocol engine.
