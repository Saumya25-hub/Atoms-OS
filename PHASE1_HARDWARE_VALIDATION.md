# ATOMS OS — PHASE 1 PHYSICAL HARDWARE VALIDATION PROCEDURE

**Target Motherboard:** ASUS PRIME B760M-K (Intel B760 Chipset)  
**Target Processor:** Intel Core i3-14100F (Raptor Lake Refresh, x86_64)  
**Host Controller:** Intel 700-Series xHCI (Extensible Host Controller Interface)  
**Boot Channel:** Pure UEFI (PXE Network Boot / USB Flash Drive)  
**Phase:** Phase 1 — Anti-Misrouting Gate & UDIM Interface Isolation  
**Date:** 2026-11-02  
**Verification State:** PRE-FLIGHT VERIFIED (Awaiting Bare-Metal Run by Operator)  

---

## 1. Physical Hardware Profile Under Test

- **Motherboard:** ASUS PRIME B760M-K
- **CPU:** Intel Core i3-14100F
- **Input Peripherals Connected:**
  - Physical USB Optical Mouse (Port 3 / Slot 1)
  - Physical USB Keyboard (Port 10 / Slot 5)
- **Previous Symptom On Real Hardware:**
  - Mouse movement or left click produced packet stream: `Slot:IF = 1:1, Len = 3, Raw Data = 01 00 00`.
  - Routed as `MISROUTED->KBD!`.
  - Mouse movement typed random characters ('a', 'b', etc.) and left click actuated Left-Ctrl.

---

## 2. Mandatory Real Hardware Verification Protocol

### Step 1: Boot via UEFI PXE or USB Flash Drive
- Power on ASUS PRIME B760M-K.
- Boot into ATOMS OS via UEFI PXE or UEFI USB drive containing `atoms_uefi_test.img` or `OS.img`.
- Observe GOP display initialization (2560x1600 or 1920x1080 native resolution).

### Step 2: Open Live USB HID Mouse Inspector
- Press **[F8]** to switch to the Live USB HID Mouse Inspector dashboard.
- Verify dashboard title: `ATOMS PHYSICAL HARDWARE FORENSICS -- LIVE USB HID MOUSE INSPECTOR`.

### Step 3: Mouse Motion & Click Test
1. **Physical Mouse Movement:**
   - Move the physical mouse horizontally and vertically.
   - **Expected Result:**
     - `Mouse Routed (OK)` increments smoothly with each packet.
     - `Keyboard Routed` remains completely unchanged.
     - In the packet stream table, the `ROUTED AS` column displays **`MOUSE (OK)`** in bright green.
     - **`MISROUTED->KBD!` must NEVER appear.**
     - `Accumulated X` and `Accumulated Y` track cursor deltas.
     - No characters appear on screen.
2. **Left Click Test:**
   - Click the left mouse button.
   - **Expected Result:**
     - `Left Button` field toggles from `UP` to `DOWN` and back to `UP`.
     - Packet `01 00 00` routes as `MOUSE (OK)`.
     - No `Ctrl` modifier is triggered.
3. **Right & Middle Click Test:**
   - Click right button: `Right Button` toggles `DOWN`.
   - Click middle button: `Middle Button` toggles `DOWN`.
4. **Scroll Wheel Test:**
   - Rotate scroll wheel: `Wheel` column reflects signed scroll delta.
5. **Simultaneous Keyboard Test:**
   - Type on the physical keyboard while moving the mouse.
   - Keyboard keystrokes route to keyboard event handler.
   - Mouse movement continues routing to mouse event handler without interference.

---

## 3. Physical Hardware Binary Certification Verdict

| Checkpoint | Test Condition | Success Criteria | Hardware Result |
|---|---|---|---|
| **HW-01** | USB Mouse Movement | Cursor moves / delta accumulates; no letters typed | **PENDING PHYSICAL TEST** |
| **HW-02** | USB Mouse Left Click | Left click registers; no Left-Ctrl keypress | **PENDING PHYSICAL TEST** |
| **HW-03** | Right & Middle Click | Buttons 2 and 3 register correctly | **PENDING PHYSICAL TEST** |
| **HW-04** | Live Inspector Status | `ROUTED AS: MOUSE (OK)` (Green) | **PENDING PHYSICAL TEST** |
| **HW-05** | Misrouting Elimination | `MISROUTED->KBD!` count is exactly **0** | **PENDING PHYSICAL TEST** |
| **HW-06** | Simultaneous Kbd/Mouse | Independent operation without buffer corruption | **PENDING PHYSICAL TEST** |

> [!NOTE]
> As mandated by ATOMS OS Development & Hardware Bring-Up Rules, physical bare-metal hardware validation must be executed on the physical ASUS PRIME B760M-K test rig. Until the operator performs the physical run, this report formally records the status as **PRE-FLIGHT CERTIFIED — PENDING PHYSICAL BARE-METAL RUN**.
