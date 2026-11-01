# ATOMS OS — ARCHITECTURE PATCH PLAN (TASK 2)

**Phase:** Phase 2 — HRE (Universal HID 1.11 Bytecode Parser Engine)  
**Input:** `PHASE2_FORENSIC_REPORT.md`  
**Date:** 2026-11-02  
**Author:** ATOMS OS Architecture Group  
**Rule 0 Compliance:** Architecture Plan Only (NO CODE MODIFICATIONS)  

---

## 1. WHAT TO MODIFY

The following files are authorized for modification and introduction in Phase 2:
1. `kernel/drivers/usb/class/hid_parser.h` *(New)* — HID 1.11 Item tags, Collection types, and Field Map structure definitions.
2. `kernel/drivers/usb/class/hid_parser.c` *(New)* — HRE Item Parser and Dynamic Bitfield Extractor implementation.
3. `kernel/drivers/usb/class/usb_hid.c` — Universal Report Descriptor fetching for all HID interfaces, and integration of HRE dynamic decoding.
4. `build.ps1` — Compilation of `kernel/drivers/usb/class/hid_parser.c` into the kernel object graph.

*Strict Prohibition:* `kernel.c`, `kernel/core/*`, `display/*`, `bovisual/*`, and all unrelated subsystems shall NOT be touched.

---

## 2. WHY MODIFY THESE FILES?

1. **Eliminate Hardcoded Packet Offsets:** Fixed array indices (`report[0]`, `report[1]`, `report[2]`) in `usb_hid.c` break on any mouse that uses Report IDs, multi-byte button masks, or 16-bit motion coordinates.
2. **Universal Compatibility:** High-performance gaming mice (Logitech G502, Razer DeathAdder, Corsair), high-polling 1K–8K Hz mice, and multi-button input devices can only be parsed dynamically via their declared HID Report Descriptors.
3. **Layer Isolation:** Implementing the item parser in `hid_parser.c` separates bytecode parsing from the USB class driver, following the architectural pattern of Linux (`drivers/hid/hid-core.c`) and Windows (`HIDPARSE.SYS`).

---

## 3. EXACT ARCHITECTURAL DESIGN

### A. Data Structures (`hid_parser.h`)

```c
#define HID_MAX_BUTTONS 16

typedef struct {
    bool     valid;
    bool     has_report_id;
    uint8_t  report_id;
    uint32_t report_length_bytes;

    /* Motion Axes */
    uint16_t x_bit_offset;
    uint8_t  x_bit_size;
    bool     x_is_signed;
    int32_t  x_logical_min;
    int32_t  x_logical_max;

    uint16_t y_bit_offset;
    uint8_t  y_bit_size;
    bool     y_is_signed;
    int32_t  y_logical_min;
    int32_t  y_logical_max;

    /* Scroll Wheel */
    bool     has_wheel;
    uint16_t wheel_bit_offset;
    uint8_t  wheel_bit_size;
    bool     wheel_is_signed;

    /* Buttons (1 to 16) */
    uint8_t  button_count;
    uint16_t button_bit_offset[HID_MAX_BUTTONS];
} hid_mouse_field_map_t;
```

---

### B. HRE Parser State Machine (`hid_parser.c`)

1. **Parser State Tracking:**
   - **Global Items:** `Usage Page`, `Logical Minimum`, `Logical Maximum`, `Report Size`, `Report Count`, `Report ID`.
   - **Local Items:** Array of declared `Usages` (or `Usage Minimum` to `Usage Maximum` range).
   - **Main Items:** `Input` (commits declared fields to current bit offset), `Collection`, `End Collection`.
2. **Dynamic Bitfield Extraction Engine:**
   ```c
   int32_t hid_extract_field(const uint8_t* report, uint32_t length, uint32_t bit_offset, uint32_t bit_size, bool is_signed);
   ```
   - Handles unaligned byte boundaries across arbitrary bit lengths (1 bit to 32 bits).
   - Performs two's-complement sign-extension when `is_signed` is true (e.g. 12-bit signed or 16-bit signed deltas).
3. **Field Mapping Logic:**
   - Detects `Usage Page == 0x01` (Generic Desktop):
     - `Usage == 0x30` $\rightarrow$ Map X Axis.
     - `Usage == 0x31` $\rightarrow$ Map Y Axis.
     - `Usage == 0x38` $\rightarrow$ Map Scroll Wheel.
   - Detects `Usage Page == 0x09` (Buttons):
     - Maps bits sequentially to `Button 1` (Left), `Button 2` (Right), `Button 3` (Middle), `Button 4` (Back), `Button 5` (Forward), etc.

---

### C. Class Driver Integration (`usb_hid.c`)

1. **Fetch Report Descriptor during `usb_hid_bind()`:**
   - Query `GET_DESCRIPTOR (USB_DESC_HID_REPORT)` for all HID interfaces (both Keyboards and Mice).
   - If mouse, parse descriptor via `hid_parse_mouse_report_desc()` and cache `hid_mouse_field_map_t` in the endpoint's UDIM context.
2. **Dynamic Report Decoding in `usb_hid_report_received_udim()`:**
   - Check if the receiving endpoint has an active `hid_mouse_field_map_t`.
   - If active:
     - If `has_report_id && report[0] != report_id`, safely ignore or route to matching secondary collection.
     - Extract `buttons = 0`: Loop over `button_count` and test `hid_extract_field()`.
     - Extract `dx = hid_extract_field(..., map->x_bit_offset, map->x_bit_size, map->x_is_signed)`.
     - Extract `dy = hid_extract_field(..., map->y_bit_offset, map->y_bit_size, map->y_is_signed)`.
     - Extract `scroll = hid_extract_field(..., map->wheel_bit_offset, map->wheel_bit_size, map->wheel_is_signed)`.
     - Forward directly to `hida_push_relative()`.
   - If no report descriptor was parsed:
     - Fall back seamlessly to the verified Phase 1 Boot Protocol decoder.

---

## 4. EXPECTED RESULTS

1. **Universal Hardware Interoperability:** Standard office mice, gaming mice (Logitech G502, Razer, Corsair, SteelSeries), trackballs, and multi-button mice decode with 100% accuracy.
2. **Dynamic Report ID Handling:** Mice utilizing Report IDs (where byte 0 is `0x01` or `0x02`) extract coordinates without axis shifts or stuck buttons.
3. **High-DPI Precision:** 12-bit and 16-bit coordinates maintain full dynamic range with zero truncation.
4. **Zero Regression:** Standard 3-byte and 4-byte boot devices continue functioning without alteration via the fallback path.

---

## 5. RISK ASSESSMENT & ROLLBACK PLAN

- **Risk Level:** Low.
- **Rollback Point:** Git tag `v2.0-mouse-stable` (Commit `4b6bb5c5`) has been pushed to GitHub.
- **Rollback Command:**
  ```powershell
  $env:GIT_CONFIG_GLOBAL = "NUL"
  git checkout v2.0-mouse-stable -- kernel/drivers/usb/class/usb_hid.c
  powershell -ExecutionPolicy Bypass -File .\build.ps1
  ```

---

**Architectural Approval:** Plan locked. Awaiting user sign-off to proceed to TASK 3 (Patch Execution).
