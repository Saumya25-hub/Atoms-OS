# Changelog

All notable changes to ATOMS OS will be documented in this file.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [v2.7.0-vmx-stable] - 2026-09-30 - Codename: "Hypervisor & Silicon Bring-Up"

### Added
- **Intel VT-x Bare-Metal Type-1 Hypervisor Engine**:
  - Implemented 28-stage hardware virtualization pipeline (`kernel/core/hypervisor/`).
  - Added VMXON, VMCS lifecycle management, EPT (Extended Page Tables) 4-level paging, and VM-exit dispatching engine.
- **VirtIO Network Subsystem (`virtio-net` / `virtio-pci`)**:
  - Legacy PCI I/O-mapped VirtIO network device with dynamic MAC provisioning (`52:54:00:12:34:56`).
  - Configured RX/TX split virtqueues, descriptor rings, and netif bridging.
- **ACPI `_CRS` Producer Integration**:
  - Updated synthetic DSDT PCI0 host bridge descriptors (`s_dsdt_aml[]`) to set `ResourceProducer` (`0x0D`) flag, resolving guest PCI resource allocation.
- **Silicon Autopsy & Hypervisor Live Dashboard**:
  - Wired live VMCS VM-exit telemetry (`exit_reason`, `guest_rip`, `guest_gpa`) to on-screen diagnostic rendering.
  - Added real-time operator toggle (F5 / scancode `0x3F`) between silicon diagnostic dashboard and guest graphics framebuffer.
- **FreeBSD Payload Loader (`freebsd_loader.c`)**:
  - Embedded ELF payload mapping, runtime trampoline, and dynamic bootinfo environment configuration.
- **Hardware Network Drivers & Network Protocol Stack**:
  - Maintained Realtek RTL8168/RTL8125 and Intel E1000 drivers.
  - Complete verification of DHCP, DNS resolution, TCP 3-way handshake, and HTTP/1.1 payload streaming.

### Verified & Certified
- **Pre-Flight Certification**: 100% clean UEFI GPT boot, ABDE diagnostic rendering, rotating heartbeat spinner across 29M+ VM-exits, and zero regressions across all core subsystems (`CERTIFICATION_REPORT.md`).

---

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
