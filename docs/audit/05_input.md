# AUDIT PART 5: INPUT SUBSYSTEM & THE MOUSE REGRESSION

## 1. Full Input Architecture & Data Paths

### 1.1 Hardware to Driver Path
```
Physical USB Hardware (xHCI Host Controller)
      | (Generates Transfer Event on USB Interrupt IN Endpoint)
      v
xhci_poll() [kernel/drivers/usb/host/xhci/xhci.c:450]
      | (Pops Transfer Event TRB from Event Ring)
      v
usb_hid_report_received() [kernel/drivers/usb/class/usb_hid.c:270]
```

### 1.2 Focus Router State Machine (`INPUT_FOCUS_HOST` vs `INPUT_FOCUS_GUEST`)
- [VERIFIED-BY-CODE: kernel/drivers/usb/class/usb_hid.c:305-420]
- Focus states:
  - `INPUT_FOCUS_HOST` (0): Host OS consumes keystrokes; dashboard responds to F1-F4 toggle keys; BWE desktop processes pointer.
  - `INPUT_FOCUS_GUEST` (1): Keystrokes are intercepted before host buffer; forwarded to virtual i8042 PS/2 controller.
- Chords:
  - `Ctrl + Alt + G` [lines 316-328]: Forces focus to `INPUT_FOCUS_HOST`. Sends break scancodes to release all modifiers in guest i8042.
  - `F5` (Usage 0x3E without Ctrl/Alt) [lines 332-342]: Toggles focus between HOST and GUEST.
  - `Ctrl + Alt + Del` [lines 310-315]: Swallowed on host; prevents guest reset.

---

## 2. Root Cause Analysis: Why Moving the Mouse Types Characters into FreeBSD

Three compounding architectural defects in `kernel/drivers/usb/class/usb_hid.c` and `virtual_platform.c` cause mouse movement to be injected as keystrokes into FreeBSD:

### Defect 1: Composite / Wireless Dongle Misclassification (`usb_hid.c:603`)
- [VERIFIED-BY-CODE: kernel/drivers/usb/class/usb_hid.c:603-608]
- **Code Quote**:
  ```c
  // Guard: If this device is already bound as Primary Keyboard, ignore secondary interfaces
  if (s_usb_kbd_dev != NULL && dev == s_usb_kbd_dev && interface_desc->bInterfaceNumber != 0) {
      display_print("[USB HID] Device is already Primary Keyboard (IF 0). Ignoring secondary IF ");
      display_print_dec(interface_desc->bInterfaceNumber);
      display_print("\n");
      return true;
  }
  ```
- **What happens on hardware**:
  - Many modern wireless desktop sets (e.g. Logitech, Dell, HP) use a single composite USB receiver with Interface 0 (Keyboard, Protocol 1) and Interface 1 (Mouse, Protocol 2).
  - `usb_hid_bind` binds Interface 0 as Keyboard and sets `dev->protocol = 1`.
  - When Interface 1 (Mouse) arrives, `usb_hid_bind` explicitly ignores it to 'preserve primary interface 0'.
  - Consequently, `dev->protocol` remains permanently set to `1` (Keyboard).
  - When the mouse is moved, the xHCI driver receives an interrupt report on Interface 1 and passes `dev` to `usb_hid_report_received`.
  - Line 275 checks: `if (protocol == 1 || (dev && dev->protocol == 1)) is_keyboard = true;`.
  - **Result**: Every mouse movement packet from the composite device is classified as a KEYBOARD packet!

### Defect 2: Heuristic Misclassification Fallback (`usb_hid.c:280`)
- [VERIFIED-BY-CODE: kernel/drivers/usb/class/usb_hid.c:275-283]
- **Code Quote**:
  ```c
  bool is_keyboard = false;
  if (protocol == 1 || (dev && dev->protocol == 1)) {
      is_keyboard = true;
  } else if (dev && dev->protocol == 2) {
      is_keyboard = false;
  } else if (length == 8 && report[1] == 0) {
      is_keyboard = true;
  }
  ```
- **What happens on hardware**:
  - Standalone USB mice operating in Report Protocol (where `dev->protocol == 0`) frequently send 8-byte reports for high-resolution 16-bit displacements or multi-button mice.
  - When the mouse moves vertically (dX = 0), `report[1]` is 0.
  - The condition `else if (length == 8 && report[1] == 0)` evaluates to `true`!
  - The mouse packet is treated as a keyboard packet.

### Defect 3: Mouse Displacements Injected as Keyboard Scancodes into i8042 (`usb_hid.c:375-390`)
- [VERIFIED-BY-CODE: kernel/drivers/usb/class/usb_hid.c:375-390]
- **Code Quote**:
  ```c
  if (atoms_input_get_focus() == INPUT_FOCUS_GUEST) {
      /* Deliver Key Press (Make) events */
      for (int b = 2; b < 8; b++) {
          uint8_t uid = report[b];
          if (uid == 0) continue;
          bool is_new_k = true;
          for (int k = 2; k < 8; k++) {
              if (old_report[k] == uid) { is_new_k = false; break; }
          }
          if (is_new_k) {
              atoms_input_forward_hid_to_i8042(true, uid);
          }
      }
      return;
  }
  ```
- In a misclassified mouse packet:
  - `report[0]` = Mouse buttons.
  - `report[1]` = dX (low byte).
  - `report[2]` = dX (high byte) or dY.
  - `report[3]` = dY (high byte) or Wheel.
- The loop reads bytes 2 through 7 and treats each non-zero displacement value as a USB HID Keyboard Usage ID (`uid`)!
- For example, if dY = `0x04`, `uid = 0x04` (USB Usage ID for key 'a').
- `atoms_input_forward_hid_to_i8042(true, 0x04)` translates Usage `0x04` to Set 1 scancode `0x1E` (Make 'A') and calls `virtual_i8042_push_byte(plat, 0x1E)`.
- The virtual i8042 asserts IRQ 1; FreeBSD `atkbd` reads port 0x60, receives scancode `0x1E`, and prints 'a' on the console!

---

## 3. Absence of Virtual PS/2 Aux (Mouse) Implementation

- [VERIFIED-BY-CODE: kernel/core/hypervisor/src/virtual_platform.c:680-825, 1720-1760]
- **Status Register Inspection**:
  - `virtual_i8042_push_byte` sets `kbd->status |= 0x01` (Output Buffer Full).
  - Bit 5 (`0x20` = AUX Output Buffer Full) is **NEVER set anywhere in the codebase**.
- **Interrupt Line Inspection**:
  - `virtual_i8042_push_byte` unconditionally asserts `platform->kbd_irq_pending = true` (IRQ 1).
  - IRQ 12 (PS/2 Aux Mouse IRQ) is **NEVER asserted anywhere in the codebase**.
- **Mouse Command Handling**:
  - Command `0xD4` (Write Next Byte to Mouse) responds with:
    `virtual_i8042_push_byte(platform, 0xFA);` [line 743].
    This puts the mouse ACK `0xFA` into the KEYBOARD buffer and fires IRQ 1!
- **Verdict**:
  - There is ZERO virtual PS/2 mouse / AUX hardware emulation.
  - FreeBSD `psm` (PS/2 mouse driver) cannot function because no AUX data or IRQ 12 exists.

---

## 4. Git Diff Analysis Across Milestones

### 4.1 Between `baseline-shell-ok` and `milestone-freebsd-keyboard-ok`
- `usb_hid.c`: Added chord detection (`Ctrl+Alt+G`, `F5`) and the GUEST focus router branch [lines 304-420].
- `virtual_platform.c`: Added `virtual_i8042_push_byte`, `atoms_input_forward_hid_to_i8042`, and scancode translation tables [lines 1720-1970].
- **Why regression started here**: Prior to `milestone-freebsd-keyboard-ok`, `atoms_input_forward_hid_to_i8042` did not exist. Keystrokes (and misclassified mouse reports) were not forwarded to the guest i8042, so the bug was dormant. The moment i8042 forwarding was connected, misclassified mouse reports immediately began emitting characters into FreeBSD.

### 4.2 Between `milestone-freebsd-keyboard-ok` and `HEAD`
- `usb_hid.c`: Added `Ctrl+Alt+Del` swallowing guard on host [lines 310-315] and modifier break codes on focus release [line 327].
- `virtual_platform.c`: Added `[KEYLOG]` telemetry in `virtual_i8042_push_byte` [lines 1735-1746] and preserved break scancodes in `atoms_input_release_all_guest_keys`.

---

## 5. Top 10 Claims Least Sure About (Part 5)

1. Whether standalone USB mice with Boot Protocol (Protocol = 2) ever hit Defect 2 if packet length is strictly 3 or 4 bytes.
2. The exact vendor/product ID of the user's physical mouse on the ASUS B760M-K patient PC.
3. Whether FreeBSD's `moused` daemon was configured to run on `/dev/psm0` or `/dev/sysmouse` in the image.
4. If a USB mouse sends negative displacement (e.g. -1 = 0xFF), which key Usage ID 0xFF translates to in BOS tables.
5. Whether `hida_push_relative` drops events if the host input queue exceeds 256 entries.
6. Whether BWE desktop pointer cursor can be made visible over the guest VirtIO display window.
7. How FreeBSD Xorg server handles mouse input if both `/dev/psm0` and VirtIO input devices are absent.
8. Whether USB HID LED output reports (Caps/Num lock) interfere with mouse interrupt IN transfers on composite endpoints.
9. If guest i8042 FIFO overflows (depth > 64), whether keyboard driver drops scancodes silently without resync.
10. Whether plugging a second physical USB keyboard causes `s_usb_kbd_dev` to overwrite the first device binding.
