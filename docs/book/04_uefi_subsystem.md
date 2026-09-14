# Chapter 04: Pure UEFI Bootloader Subsystem

The ATOMS OS bootloader is implemented in [`boot/uefi/bootx64.c`](file:///D:/Signatures_OS/boot/uefi/bootx64.c) and compiles into **`EFI/BOOT/BOOTX64.EFI`**.

## 1. Design Principles
- **Zero External Dependencies**: Implements its own minimal EFI runtime headers without relying on GNU-EFI or EDK2 build harnesses.
- **Pure 64-bit PE32+**: Compiled directly via Clang:
  ```powershell
  clang -target x86_64-unknown-windows -Wl,-subsystem:efi_application -Wl,-entry:efi_main -nostdlib -ffreestanding -I. boot/uefi/bootx64.c -o build/BOOTX64.EFI
  ```
- **CSM Independence**: Boots natively on modern motherboards with legacy BIOS Compatibility Support Modules permanently disabled.

## 2. GOP Framebuffer Negotiation
The bootloader locates the `EFI_GRAPHICS_OUTPUT_PROTOCOL` (GOP) GUID, iterates through available video modes, and prioritizes native display resolution (2560x1600 or 1920x1080 at 32-bpp BGRA).

## 3. ExitBootServices Retry Loop
Firmware memory allocation timers frequently invalidate the UEFI memory map between `GetMemoryMap()` and `ExitBootServices()`. The bootloader executes a robust retry loop:
```c
EFI_STATUS status = uefi_call(gBS->GetMemoryMap, ...);
status = uefi_call(gBS->ExitBootServices, image_handle, map_key);
if (EFI_ERROR(status)) {
    // Re-acquire memory map key and retry
    uefi_call(gBS->GetMemoryMap, ...);
    uefi_call(gBS->ExitBootServices, image_handle, map_key);
}
```
Once `ExitBootServices` returns `EFI_SUCCESS`, firmware interrupts are disabled, firmware runtime services are abandoned, and full hardware control is transferred to the BOS Kernel.
