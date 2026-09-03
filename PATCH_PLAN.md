# ARCHITECTURE PLAN — XHCI EVENT RING DEQUEUE & REENTRANCY ISOLATION

**Document ID**: `PATCH_PLAN_20260903_XHCI_EVENT_RING_CORRUPTION`  
**Architect**: Architecture Team  
**Input Document**: `FORENSIC_REPORT.md`  
**Physical Target**: ASUS B750M-K (Intel Core i3-14100F, LGA1700 Architecture)  

---

## 1. What to Modify

### File: `kernel/drivers/usb/host/xhci/xhci.c`
- **Function**: `xhci_poll()`
- **Modification**:
  1. Restructure the event ring consumption order:
     - Check cycle bit against `ring->cycle`.
     - Copy event TRB to a local stack variable: `XHCITrb event = *trb;`.
     - Advance `ring->dequeue` and toggle `ring->cycle` immediately.
     - Update ERDP register (`*erdp = new_erdp | (1 << 3)`) immediately to commit consumption to xHCI hardware.
     - Dispatch event handlers using the local stack `event` copy.
  2. Protect against reentrant state interference by decoupling TRB consumption from downstream event processing.

---

## 2. Why This is Necessary

When a keypress is handled, `usb_hid_sync_leds_ex()` submits an EP0 Control Transfer. The control transfer wait loop calls `xhci_poll()` recursively to await EP0 completion.
Under the previous implementation, the outer `xhci_poll()` had not yet advanced `ring->dequeue` or updated ERDP when invoking the handler. The nested `xhci_poll()` saw the same un-popped event, double-incremented `ring->dequeue`, and wrote an out-of-sync ERDP pointer to hardware. This corrupted the event ring state after exactly 4 events, permanently freezing keyboard event reception.

---

## 3. Expected Results

1. **Continuous Keyboard Event Reception**:
   - `xHCI Transfer Events` counter increments continuously on every key press and key release.
   - No freezing at 4 events.
2. **Smooth LED Toggling**:
   - Press Caps Lock: LED turns ON (Event 5).
   - Release Caps Lock: Key release processed (Event 6).
   - Press Caps Lock again: LED turns OFF (Event 7).
   - Press Num Lock: Toggles ON/OFF reliably.
3. **No Regressions**:
   - Command completion events continue working.
   - Cooperative screenshots continue streaming.
   - Live heartbeat continues advancing.

---

## 4. Risk Analysis & Rollback Plan

- **Risk**: Very low. Advances the dequeue pointer immediately upon popping the TRB, which is standard xHCI ring consumer semantics.
- **Rollback Plan**: Revert `kernel/drivers/usb/host/xhci/xhci.c` via git checkout.
