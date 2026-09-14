# Chapter 15: USB 3.0 xHCI & HID Subsystem

The **xHCI (eXtensible Host Controller Interface)** driver provides high-performance USB 3.0, 2.0, and 1.1 device support without relying on legacy BIOS emulation.

## 1. Core Structures
- **Device Context Base Address Array (DCBAA)**: Array of 64-bit physical pointers to Device Contexts for up to 256 devices.
- **Command Ring**: Circular ring of Transfer Request Blocks (TRBs) used by the kernel to issue commands (Enable Slot, Address Device, Configure Endpoint) to the controller.
- **Event Ring**: Ring populated by the controller to signal command completions and transfer events to the kernel.
- **Transfer Rings**: Per-endpoint circular rings used to schedule DMA transfers.

## 2. HID Protocol Implementation
- Automatically binds to USB Human Interface Devices (Subclass 1, Protocol 1 keyboard; Protocol 2 mouse).
- Configures device into **Boot Protocol 0** for deterministic packet layout.
- Schedules periodic Interrupt IN transfers on Endpoint 1.
- Unpacks mouse packets: button states (left, right, middle), signed delta X, signed delta Y.
- Clamps cursor coordinates strictly within display bounds (`0 <= x < screen_width`, `0 <= y < screen_height`).
