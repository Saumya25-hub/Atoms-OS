# ATOMS OS — START HERE

Welcome to the **ATOMS OS** repository. This document is the primary technical entry point for developers, researchers, and systems engineers exploring the project.

---

## 1. What is ATOMS OS?

**ATOMS OS** is an independently developed, 64-bit operating system engineered from bare metal.

> [!IMPORTANT]
> **Independent Identity**: ATOMS OS is **NOT** a Linux distribution, Linux derivative, Unix, BSD, or Windows subsystem. It boots independently from its own 64-bit UEFI bootloader (`BOOTX64.EFI`) directly into its native **BOS Kernel**. Comparisons with other operating systems are made only when technically necessary.

### Architectural Pillars
- **Kernel**: **BOS Kernel** — Monolithic 64-bit Long Mode kernel.
- **Native Filesystem**: **BOFS** (BOS Filesystem) — Custom transactional, extent-based filesystem with Write-Ahead Logging (WAL) and POSIX-compliant VFS abstraction.
- **Shared Library Format**: **`.sll`** (Signatures Shared Library) dynamic runtime libraries.
- **Executable Binaries**: Native **BOSX** (BOS Executable) and standard 64-bit **ELF** binaries.
- **Privilege Separation**: Ring 0 kernel supervisor, Ring 3 userspace process isolation via `IA32_LSTAR` hardware fast-syscall gateway.
- **Desktop Subsystem**: Custom double-buffered hardware compositor (**BSPE** / **BCM**), **Rook** login supervisor, and native desktop shell.

---

## 2. Current Project Status

ATOMS OS is developed by a solo developer (**Saumya Chaudhari**) utilizing rigorous **AI-assisted systems engineering**.

We maintain absolute factual honesty regarding project maturity:

| Subsystem | Development Stage | Status & Certification |
| :--- | :--- | :--- |
| **UEFI Bootloader (`BOOTX64.EFI`)** | Core Infrastructure | **STABLE** — Certified on physical Haswell H81 & ASUS B760M-K UEFI firmware. |
| **Physical Memory Manager (PMM)** | Core Kernel (Ring 0) | **STABLE** — 32 GB Bitmap physical page allocator certified. |
| **Virtual Memory Manager (VMM)** | Core Kernel (Ring 0) | **STABLE** — 4-level PML4 paging, recursive mapping, user/kernel split certified. |
| **Interrupts & GDT/TSS** | Core Kernel (Ring 0) | **STABLE** — Dual TSS 64-bit TSS, IST stacks, APIC/PIC certified. |
| **SMP (Multiprocessing)** | Core Kernel (Ring 0) | **STABLE** — Multi-core AP discovery, INIT-SIPI-SIPI boot sequence verified. |
| **Hardware Drivers (xHCI / NVMe / AHCI)** | Core Drivers (Ring 0) | **STABLE** — USB 3.0 xHCI controller, HID mouse/keyboard, NVMe Gen4, AHCI SATA verified. |
| **VFS & Filesystems (BOFS / FAT32 / NTFS)** | Filesystem Layer | **STABLE (RO/Basic RW)** — BOFS transactional WAL certified; FAT32 ESP read/write certified; NTFS read-only parsing certified. |
| **Syscall Gateway (`IA32_LSTAR`)** | Kernel-to-User Bridge | **STABLE** — Fast syscall dispatcher (`SYS_EXIT`, `SYS_WRITE`, `SYS_MMAP`, `SYS_MPROTECT`, windowing syscalls). |
| **Desktop Shell & Compositor (BWE / BCM)** | Graphics Subsystem | **STABLE PREVIEW** — Double-buffered 2560x1600 hardware compositor, Rook login loop, wallpaper rendering active. |
| **Ring 3 Userspace Processes** | Userspace Foundation | **IN ACTIVE DEVELOPMENT** — Basic ELF loading and userspace page mapping operational; multi-process scheduler, dynamic `.sll` linker, and process teardown under refinement. |
| **ATRIX Browser Engine Probe** | Web Runtime Subsystem | **EXPERIMENTAL** — GN/Ninja toolchain integration, Blink DOM and V8 bindings compiled as host/guest prototypes. |

---

## 3. Quick Start: Building & Booting

### Prerequisites
- **Host OS**: Windows 10/11 (PowerShell 5.1+ or 7+).
- **Compiler Toolchain**: LLVM/Clang 16+ (`clang`, `clang++`, `lld-link` or `ld.lld`, `llvm-mc`).
- **Assembler**: NASM x86_64.
- **Virtual Machine**: QEMU x86_64 with OVMF UEFI firmware (`edk2-x86_64-code.fd`).
- **Disk Tools**: Built-in `build\gpt_image_builder.exe` (compiles from `tools/gpt_image_builder.c`).

### Build the Full Operating System
From the repository root, run the primary build script:
```powershell
.\build.ps1
```
This single build command:
1. Compiles the BOS Kernel and all Ring 0 drivers into `build/kernel.bin`.
2. Compiles the standalone UEFI bootloader into `build/BOOTX64.EFI`.
3. Compiles the Ring 3 desktop shell and userspace applications (`build/*.elf`).
4. Packages the FAT32/VDI/VMDK virtual hard disks (`build/OS.img`, `build/SignaturesOS.vdi`, `build/SignaturesOS.vmdk`).

### Generate the Bootable UEFI GPT Disk Image
```powershell
.\build\gpt_image_builder.exe build\BOOTX64.EFI build\kernel.bin build\atoms_uefi_test.img
```
This produces a 512 MB GPT raw disk image (`build/atoms_uefi_test.img`) formatted with a FAT32 EFI System Partition containing `/EFI/BOOT/BOOTX64.EFI`, `KERNEL.BIN`, and bundled test media.

### Boot in Pure UEFI QEMU
Run the automated test runner:
```powershell
.\tools\qemu\run_qemu_test.ps1
```
Or run QEMU directly:
```powershell
qemu-system-x86_64 -drive if=pflash,format=raw,readonly=on,file=D:\OS-QEMU-EMU\qemu\share\edk2-x86_64-code.fd `
                   -drive file=build\atoms_uefi_test.img,format=raw `
                   -device qemu-xhci -device usb-kbd -device usb-mouse `
                   -serial file:build\uefi_forensic_serial.log `
                   -m 2048M -display none -no-reboot
```

---

## 4. Documentation Architecture

All technical and architectural documentation is categorized hierarchically inside [`docs/`](file:///D:/Signatures_OS/docs):

- **Core Documentation**:
  - [`docs/START_HERE.md`](file:///D:/Signatures_OS/docs/START_HERE.md): You are here.
  - [`docs/AI_ASSISTED_DEVELOPMENT.md`](file:///D:/Signatures_OS/docs/AI_ASSISTED_DEVELOPMENT.md): Engineering manifesto, human review protocols, and "vibe coding" technical rebuttal.
  - [`docs/TESTING.md`](file:///D:/Signatures_OS/docs/TESTING.md): Hardware and QEMU test matrix, validation baselines, and test logs.
  - [`docs/KNOWN_ISSUES.md`](file:///D:/Signatures_OS/docs/KNOWN_ISSUES.md): Active bug tracking for Ring 3 userspace, syscall edge cases, and runtime integration.
  - [`docs/MAINTENANCE.md`](file:///D:/Signatures_OS/docs/MAINTENANCE.md): Repository maintenance policies, directory standards, and PR workflows.
- **The Book of ATOMS OS**:
  - [`docs/book/ATOMS_OS_BOOK.md`](file:///D:/Signatures_OS/docs/book/ATOMS_OS_BOOK.md): Master table of contents and progressive technical book covering CPU architecture, memory management, drivers, filesystems, graphics, and userspace.
- **Subsystem Architecture**:
  - [`docs/architecture/`](file:///D:/Signatures_OS/docs/architecture): Detailed architectural blueprints (Privilege Rings, Syscalls, Compositor, VFS, Memory Layout).
  - [`docs/certifications/`](file:///D:/Signatures_OS/docs/certifications): Formal hardware and emulator milestone certification reports.
  - [`docs/milestones/`](file:///D:/Signatures_OS/docs/milestones): Phase deliverable reports and historical engineering milestones.
  - [`docs/third_party/`](file:///D:/Signatures_OS/docs/third_party): Third-party provenance manifests (Chromium, Skia, V8, musl, FFmpeg licenses).
  - [`docs/history/`](file:///D:/Signatures_OS/docs/history): Forensic investigation postmortems, patch plans, and debugging autopsies.

---

## 5. Physical Bare-Metal Verification

ATOMS OS is developed with real hardware bring-up as the primary standard of truth:
- **Baseline Hardware Target**: Intel Haswell LGA1150 Platform (H81 Express Chipset, Core i3-4130/4160, 8 GB DDR3 RAM, 2022 Native UEFI BIOS).
- **Secondary Target**: Intel Raptor Lake-S Refresh Platform (ASUS B760M-K, Core i3-14100F, 16 GB DDR5 RAM, Western Digital Blue SN5000 NVMe Gen4 SSD).
- **Flashing Instructions**:
  Write `build/atoms_uefi_test.img` directly to a USB drive using raw block write tools:
  ```powershell
  # Using dd on host (replace PhysicalDriveN with target USB drive number)
  dd if=build/atoms_uefi_test.img of=\\.\PhysicalDriveN bs=1M
  ```

---

## 6. AI-Assisted Engineering Disclosure

ATOMS OS is built by a solo developer with AI-assisted software engineering. 

We do not use generic AI buzzwords or claim magical autonomy. Operating systems cannot be "vibe-coded": hardware registers, paging data structures, and PCI bus protocols fail catastrophically if code contains hallucinations.

For full disclosure of our four-stage verification protocol (**Investigate $\to$ Plan $\to$ Patch $\to$ Certify**), read [`docs/AI_ASSISTED_DEVELOPMENT.md`](file:///D:/Signatures_OS/docs/AI_ASSISTED_DEVELOPMENT.md).
