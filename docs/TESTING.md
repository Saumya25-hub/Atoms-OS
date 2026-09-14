# ATOMS OS — Testing & Hardware Certification Matrix

This document defines the testing methodology, environment specifications, and formal certification status for all subsystems of ATOMS OS.

---

## 1. Test Environments

### Environment A: Physical Bare-Metal Baseline (Intel Haswell H81)
- **Motherboard**: Intel H81 Express Chipset (LGA1150 Socket)
- **Firmware**: 2022 Updated BIOS, Native UEFI 2.3.1 (CSM Disabled)
- **CPU**: Intel Core i3-4130 / i3-4160 (Haswell x86_64, 2 Cores, 4 Threads, 3.40 GHz)
- **Memory**: 8 GB DDR3-1600 RAM (Single / Dual Channel)
- **Storage**: SATA SSD / USB 3.0 SanDisk Ultra Flash Drive
- **Network**: Realtek RTL8111G Gigabit Ethernet Controller
- **Audio**: Realtek ALC662 High Definition Audio Codec

### Environment B: Physical Modern Platform (Intel Raptor Lake-S Refresh)
- **Motherboard**: ASUS PRIME B760M-K (LGA1700 Socket)
- **Firmware**: AMI UEFI BIOS (Pure UEFI Mode)
- **CPU**: Intel Core i3-14100F (4 Performance Cores, 8 Threads)
- **Memory**: 16 GB DDR5 RAM
- **Storage**: Western Digital Blue SN5000 NVMe Gen4 SSD (PCIe 4.0 x4)
- **Graphics**: Discrete PCIe GPU / Framebuffer GOP

### Environment C: Virtualized UEFI Reference (QEMU x86_64)
- **Hypervisor**: QEMU 7.x / 8.x / 9.x (`qemu-system-x86_64`)
- **Firmware**: Pure 64-bit TianoCore EDK2 OVMF (`edk2-x86_64-code.fd`)
- **Configuration**: 2048 MB RAM, `-device qemu-xhci`, `-device usb-kbd`, `-device usb-mouse`, `-serial file:...`
- **Purpose**: Rapid automated pre-flight testing and regression prevention before bare-metal USB flashing.

---

## 2. Comprehensive Subsystem Test Matrix

| Subsystem | QEMU UEFI | Physical H81 | Physical B760M-K | Status | Notes & Evidence |
| :--- | :---: | :---: | :---: | :---: | :--- |
| **UEFI Bootloader (`BOOTX64.EFI`)** | **PASS** | **PASS** | **PASS** | **CERTIFIED** | GOP discovery, memory map acquisition, and `ExitBootServices()` retry loops verified. |
| **Trampoline & Kernel Entry** | **PASS** | **PASS** | **PASS** | **CERTIFIED** | 256 KB stack setup, ABI parameter handoff, and segment reload verified. |
| **CPU Long Mode & GDT/TSS** | **PASS** | **PASS** | **PASS** | **CERTIFIED** | 64-bit Flat GDT, Dual TSS descriptors, and `RSP0` kernel stack switching verified. |
| **Interrupts & IDT / APIC** | **PASS** | **PASS** | **PASS** | **CERTIFIED** | 256-entry IDT, IST exception stacks, 8259 PIC masking, and Local APIC timer verified. |
| **Physical Memory Manager (PMM)** | **PASS** | **PASS** | **PASS** | **CERTIFIED** | Bitmap allocator managing up to 32 GB RAM; contiguous page reservation verified. |
| **Virtual Memory Manager (VMM)** | **PASS** | **PASS** | **PASS** | **CERTIFIED** | 4-level PML4 paging, recursive page table mapping, and user-space page attributes verified. |
| **Kernel Heap Allocator** | **PASS** | **PASS** | **PASS** | **CERTIFIED** | Block-based dynamic kernel heap allocator with boundary tag coalescing verified. |
| **SMP (Symmetric Multiprocessing)** | **PASS** | **PASS** | **PARTIAL** | **CERTIFIED** | MADT table parsing, INIT-SIPI-SIPI AP wake sequence verified on Haswell dual-core. |
| **USB 3.0 Controller (xHCI)** | **PASS** | **PASS** | **PASS** | **CERTIFIED** | Command Ring, Event Ring, Transfer Rings, Slot Context, and Endpoint 0 enumeration verified. |
| **USB HID (Mouse & Keyboard)** | **PASS** | **PASS** | **PASS** | **CERTIFIED** | Boot Protocol 0, interrupt IN polling, packet unpacking, and cursor clamping verified. |
| **Storage: NVMe Gen4 Controller** | **PASS** | **N/A** | **PASS** | **CERTIFIED** | Admin Queue, I/O Submission/Completion queues, 4KB block read/write verified on SN5000 SSD. |
| **Storage: AHCI SATA Controller** | **PASS** | **PASS** | **PASS** | **CERTIFIED** | FIS command construction, PRDT setup, and direct sector reads verified on physical SATA. |
| **Filesystem: BOFS (Native)** | **PASS** | **PASS** | **PASS** | **CERTIFIED** | 4KB block allocation, inode extent trees, Write-Ahead Logging (WAL), and VFS mounting verified. |
| **Filesystem: FAT32 (ESP)** | **PASS** | **PASS** | **PASS** | **CERTIFIED** | BPB parsing, cluster chain traversal, file read/write inside EFI System Partition verified. |
| **Filesystem: NTFS (Read-Only)** | **PASS** | **PASS** | **PASS** | **CERTIFIED** | MFT parsing, `$FILE_NAME` attribute indexing, and Windows XP / 7 / 10 volume read verified. |
| **Graphics: GOP & DGL / BSPE** | **PASS** | **PASS** | **PASS** | **CERTIFIED** | 2560x1600 & 1920x1080 32-bpp double-buffered rendering, font blitting, and damage tracking verified. |
| **Compositor (BOS BCM)** | **PASS** | **PASS** | **PASS** | **CERTIFIED** | Multi-surface blitting, alpha-blending, dirty rect bounding, and wallpaper layering verified. |
| **Desktop Shell & Rook Login** | **PASS** | **PASS** | **PASS** | **STABLE PREVIEW**| Interactive login supervisor loop, wallpaper transition, and taskbar presentation verified. |
| **Fast Syscalls (`IA32_LSTAR`)** | **PASS** | **PASS** | **PASS** | **STABLE** | Hardware MSR gateway, user/kernel register preservation, and syscall dispatch verified. |
| **Ring 3 Userspace Runtime** | **PASS** | **PARTIAL** | **PARTIAL** | **IN DEVELOPMENT**| Basic ELF process loading and CR3 switching active; IPC and process destruction under test. |
| **Audio (AC97 / Intel HDA)** | **PASS** | **PASS** | **N/A** | **STABLE** | DMA circular buffer playback, mixer control, and WAV PCM streaming verified on ALC662. |
| **Networking (RTL8168 / TCP/IP)** | **PASS** | **PASS** | **N/A** | **STABLE** | PCI descriptor ring RX/TX, ARP resolution, ICMP ping reply, and basic TCP sockets verified. |
| **DOOM Native Port** | **PASS** | **PASS** | **PASS** | **CERTIFIED** | Embedded userspace game binary running with software rasterization and keyboard input. |
| **ATRIX Browser Engine Probe** | **PASS** | **EXPERIMENTAL**| **EXPERIMENTAL**| **EXPERIMENTAL** | GN/Ninja Blink/V8 host/guest toolchain prototype; active ongoing development. |

---

## 3. Mandatory Pre-Flight Verification Protocol

Before any new build is flashed to a physical USB drive for bare-metal testing on the H81 motherboard, it must satisfy the **6-Step Pre-Flight Protocol**:

1. **Clean Build**: `.\build.ps1` executes with return code 0 and zero compilation/link warnings treated as fatal.
2. **GPT Image Build**: `build\gpt_image_builder.exe` generates `build\atoms_uefi_test.img` cleanly.
3. **Pure UEFI QEMU Execution**: Boots without crash under EDK2 OVMF.
4. **ABDE Rendering Verification**: ABDE diagnostic table renders cleanly on screen without visual corruption.
5. **Heartbeat Spinner Verification**: Hardware-timed heartbeat spinner rotates continuously (`| / - \`).
6. **Zero Regression**: Serial telemetry confirms all previously certified checkpoints (CP0 through CP24) pass.
