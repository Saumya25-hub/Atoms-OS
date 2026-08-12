# ATOMS OS — Universal USB HID Class Driver (Mouse & Keyboard) Specification

## 1. Executive Overview

The **Universal USB HID (Human Interface Device) Class Driver** implements the USB Device Class Definition for HID Revision 1.11. On x86_64 hardware, USB Mice and USB Keyboards connect as HID peripherals on the xHCI host controller.

To eliminate complex Report Descriptor parsing failures across diverse keyboard/mouse manufacturers (Logitech, Razer, Corsair, Dell, HP), ATOMS OS enforces the **USB Boot Protocol Fallback (`SET_PROTOCOL = 0`)**. This forces all connected USB input devices to transmit standardized, fixed-size data reports.

---

## 2. USB Boot Protocol Specifications

### 2.1 USB Mouse Boot Protocol Report (3 Bytes)

```
+-------------------------------------------------------------+
| Byte 0: Buttons (Bit 0: Left, Bit 1: Right, Bit 2: Middle)  |
| Byte 1: X Relative Displacement (Signed 8-bit Integer)      |
| Byte 2: Y Relative Displacement (Signed 8-bit Integer)      |
+-------------------------------------------------------------+
```

### 2.2 USB Keyboard Boot Protocol Report (8 Bytes)

```
+-------------------------------------------------------------+
| Byte 0: Modifier Keys Bitmap                                |
|   - Bit 0: Left Ctrl     - Bit 4: Right Ctrl                |
|   - Bit 1: Left Shift    - Bit 5: Right Shift               |
|   - Bit 2: Left Alt      - Bit 6: Right Alt                 |
|   - Bit 3: Left GUI/Win  - Bit 7: Right GUI/Win             |
| Byte 1: Reserved (0x00)                                     |
| Byte 2..7: Key Array (Up to 6 Simultaneous USB HID Keys)    |
+-------------------------------------------------------------+
```

---

## 3. Keycode Translation Matrix (USB HID Usage Table -> BOS Keycodes)

| USB HID Usage ID | Character / Function | BOS Keycode Constant |
| :--- | :--- | :--- |
| `0x04` - `0x1D` | `'a'` - `'z'` | `BOS_KEY_A` - `BOS_KEY_Z` |
| `0x1E` - `0x27` | `'1'` - `'0'` | `BOS_KEY_1` - `BOS_KEY_0` |
| `0x28` | Enter (`\n`) | `BOS_KEY_ENTER` |
| `0x29` | Escape | `BOS_KEY_ESC` |
| `0x2A` | Backspace (`\b`) | `BOS_KEY_BACKSPACE` |
| `0x2B` | Tab (`\t`) | `BOS_KEY_TAB` |
| `0x2C` | Space (`' '`) | `BOS_KEY_SPACE` |
| `0x3A` - `0x45` | F1 - F12 | `BOS_KEY_F1` - `BOS_KEY_F12` |
| `0x4F` | Right Arrow | `BOS_KEY_RIGHT` |
| `0x50` | Left Arrow | `BOS_KEY_LEFT` |
| `0x51` | Down Arrow | `BOS_KEY_DOWN` |
| `0x52` | Up Arrow | `BOS_KEY_UP` |

---

## 4. Class Driver Binding Sequence

1. **Interface Detection**: Match `bInterfaceClass == 0x03` (HID).
2. **Endpoint Discovery**: Locate Interrupt IN Endpoint (`bEndpointAddress & 0x80`).
3. **SET_PROTOCOL Command**: Send Control Transfer `bRequest = 0x0B, wValue = 0` (Boot Protocol).
4. **SET_IDLE Command**: Send Control Transfer `bRequest = 0x0A, wValue = 0` (Indefinite Idle).
5. **Interrupt Transfer Submission**: Submit continuous Interrupt IN transfer ring requests on xHCI.
