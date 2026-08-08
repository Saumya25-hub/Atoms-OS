# ATOMS OS — Bare-Metal x86_64 Operating System

![Bare-Metal Certified](https://img.shields.io/badge/Bare--Metal_H81-VERIFIED-00FF00?style=for-the-badge&logo=hardware)
![UEFI GPT](https://img.shields.io/badge/Boot-UEFI_GPT-blue?style=for-the-badge)
![Status](https://img.shields.io/badge/Status-v0.4.0--alpha.1_Emerald_Handoff-brightgreen?style=for-the-badge)

```text
       █████╗ ████████╗██████╗ ███╗   ███╗███████╗     ██████╗ ███╗   ██╗
      ██╔══██╗╚══██╔══╝██╔═══██╗████╗ ████║██╔════╝    ██╔═══██╗████╗  ██║
      ███████║   ██║   ██║   ██║██╔████╔██║███████╗    ██║   ██║██╔██╗ ██║
      ██╔══██║   ██║   ██║   ██║██║╚██╔╝██║╚════██║    ██║   ██║██║╚██╗██║
      ██║  ██║   ██║   ╚██████╔╝██║ ╚═╝ ██║███████║    ╚██████╔╝██║ ╚████║
      ╚═╝  ╚═╝   ╚═╝    ╚═════╝ ╚═╝     ╚═╝╚══════╝     ╚═════╝ ╚═╝  ╚═══╝
```

---

## 🏆 Major Milestone: First Verified Bare-Metal C-Kernel Execution

**Release Version:** `v0.4.0-alpha.1`  
**Codename:** `Emerald Handoff`  
**Certification Date:** August 8, 2026  
**Verified Hardware:** Physical Intel H81 Motherboard (x86_64 Bare-Metal, UEFI 2.x, GOP Graphics)  

Photographically verified on real Intel H81 bare-metal hardware:
- ✅ **UEFI Bootloader (`BOOTX64.EFI`)**: GOP graphics initialized, FAT32 EFI partition parsed, `ExitBootServices()` completed.
- ✅ **Assembly Trampoline (`kernel_entry.asm`)**: Stack set (`RSP = 0x90000`), 16-byte aligned, SSE (`CR0`/`CR4`) enabled.
- ✅ **C-Kernel Execution (`kernel_main()`)**: System V AMD64 ABI transition certified, direct VRAM memory writes confirmed.
- 🟩 **Visual Proof (CP5A)**: **Full-width 150px Bright Neon Green Bar** rendered directly from C code on bare-metal hardware.

Detailed Milestone Report: [`docs/milestones/first_real_hardware_c_kernel_execution.md`](file:///d:/Signatures_OS/docs/milestones/first_real_hardware_c_kernel_execution.md)

---

## 🔬 Hardware Verification Matrix

| Layer | Component | Target | Bare-Metal Status |
| :--- | :--- | :--- | :--- |
| **Boot** | UEFI 2.x / GPT | `BOOTX64.EFI` | **PASS (Intel H81)** |
| **GOP** | Linear Framebuffer | 2560x1600 / 1920x1080 | **PASS (VRAM Mapped)** |
| **ExitBootServices** | Memory Map Handoff | MapKey Sync | **PASS** |
| **Assembly Entry** | `kernel_entry.asm` | `_start` @ `0x100000` | **PASS** |
| **Stack & SIMD** | System V AMD64 ABI | `RSP = 0x90000`, SSE | **PASS** |
| **C Kernel Entry** | `kernel_main()` | `call kernel_main` | **PASS (CP5A Green Bar)** |

---

## 📁 Repository Documentation

- 📄 **[Changelog](file:///d:/Signatures_OS/CHANGELOG.md)** — Project version history and recent release notes.
- 📄 **[Hardware Milestone Report](file:///d:/Signatures_OS/docs/milestones/first_real_hardware_c_kernel_execution.md)** — Detailed technical forensic write-up of bare-metal validation.
- 📄 **[Walkthrough Report](file:///C:/Users/Saumya%20Chaudhari/.gemini/antigravity-ide/brain/0d55c911-de87-463b-b203-b27ebd7560fc/walkthrough.md)** — Hardware binary-search diagnostic walkthrough.

---

## 💾 Storage Subsystem & NTFS Certification (v0.9.8)

**NTFS Read-Only Production Certification**: Validated against genuine Microsoft Windows XP created NTFS volumes with real on-disk metadata, resident/non-resident files, multi-extent runlists, and fragmented files (up to 73 extents / 200MB).

---

## 🚀 Quick Start & Build Verification

```powershell
# Build UEFI GPT Image and Run QEMU Verification
powershell -ExecutionPolicy Bypass -File .\run_uefi_forensic_test.ps1
```

Generated Image: `build/atoms_uefi_test.img` (Flash to USB via Rufus for bare-metal testing).
