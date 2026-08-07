# SignaturesOS Production UEFI Bootloader Subsystem Architecture
## Subsystem Specification (`boot/uefi/BOOTX64.EFI`)

### 1. Architectural Overview & Design Goals
The **Production UEFI Bootloader Subsystem (`boot/uefi/`)** is a standalone, production-grade 64-bit EFI application that builds into **`EFI/BOOT/BOOTX64.EFI`**. It enables SignaturesOS to boot natively on modern UEFI platforms, virtual machines (QEMU, VirtualBox, VMware), and real physical PC hardware without requiring Legacy BIOS Compatibility Support Modules (CSM).

```
+-------------------------------------------------------------+
|              UEFI Firmware (x86_64 Long Mode)                |
+-------------------------------------------------------------+
                              │
                              v
+-------------------------------------------------------------+
|    Production UEFI Bootloader (EFI/BOOT/BOOTX64.EFI)        |
|    - 1. Locate EFI Graphics Output Protocol (GOP 32bpp)     |
|    - 2. Load \kernel.bin to Physical Address 0x100000       |
|    - 3. Convert UEFI Memory Descriptors to boot_info_t      |
|    - 4. Invoke ExitBootServices()                           |
|    - 5. Set RDI = boot_info_t* & Jump to _start (0x100000)   |
+-------------------------------------------------------------+
                              │
                              v
+-------------------------------------------------------------+
|            SignaturesOS Kernel Entry (_start)               |
|            - Zero BSS, Initialize C Kernel                  |
|            - BVMM Phase 1 - 12 Stack Active                 |
+-------------------------------------------------------------+
```

---

### 2. Zero-Kernel Modification Invariant
The UEFI Bootloader is engineered to be **100% transparent** to the SignaturesOS kernel:
- **Kernel Image**: `kernel.bin` remains identical whether booted via Legacy BIOS MBR or Modern UEFI GPT.
- **Register Interface**: Passes `boot_info_t*` in System V ABI register **`RDI`**.
- **Memory Layout**: Loads kernel binary to physical address **`0x100000` (1 MB)**.
- **Display Protocol**: Maps UEFI GOP framebuffer parameters (`HorizontalResolution`, `VerticalResolution`, `PixelsPerScanLine * 4`, `32bpp`, `FrameBufferBase`) 1-to-1 into `boot_info_t`'s `vbe_*` fields.

---

### 3. Build & Compilation
`BOOTX64.EFI` is compiled using `clang` targeting PE-COFF x86_64 Windows/EFI application subsystem:
```powershell
clang -target x86_64-unknown-windows -Wl,-subsystem:efi_application -Wl,-entry:efi_main -nostdlib -ffreestanding -I. boot\uefi\bootx64.c -o build\BOOTX64.EFI
```
