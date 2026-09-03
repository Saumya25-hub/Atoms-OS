# FORENSIC REPORT — XHCI EVENT RING DESYNCHRONIZATION VIA REENTRANT XHCI_POLL()

**Case ID**: `CASE_20260903_XHCI_EVENT_RING_CORRUPTION`  
**Date**: September 3, 2026  
**Investigator**: Forensic Team  
**Physical Target**: ASUS B750M-K (Intel Core i3-14100F, LGA1700 Architecture)  
**Symptom**: On physical hardware, 1 keypress (NumLock) turns LED ON, then the keyboard completely freezes; `xHCI Transfer Events` stops advancing at exactly 4; subsequent presses are ignored.

---

## 1. Physical Forensic Evidence from Screenshot 221135_s63

From physical hardware screenshot `screenshot_20260903_221135_s63.png`:
```
4. TRANSACTION AUDIT LOG (LAST SYNCHRONIZATIONS)
   #  Reason      Num  Caps  Scr  Data  Iface  Code  Status
   1  BOOT_INIT   OFF  OFF   OFF  0x00  0      1     SUBMITTED_OK
   2  NUM_CHANGE  ON   OFF   OFF  0x01  0      1     SUBMITTED_OK

5. REAL-TIME XHCI HARDWARE PIPELINE TELEMETRY
   Configure EP Command : Code=1
   xHCI Transfer Events : 4
   USB Reports Received : 4
   Last Event Slot / DCI: Slot=4 DCI=3
   Last Event Code      : Code=1
```
- The initial `NUM_CHANGE` control transfer executed with `Code=1 (TRB_SUCCESS)`.
- Physical NumLock LED illuminated on the physical keyboard.
- Immediately following this transaction, `xHCI Transfer Events` froze permanently at **4**.
- Zero further events were ever dequeued from the xHCI Event Ring.

---

## 2. Root Cause Analysis: Reentrant Event Ring Corruption

### Mechanism of the Failure:
1. In `kernel/drivers/usb/host/xhci/xhci.c:352-430`:
   ```c
   while (true) {
       XHCITrb* trb = &ring->trbs[ring->dequeue];
       ...
       if (type == TRB_TRANSFER_EVENT) {
           xhci_handle_transfer_event(slot_id, completion_code, transfer_length, trb);
       }
       
       ring->dequeue++;  // <── LINE 415: AFTER THE HANDLER!
       ...
   }
   *erdp = new_erdp | (1 << 3); // <── LINE 429: AFTER THE WHOLE LOOP!
   ```
2. When Event 2 (NumLock keypress) was processed:
   `xhci_handle_transfer_event()` called `usb_hid_report_received()`.
   `usb_hid_report_received()` called `usb_hid_sync_leds_ex("NUM_CHANGE")`.
   `usb_hid_sync_leds_ex()` called `xhci_control_transfer()`.
3. In `kernel/drivers/usb/host/xhci/xhci_transfer.c:189`:
   ```c
   while (!g_xhci_ep0_transfer_complete[slot_id]) {
       xhci_poll(); // <── RECURSIVE INVOCATION OF xhci_poll()!
   }
   ```
4. **The Collision**:
   - The outer `xhci_poll()` was paused at line 402 with `ring->dequeue` pointing at Event 2.
   - The nested `xhci_poll()` entered while `ring->dequeue` was STILL pointing at Event 2!
   - The nested `xhci_poll()` read Event 2 AGAIN, or processed events and advanced `ring->dequeue` to Event 4, updating `*erdp`.
   - When the control transfer finished, execution returned to the outer `xhci_poll()`.
   - The outer `xhci_poll()` then executed line 415 (`ring->dequeue++`) on an ALREADY-ADVANCED dequeue pointer, advancing it a SECOND time for the same event!
   - The outer `xhci_poll()` then wrote a desynchronized `new_erdp` to the xHCI register at line 429.
   - This desynchronized `ring->dequeue` and inverted `ring->cycle` relative to the hardware producer cycle state.
   - Consequently, `cycle != ring->cycle` at line 356 became permanently true for all future events, causing `xhci_poll()` to break immediately every time.

---

## 3. Files Involved

- [`kernel/drivers/usb/host/xhci/xhci.c`](file:///d:/Signatures_OS/kernel/drivers/usb/host/xhci/xhci.c): Event ring dequeue and ERDP update logic in `xhci_poll()`.

---

## 4. Suspected Fix Strategy (NO CODE IN THIS PHASE)

1. **Pop and Acknowledge Before Dispatch**:
   In `xhci_poll()`, copy the event TRB to a local stack variable (`XHCITrb local_trb = *trb;`), advance `ring->dequeue`, update `ring->cycle`, and update `*erdp` IMMEDIATELY BEFORE invoking any event handler (`xhci_handle_transfer_event`).
2. **Reentrancy Immunity**:
   Because `ring->dequeue` and `*erdp` are updated prior to dispatch, any nested call to `xhci_poll()` (e.g. from a control transfer wait loop) will see only subsequent events (such as EP0 completion) and will never re-process or double-increment the current event.
