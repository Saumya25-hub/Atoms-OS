# ♜ ATOMS OS — Architecture Patch Plan
## Real Hardware USB & PS/2 Keyboard/Mouse Unified Pipeline Bring-Up

**Plan ID:** `PATCH-PLAN-H81-UNIFIED-INPUT-V1`  
**Phase:** `TASK 2 — ARCHITECT TEAM (PLANNING PHASE — NO CODE EDIT)`  
**Target Hardware:** Intel Core i3 Haswell LGA1150 / H81 Motherboard / 8GB DDR3 RAM / 2022 UEFI Mode  

---

### 1. Architectural Scope & Objectives

To achieve 100% reliable physical keyboard and mouse operation on real Haswell H81 bare-metal hardware matching Linux and Windows NT input architectures:
1. **Full 8042 PS/2 Controller Industrial Bring-Up:** Send standard 8042 controller configuration commands to enable IRQ 1, IRQ 12, port translation, and scanning on physical hardware.
2. **Unified Input Stream (USB + PS/2):** Connect `usb_hid.c` / `hida.c` directly into `keyboard.c`'s `kbd_buffer[]` via `keyboard_push_event()` so all applications and login pages receive keystrokes uniformly.
3. **Low-Latency Polling in ROOK Login Supervisor Loop:** Integrate `xhci_poll()` into `rook_login_spin()` so USB transfers are processed deterministically every 16.6ms frame.

---

### 2. Files to be Modified

| File | Purpose of Modification |
| :--- | :--- |
| `kernel/drivers/keyboard/include/keyboard.h` | Declare `void keyboard_push_event(const KeyboardEvent* event);` |
| `kernel/drivers/keyboard/src/keyboard.c` | Implement thread-safe `keyboard_push_event()` inserting into `kbd_buffer[]` |
| `kernel/drivers/input/core/hida.c` | Call `keyboard_push_event()` in `hida_push_keyboard_event()` |
| `drivers/input/ps2/ps2.c` | Implement full 8042 controller configuration (0xAE, 0x20/0x60, 0xF4 enable scanning) |
| `kernel/shell/rook/src/rook_core.c` | Call `xhci_poll()` inside `rook_login_spin()` supervisor frame loop |

---

### 3. Expected Results & Telemetry Verification

* **Keystroke Response:** Pressing any key on physical USB or PS/2 keyboard instantly triggers the login unlock animation.
* **Typing Response:** In the Sign-In page, typing alphanumeric characters populates the password box with zero missed keys.
* **CPU / Memory Budget:** 0 heap allocations (`kmalloc = 0`), $<0.01\text{ms}$ input processing overhead per frame.

---

### 4. Rollback & Safety Plan

All changes are isolated to input routing and polling; no memory allocators, page tables, or graphics pipelines are touched. In case of regression, git restore restores the certified Phase 5B state.
