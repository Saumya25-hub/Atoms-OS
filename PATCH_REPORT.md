# PATCH REPORT — XHCI EVENT RING DEQUEUE & REENTRANCY ISOLATION

**Document ID**: `PATCH_REPORT_20260903_XHCI_EVENT_RING_CORRUPTION`  
**Patch Engineer**: Patch Team  
**Input Documents**: `FORENSIC_REPORT.md`, `PATCH_PLAN.md`  
**Target Hardware**: ASUS B750M-K (Intel Core i3-14100F, LGA1700 Architecture)  

---

## 1. Files Modified

1. [`kernel/drivers/usb/host/xhci/xhci.c`](file:///d:/Signatures_OS/kernel/drivers/usb/host/xhci/xhci.c)

Zero unrelated files modified.

---

## 2. Detailed Modifications

### [`kernel/drivers/usb/host/xhci/xhci.c`](file:///d:/Signatures_OS/kernel/drivers/usb/host/xhci/xhci.c)
- **Function**: `xhci_poll()`
- **Changes**:
  - Moved TRB consumption and ERDP acknowledgment to the start of each event cycle:
    1. Copies event TRB to stack: `XHCITrb event = *trb;`
    2. Increments `ring->dequeue` and toggles `ring->cycle` immediately.
    3. Writes updated `new_erdp` to xHCI hardware register with bit 3 (EHB) set immediately.
    4. Increments `g_xhci_events`.
    5. Dispatches handlers (`xhci_handle_transfer_event`) using the local `&event` copy.
- **Result**:
  - If an event handler triggers a control transfer that calls `xhci_poll()` recursively to await EP0 completion, the event ring has already been popped and acknowledged.
  - Reentrant `xhci_poll()` calls will never re-read the active event, double-increment `ring->dequeue`, or rewind `*erdp`.
- **Lines Changed**: Lines 344–420 (approx 50 lines modified/restructured).

---

## 3. Subsystem Invariance Audit

- USB Keyboard Endpoint-Scoping (Interface 0 -> EP 1): **UNTOUCHED & PRESERVED**
- Cooperative Non-Blocking Screenshot Engine: **UNTOUCHED & PRESERVED**
- HID Report Parser & Keycode Handlers: **UNTOUCHED & PRESERVED**
- Compositor & Window System: **UNTOUCHED & PRESERVED**
- Scheduler & Dispatcher: **UNTOUCHED & PRESERVED**
- PMM & VMM: **UNTOUCHED & PRESERVED**
- Mouse Subsystem: **UNTOUCHED & PRESERVED**
