# Changelog

All notable changes to ATOMS OS will be documented in this file.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [v0.4.0-alpha.1] - 2026-08-08 - Codename: "Emerald Handoff"

### Added
- **First Verified Bare-Metal C-Kernel Execution**: Successfully booted ATOMS OS on physical Intel H81 motherboard hardware via UEFI GPT.
- **Isolated CP5A Visual Checkpoint System**: Implemented full-width 150px Bright Neon Green framebuffer bar (`0x0000FF00`) rendered directly from `kernel_main()` to visually certify C runtime entry.
- **Explicit Kernel Compilation Pipeline**: Updated `run_uefi_forensic_test.ps1` to compile `kernel/kernel.c` directly to `build/kernel.o` with stubbed cross-object references to prevent stale linking.
- **Hardware Forensic Documentation**: Added landmark milestone report at [`docs/milestones/first_real_hardware_c_kernel_execution.md`](file:///d:/Signatures_OS/docs/milestones/first_real_hardware_c_kernel_execution.md).

### Verified
- **UEFI Bootloader (`BOOTX64.EFI`)**: GOP initialization, FAT32 EFI partition parsing, `kernel.bin` RAM pool allocation, and `ExitBootServices()` execution on physical H81 firmware.
- **Assembly Handoff Trampoline (`kernel_entry.asm`)**: Stack alignment (`RSP = 0x90000`), SSE/SIMD enablement (`CR0`/`CR4`), and System V AMD64 ABI `call kernel_main` jump.
- **C Runtime Memory Access**: Direct linear framebuffer VRAM writes from `kernel_main()` verified via physical hardware monitor capture.
