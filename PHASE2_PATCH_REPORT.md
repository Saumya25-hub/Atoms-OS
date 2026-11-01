# ATOMS OS — PHASE 2 PATCH REPORT (TASK 3)

**Phase:** Phase 2 — HRE (Universal HID 1.11 Bytecode Parser Engine)  
**Date:** 2026-11-02  
**Author:** ATOMS OS Patch Team  
**Scope of Changes:** Strictly limited to `kernel/drivers/usb/class/hid_parser.h`, `kernel/drivers/usb/class/hid_parser.c`, `kernel/drivers/usb/class/usb_hid.c`, and `build.ps1`.  
**Forbidden Modules Touched:** ZERO (`kernel.c`, GDT, IDT, PMM, VMM, BSPE untouched).

---

## 1. Files Added / Modified

| File | Status | Lines Changed | Description |
|---|---|---|---|
| `kernel/drivers/usb/class/hid_parser.h` | **NEW** | +103 | HID 1.11 Item tags, usage pages, generic desktop usages, `hid_mouse_field_map_t` struct, and HRE public API prototypes. |
| `kernel/drivers/usb/class/hid_parser.c` | **NEW** | +264 | Universal HID 1.11 Bytecode Parser Engine, dynamic unaligned bitfield extractor (`hid_extract_field`), and deterministic zero-heap mouse report decoder (`hid_decode_mouse_report`). |
| `kernel/drivers/usb/class/usb_hid.c` | **MODIFIED** | +72, -18 | Fetches HID Report Descriptor universally for all interfaces during `usb_hid_bind()`, attaches `mouse_map` to `udim_endpoint_ctx_t`, and dynamically decodes incoming mouse reports with Phase 1 boot protocol fallback. |
| `build.ps1` | **MODIFIED** | +1, -2 | Configured compilation of `kernel/drivers/usb/class/hid_parser.c` to `build/hid_parser.o` and eliminated duplicate symbol linkage. |

---

## 2. Functions Created & Modified

### A. `kernel/drivers/usb/class/hid_parser.c`
1. `hid_extract_field(const uint8_t* report, uint32_t length_bytes, uint32_t bit_offset, uint32_t bit_size, bool is_signed)`
   - **Functionality:** Extracts unaligned bit sequences (1 to 32 bits) spanning arbitrary byte boundaries and performs two's complement sign-extension for signed axes.
   - **Complexity:** $O(1)$ runtime, zero dynamic allocations.

2. `hid_parse_mouse_report_desc(const uint8_t* desc, uint16_t len, hid_mouse_field_map_t* out_map)`
   - **Functionality:** Bounded bytecode state machine parsing USB HID 1.11 Main, Global, and Local items. Computes exact bit positions for Report ID, X axis, Y axis, Scroll Wheel, and up to 16 buttons.
   - **Safety:** Bounds-checked loop, fixed stack depth (8 levels) for Push/Pop items.

3. `hid_decode_mouse_report(const hid_mouse_field_map_t* map, const uint8_t* report, uint32_t length_bytes, int32_t* out_dx, int32_t* out_dy, uint8_t* out_buttons, int32_t* out_wheel)`
   - **Functionality:** Matches Report ID, extracts button states bit-by-bit, and extracts X/Y/Wheel deltas using the compiled field map.

### B. `kernel/drivers/usb/class/usb_hid.c`
1. `usb_hid_bind(USBDevice* dev, USBInterfaceDescriptor* interface_desc, void* config_desc_buffer, uint16_t total_length)`
   - **Change:** Removed `if (interface_desc->bInterfaceProtocol == 1)` restriction on `GET_DESCRIPTOR (USB_DESC_HID_REPORT)`. Report Descriptors are now fetched for mice and composite interfaces. Calls `hid_parse_mouse_report_desc()` and stores the parsed map into `udim_endpoint_ctx_t`.

2. `usb_hid_report_received_udim(USBDevice* dev, uint8_t dci, uint8_t* report, uint32_t length)`
   - **Change:** Inspects endpoint `ctx->mouse_map`. If valid, decodes with `hid_decode_mouse_report()`. If descriptor parsing was unavailable or failed, falls back seamlessly to the certified Phase 1 boot protocol decoder.

---

## 3. Zero-Regression Architecture Verification

1. **Dual-Path Decoding:** If any non-standard device fails report descriptor parsing, `ctx->mouse_map.valid` remains false, triggering immediate fallback to certified Phase 1 Boot Protocol decoding (`length == 3, 4, 5`).
2. **Keyboard Isolation:** Keyboards continue to route strictly through `hid_keyboard_handle_report()` with independent UDIM DCI binding.
3. **Deterministic Memory Footprint:** The HRE engine uses only stack-allocated structures. Zero dynamic heap (`malloc`/`kalloc`) is used during report descriptor parsing or packet extraction.
