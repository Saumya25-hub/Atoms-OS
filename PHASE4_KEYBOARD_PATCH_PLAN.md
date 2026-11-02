# ATOMS OS — ARCHITECTURE & PATCH PLAN (TASK 2)
**Subsystem:** Imperial Keyboard Subsystem (Phase 4: ALE, UDIM Unification, TRE Typematic Repeat)  
**Input:** `PHASE4_KEYBOARD_FORENSIC_REPORT.md`  
**Compliance:** Rule 0 Phase Isolation (NO CODE MODIFICATIONS IN THIS STEP)

---

## 1. OBJECTIVE

Deliver an enterprise-grade, low-latency, non-blocking Keyboard Subsystem on bare-metal hardware (ASUS PRIME B760M-K / Raptor Lake & Haswell H81):
1. **Zero-Latency LED Synchronization (ALE):** Completely eliminate the 100ms CPU freeze and reentrancy deadlocks on CapsLock/NumLock toggles.
2. **Authoritative Hardware Discovery (UDIM Binding):** Dynamic slot-aware keyboard binding ensuring live detection, accurate VID/PID, and active slot reporting in forensic telemetry.
3. **Smooth Key Holding & Repetition (TRE):** Full Make/Break lifecycle with autonomous Typematic Auto-Repeat (250ms initial delay, 30Hz repeat rate), enabling gentle, instant F2/F3 screen switching and holding.
4. **Zero-Resource Idle Footprint (KDG Guardian):** Asynchronous lockless architecture with 0% CPU overhead when idle.

---

## 2. MODIFICATION PLAN (FILE-BY-FILE & FUNCTION-BY-FUNCTION)

### A. `kernel/drivers/usb/class/usb_hid.c`
* **Functions to Modify:**
  1. `usb_hid_get_keyboard_device()`:
     - If `s_usb_kbd_dev` is NULL, search `g_udim_endpoints[slot][dci]` for `UDIM_DEV_KEYBOARD`.
     - On match, retrieve `USBDevice*` via `usb_get_device_by_slot(slot)` and cache it as `s_usb_kbd_dev`.
  2. `usb_hid_sync_leds_ex()`:
     - Remove the synchronous call to `usb_hid_set_leds_ex()`.
     - Write the computed `led_mask` and `reason` to lockless atomic variables:
       - `s_ale_pending_mask`
       - `s_ale_pending_reason`
       - `s_ale_has_pending = true`
     - Return immediately (0 nanoseconds execution time).
  3. New function `usb_hid_ale_step(void)`:
     - Check `if (!s_ale_has_pending) return;`
     - Check if xHCI controller is currently processing events (`g_xhci_event_processing`); if so, defer to next step.
     - Latch and clear `s_ale_has_pending = false`.
     - Retrieve `dev = usb_hid_get_keyboard_device()`.
     - Execute `usb_hid_set_leds_ex(dev, mask, reason)`.
  4. `usb_hid_report_received_udim()`:
     - On key press: Update lock states, push Make event (`pressed = true`), and register key with TRE.
     - On key release: Inspect keys in `old_report` that are absent in `report`.
     - Push Break event (`pressed = false`) to `hida_push_keyboard_event()` and `keyboard_push_event()`.
     - Unregister key from TRE if released.

### B. `kernel/drivers/keyboard/src/keyboard.c`
* **Functions to Modify:**
  1. New function `keyboard_tre_step(void)`:
     - Inspect `g_tre_active_keycode`.
     - If a key is active and `timer_get_ticks()` >= `g_tre_next_tick`:
       - Construct repeat `KeyboardEvent` (`.pressed = true`).
       - Enqueue into `kbd_buffer`.
       - Advance `g_tre_next_tick += 32` (approx 30Hz repeat rate).
  2. `keyboard_poll_event()`:
     - Call `keyboard_tre_step()` before checking queue head/tail to ensure repeat events are delivered seamlessly to consumers.

### C. `kernel/debug/usb_hid_led_debug.c`
* **Functions to Modify:**
  1. `audit_locate_usb_keyboard()`:
     - Query `usb_hid_get_keyboard_device()` dynamically.
  2. `audit_render_dynamic_state()`:
     - If `g_usb_hid_led_audit.keyboard_found == false`, re-check `audit_locate_usb_keyboard()`.
     - If found, re-render Section 1 with live Slot ID, VID, PID, and Endpoint configuration.
  3. `usb_hid_led_debug_run()`:
     - In the main interactive loop, call `usb_hid_ale_step()` so LED transfers execute smoothly outside event processing.

### D. `kernel/drivers/usb/host/xhci/xhci.c`
* **Functions to Modify:**
  1. `xhci_poll()`:
     - After `xhci_process_events()` returns and releases `g_xhci_event_processing`, call `usb_hid_ale_step()`.

### E. `kernel/debug/hypervisor_dashboard/hypervisor_dashboard.c`
* **Functions to Modify:**
  1. `hv_poll_developer_toggle_keys()`:
     - Call `usb_hid_ale_step()`.
     - Guard `hypervisor_dashboard_render_frame()` so it only re-renders if `g_hv_dashboard_page` actually changed, eliminating 25ms redraw stalls when F2/F3 are held.

---

## 3. EXPECTED OUTCOMES & METRICS

1. **NumLock / CapsLock:**
   - Latency drops from ~100ms to < 10 microseconds.
   - Zero CPU stalls or frame drops.
   - Transaction Audit Log records `Status: SUCCESS` instead of `ERROR`.
2. **F2 / F3 Screen Toggle & Hold:**
   - Single gentle press immediately switches view.
   - Holding down key repeats view at 30Hz with zero lag.
3. **Forensic Console:**
   - Section 1 displays `DETECTED & ENUMERATED` on `Slot=2` with genuine Vendor and Product IDs.
4. **Pointer / Mouse Stability:**
   - Zero impact on Phase 1-3 mouse functionality.

---

## 4. ROLLBACK & RECOVERY PLAN

- Checked out at clean commit `04fb8ec2` (`v3.0-msix-input-stable`).
- If any keyboard regression is detected:
  - `git checkout -- kernel/drivers/usb/class/usb_hid.c kernel/drivers/keyboard/src/keyboard.c kernel/debug/usb_hid_led_debug.c kernel/drivers/usb/host/xhci/xhci.c kernel/debug/hypervisor_dashboard/hypervisor_dashboard.c`
  - Revert takes < 5 seconds with zero residual side effects.

---
**Verdict:** Architecture and Patch Plan approved. Proceeding to TASK 3 (Implementation).
