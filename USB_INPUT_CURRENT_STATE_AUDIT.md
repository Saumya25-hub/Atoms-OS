# USB Input Current State Audit

## Audit Findings

After a thorough audit of the `Signatures_OS` repository, here is the current state of USB support:

- **xHCI / EHCI / UHCI / OHCI**: Missing (0 files)
- **USB initialization**: Missing
- **USB device enumeration**: Missing
- **USB descriptors**: Missing
- **USB endpoints**: Missing
- **Control transfers**: Missing
- **Interrupt transfers**: Missing
- **HID class support**: Missing
- **HID report descriptors**: Missing
- **USB mouse support**: Missing
- **USB keyboard support**: Missing

The repository contains only a few fragmented stubs related to USB. Here is the breakdown for every existing USB component:

### 1. `kernel/drivers/input/input.c`
- **What it actually implements**: `local_detect_usb_controller` (scans PCI bus 0-255 looking for Class `0x0C`, Subclass `0x03`) and `local_pci_read_config`.
- **Completeness**: Dead code / Partial.
- **Boot Initialization**: No. The functions exist but are never called in the boot path or anywhere else in `input.c`.
- **HIDA Connection**: No.

### 2. `drivers/input/usb_tablet/usb_tablet.c` (and `.h`)
- **What it actually implements**: 
  - `usb_tablet_probe`: Uses `pci_find_by_class` to find a USB controller, prints a failure message stating the driver is unimplemented, and returns `0`.
  - `usb_tablet_init`: Prints an initialization message to the display.
  - `usb_tablet_report_event`: A hook designed to take absolute X/Y coordinates and push them.
- **Completeness**: Stub.
- **Boot Initialization**: `usb_tablet_init()` is called during boot inside `kernel_input_init()`, but `usb_tablet_probe()` is never called.
- **HIDA Connection**: Yes, `usb_tablet_report_event()` directly calls `hida_push_absolute(HIDA_BACKEND_USB, ...)`. However, because the USB stack doesn't exist, this function is never fed actual data.

### 3. `kernel/drivers/input/core/hida.c` (and `.h`)
- **What it actually implements**: Defines the `HIDA_BACKEND_USB` identifier (`121`) for the arbitration logic.
- **Completeness**: Complete for arbitration purposes.
- **Boot Initialization**: Yes, `hida_init()` runs during boot.
- **HIDA Connection**: This *is* the HIDA subsystem. It is aware of USB as a potential authority but currently receives no USB data.

---

## Smallest Realistic Roadmap for USB HID Mouse (QEMU Target)

To achieve a working standard USB HID mouse in QEMU, the smallest realistic path is to implement a minimal **UHCI** (simplest legacy default) or **xHCI** (modern standard) stack. Given the prompt's request for "modern USB mouse support," targeting **xHCI** via QEMU's `qemu-xhci` device is recommended.

**Phase 1: USB Host Controller (xHCI)**
1. **PCI Enumeration**: Scan PCI for Class `0x0C`, Subclass `0x03`, Prog IF `0x30` (xHCI).
2. **xHCI Initialization**: 
   - Read capabilities/operational registers.
   - Setup the Device Context Base Address Array (DCBAA).
   - Setup the Command Ring and Event Ring.
   - Start the controller.

**Phase 2: USB Core (Root Hub & Enumeration)**
1. **Port Polling**: Detect port connection status changes.
2. **Device Reset**: Issue a port reset to transition the device to the default state.
3. **Address Assignment**: Send an `Enable Slot` command, followed by an `Address Device` command to transition the device to the Addressed state.
4. **Endpoint 0 Setup**: Read the initial Device Descriptor to determine `MaxPacketSize0`.

**Phase 3: USB HID Class Driver**
1. **Read Configuration Descriptor**: Perform a Control Transfer to get the full configuration (including Interface and Endpoint descriptors).
2. **Set Configuration**: Issue a Control Transfer to select Configuration 1.
3. **HID Specifics**: 
   - Issue `Set Protocol` (value = 0 for Boot Protocol) to bypass complex Report Descriptor parsing. Boot Protocol guarantees a standard 3-byte or 4-byte mouse packet.
   - Issue `Set Idle` (value = 0) to ensure the device only reports upon state changes.

**Phase 4: USB HID Mouse Driver & Integration**
1. **Interrupt Transfer Setup**: Queue a TRB (Transfer Request Block) on the interrupt endpoint ring (Endpoint 1 IN) to listen for the 3/4-byte mouse reports.
2. **Polling / Event Loop**: Handle xHCI Event Ring interrupts to process completed transfers.
3. **HIDA Integration**: Decode the Boot Protocol packet (Buttons, dX, dY). Feed this data directly to `hida_push_relative(HIDA_BACKEND_USB, dx, dy, buttons, 0);`. Re-queue the TRB to listen for the next event.
