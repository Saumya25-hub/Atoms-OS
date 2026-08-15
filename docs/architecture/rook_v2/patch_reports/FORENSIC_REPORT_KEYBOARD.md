# 🔬 ATOMS OS — FORENSIC INVESTIGATION REPORT
## Target Hardware: Intel Core i3 Haswell LGA1150 / H81 Motherboard / 8GB RAM
## Issue: Physical Hardware Keyboard Non-Responsive on Lock Screen (Works in QEMU, Dead on Bare-Metal)

**Report ID:** `FORENSIC-H81-KEYBOARD-PIPELINE-001`  
**Phase:** `TASK 1 — FORENSIC TEAM (NO CODE MODIFICATION)`  
**Investigator:** Antigravity Forensic Engineering Team  

---

### 1. Forensic Executive Summary

Physical hardware bring-up on Haswell H81 bare-metal reveals that while the display, wallpaper, clock, and date engines render at 60 FPS, pressing keys on a physical keyboard fails to unlock the system or transition to the Sign-In state. The keyboard LED is powered ON (5V VBUS active), but keystrokes never trigger the login transition.

Forensic investigation across the input subsystem, USB stack, 8042 PS/2 driver, and ROOK event loop uncovered **THREE CONCURRENT ARCHITECTURAL ROOT CAUSES**:

1. **Root Cause A (ROOK Loop xHCI Starvation):** `rook_login_spin()` runs a dedicated frame loop before the multi-task scheduler starts. It calls `rook_update()` and `rook_render()`, but **NEVER polls `xhci_poll()`**. USB transfer rings and event TRBs from physical USB keyboards are never harvested from the Haswell Lynx Point xHCI controller during the lock screen.
2. **Root Cause B (USB HID to `keyboard.c` Disconnection):** Even when USB packets are processed by `usb_hid.c`, the events are dispatched to `hida_push_keyboard_event()` $\rightarrow$ `kernel_input_push_key_event()` $\rightarrow$ `event_queue` in `input.c`. However, `page_login.c` polls `keyboard_poll_event()` which reads exclusively from `kbd_buffer[]` in `keyboard.c`. USB keypresses bypass `kbd_buffer[]` entirely!
3. **Root Cause C (Uninitialized 8042 PS/2 Controller on UEFI H81):** `ps2_init()` in `drivers/input/ps2/ps2.c` only flushes port `0x60`. It never sends `0xAE` (Enable 1st Port), never configures the 8042 Command Byte (`0x20`/`0x60`) to enable IRQ 1 generation (`bit 0 = 1`), and never sends `0xF4` (Enable Scanning) to the keyboard. On physical 2022 UEFI H81 boards, UEFI leaves 8042 IRQ generation disabled at `ExitBootServices()`.

---

### 2. Forensic Evidence & Code Trace

#### Evidence A: `rook_login_spin()` Event Starvation
* **File:** `kernel/shell/rook/src/rook_core.c:186-211`
* **Evidence:** The supervisor loop executes:
  ```
  while (g_current_page && g_current_page->id == ROOK_PAGE_LOGIN) {
      rook_update(16);
      rook_render();
      // Hardware TSC delay...
  }
  ```
* **Impact:** `xhci_poll()` is only called in `scheduler_on_tick()`. Because the scheduler is not yet running during `rook_login_spin()`, hardware USB Interrupt-IN endpoints are completely starved.

#### Evidence B: Pipeline Disconnect Between USB HID & `page_login.c`
* **File 1:** `kernel/shell/rook/pages/page_login.c:679`
  * Calls `keyboard_poll_event(&key_evt)` which reads `kbd_buffer` in `kernel/drivers/keyboard/src/keyboard.c`.
* **File 2:** `kernel/drivers/usb/class/usb_hid.c:147`
  * Calls `hida_push_keyboard_event(HIDA_BACKEND_USB_KBD, &kevt)` $\rightarrow$ routes to `input.c:event_queue`.
* **File 3:** `kernel/drivers/keyboard/src/keyboard.c:21`
  * `kbd_buffer` is ONLY written inside `keyboard_irq_handler()` (IRQ 1). USB keyboards never write to `kbd_buffer`.

#### Evidence C: 8042 Controller Configuration Void
* **File:** `drivers/input/ps2/ps2.c:7-15`
  * Only contains a 1000-count loop reading `0x60`. No initialization commands (`0xAE`, `0x20`/`0x60`, `0xF4`) are ever issued to port `0x64`.

---

### 3. Risk Analysis

* **Low Risk:** Bridging `hida_push_keyboard_event` directly into `keyboard_push_event()` unifies USB and PS/2 keyboards into a single deterministic stream.
* **Low Risk:** Adding `xhci_poll()` and `ps2_poll()` to `rook_update()` guarantees zero-latency input polling on both bare-metal hardware and emulators.
* **Subsystems Involved:** `kernel/shell/rook/src/rook_core.c`, `kernel/drivers/keyboard/src/keyboard.c`, `kernel/drivers/input/core/hida.c`, `drivers/input/ps2/ps2.c`.

---

### 4. Suspected Architecture Fix (NO CODE)

1. **Expose `keyboard_push_event(const KeyboardEvent* ev)` in `keyboard.c`:** Allow USB HID and HIDA to insert decoded keystrokes into `kbd_buffer` so both PS/2 and USB keyboards share the exact same ring buffer.
2. **Hook HIDA to `keyboard.c`:** In `hida_push_keyboard_event()`, invoke `keyboard_push_event()` so every USB keystroke arrives immediately in `keyboard_poll_event()`.
3. **Add Input Polling to `rook_update()`:** Call `xhci_poll()` and PS/2 controller status check during each frame of the login loop so keystrokes are processed with $< 1\text{ms}$ latency without waiting for the scheduler.
4. **Complete 8042 Controller Initialization:** Send proper 8042 commands (`0xAE`, enable IRQ 1/12 via command byte `0x60`, enable scanning `0xF4`) for PS/2 keyboards and legacy USB emulation.
