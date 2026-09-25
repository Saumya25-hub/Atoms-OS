# ATOMS OS — Technical Architecture & Forensic Documentation

[![Official Info Web](https://img.shields.io/badge/Official%20Website-atoms--os--infoweb.vercel.app-00f0ff?style=for-the-badge&logo=vercel)](https://atoms-os-infoweb.vercel.app/)
[![Creator](https://img.shields.io/badge/Author-Saumya%20Chaudhari-10b981?style=for-the-badge&logo=github)](https://github.com/Saumya25-hub)
[![Reddit](https://img.shields.io/badge/Reddit-u%2FSaumya--25-ff4500?style=for-the-badge&logo=reddit)](https://www.reddit.com/user/Saumya-25/)

> 🌐 **Official Web Portal & Live Telemetry**: **[https://atoms-os-infoweb.vercel.app/](https://atoms-os-infoweb.vercel.app/)**  
> *(Interactive ABDE diagnostic terminal, 8 architectural tiers, 14 hardware drivers, native `.sll` runtime catalog, and physical bare-metal monitor photos)*
>
> **Creator & Solo Systems Architect**: **Saumya Chaudhari** ([@Saumya25-hub](https://github.com/Saumya25-hub) / [u/Saumya-25](https://www.reddit.com/user/Saumya-25/))  
> **Operating System Architecture**: Independent 64-bit Operating System Architecture  
> **Core Kernel**: BOS Kernel (Native 64-bit Long Mode Microkernel)  
> **Native Shared Library Format**: `.sll` (Shared Link Library)  
> **Target Execution Model**: Pure UEFI 2.x 64-bit Long Mode (No CSM / No BIOS Legacy Mode)  
> **Repository Baseline**: Verified on Bare-Metal Hardware & Pure UEFI Pre-Flight Validation  

---

## 1. Project Identity

**ATOMS OS** is an independent operating-system project built from bare metal. It is **not** a distribution of Linux, **not** a derivative of Windows, and **not** based on Unix or BSD codebases. 

ATOMS OS implements its own standalone system architecture:
- **BOS Kernel**: An independent 64-bit kernel written in C and x86_64 assembly.
- **Process & Task Infrastructure**: Custom task management, kernel task stacks, and process image loaders (`BOSX` / native ELF / `.sll` shared libraries).
- **Two-Tier Memory Architecture**: Physical Memory Manager (PMM) frame allocator and 4-level Virtual Memory Manager (VMM) with dynamic address space lifecycle management.
- **Hardware Syscall Gateway**: `IA32_LSTAR` MSR-driven fast syscall entry with user-space memory boundary and page-table validation.
- **Device & Input Architecture**: Native xHCI USB 3.0 host controller, USB HID keyboard/mouse stack, and legacy PS/2 fallback drivers.
- **Graphics Pipeline**: Platform-neutral UEFI Graphics Output Protocol (GOP) linear framebuffer abstraction, Display Governance Layer (DGL), BSPE Display HAL, and BOS Composition Manager (BCM).
- **Desktop Environment**: Custom desktop shell, taskbar, start menu, window manager surfaces, and font engine.
- **Filesystem & VFS**: Virtual File System abstraction layer with native FAT32 and NTFS read/write support.
- **Network Stack**: Native PCI network interface card (NIC) drivers for Intel E1000 and Realtek R8168/R8111, operating bare-metal UDP datagram telemetry, remote power management, and PXE deployment.
- **Hardware Virtualization & Type-1 Micro-Hypervisor**: Native Intel VT-x (VMX) and Extended Page Tables (EPT) engine running in Ring 0 VMX root operation on bare-metal silicon (Intel Core i3-14100F / LGA1700), featuring guest VMCS lifecycle management, SLAT paging, VirtIO device models, and full forensic diagnostics.
- **AI-(P)DEBUG Forensic Infrastructure**: Structured, evidence-based diagnostic framework operating over COM1 serial, full-screen Advanced Bare-Metal Diagnostic Engine (ABDE) dashboards, and cooperative UDP network telemetry.

---

---

## Canonical Documentation & Navigation

| Document | Purpose |
| :--- | :--- |
| [**`docs/START_HERE.md`**](file:///D:/Signatures_OS/docs/START_HERE.md) | **Primary Guide**: What ATOMS is, current status, build & boot guide, architecture index. |
| [**`docs/hypervisor/ATOMS_VMX_HARDWARE_CERTIFICATION.md`**](file:///D:/Signatures_OS/docs/hypervisor/ATOMS_VMX_HARDWARE_CERTIFICATION.md) | **Intel VT-x Hardware Hypervisor Proof**: Physical LGA1700 silicon certification, VM-entry/exit logs, and screenshot evidence. |
| [**`docs/AI_ASSISTED_DEVELOPMENT.md`**](file:///D:/Signatures_OS/docs/AI_ASSISTED_DEVELOPMENT.md) | **Engineering Manifesto**: Solo development workflow, human gatekeeping, and "vibe coding" technical rebuttal. |
| [**`docs/TESTING.md`**](file:///D:/Signatures_OS/docs/TESTING.md) | **Hardware & QEMU Test Matrix**: Bare-metal Haswell H81, ASUS B760M-K, and QEMU pre-flight verification. |
| [**`docs/KNOWN_ISSUES.md`**](file:///D:/Signatures_OS/docs/KNOWN_ISSUES.md) | **Active Debt & Bug Tracker**: Ring 3 userspace, syscall edge cases, dynamic `.sll` shared libraries. |
| [**`docs/MAINTENANCE.md`**](file:///D:/Signatures_OS/docs/MAINTENANCE.md) | **Repository Layout Standards**: Structural rules, directory boundaries, commit & release protocols. |
| [**`docs/book/ATOMS_OS_BOOK.md`**](file:///D:/Signatures_OS/docs/book/ATOMS_OS_BOOK.md) | **The Book of ATOMS OS**: 20 progressive technical chapters from CPU boot to desktop compositor. |

---

## 2. Current Project Status

The following matrix represents the verified status of each major subsystem based strictly on repository implementation, automated pre-flight testing, and documented bare-metal hardware certification:

| Subsystem | Status | Implementation Authority | Verified Evidence |
| :--- | :---: | :--- | :--- |
| **UEFI Boot** | **CERTIFIED** | [`boot/uefi/bootx64.c`](file:///d:/Signatures_OS/boot/uefi/bootx64.c) | H81 & B750M-K Pure UEFI GPT Boot ([`CPU_ENGINE_CERTIFICATION.md`](file:///d:/Signatures_OS/docs/certifications/CPU_ENGINE_CERTIFICATION.md)) |
| **Kernel Entry** | **CERTIFIED** | [`kernel/kernel_entry.asm`](file:///d:/Signatures_OS/kernel/kernel_entry.asm) | 16KB `.bss` stack, CR0/CR4 SSE/FXSR, System V ABI handoff |
| **CPU Engine** | **CERTIFIED** | [`kernel/core/cpu/`](file:///d:/Signatures_OS/kernel/core/cpu/) | CPUID vendor, Leaf 1 features, SSE/AVX detection on H81 & B750M-K |
| **GDT Engine** | **CERTIFIED** | [`arch/x86_64/gdt/gdt.c`](file:///d:/Signatures_OS/arch/x86_64/gdt/gdt.c) | Per-CPU GDT arrays (`gdt_cpus[8][7]`), GDT descriptor loading |
| **SMP Bring-Up** | **CERTIFIED** | [`arch/x86_64/smp/smp.c`](file:///d:/Signatures_OS/arch/x86_64/smp/smp.c) | ACPI MADT parsing, AP INIT-SIPI-SIPI bring-up, AP heartbeat loops |
| **IDT / Exceptions** | **CERTIFIED** | [`kernel/core/interrupt/`](file:///d:/Signatures_OS/kernel/core/interrupt/) | 256-entry IDT, 32 CPU exception handlers, ISR dispatch |
| **PIC / APIC** | **CERTIFIED** | [`drivers/interrupt/pic/pic.h`](file:///d:/Signatures_OS/drivers/interrupt/pic/pic.h) | Legacy 8259A remap (0x20/0x28), Local APIC ICR/EOI operational |
| **PMM Allocator** | **CERTIFIED** | [`kernel/core/memory/pmm/`](file:///d:/Signatures_OS/kernel/core/memory/pmm/) | 32 GB frame ceiling, zero drift across 1,920 allocation cycles ([`PMM_FORENSIC_AUDIT.md`](file:///d:/Signatures_OS/docs/certifications/PMM_FORENSIC_AUDIT.md)) |
| **VMM Paging** | **CERTIFIED** | [`kernel/core/memory/vmm/`](file:///d:/Signatures_OS/kernel/core/memory/vmm/) | 4-level paging, CR3 safety, 100-cycle zero-drift process teardown ([`VMM_MEMORY_LIFECYCLE_CERTIFICATION.md`](file:///d:/Signatures_OS/docs/certifications/VMM_MEMORY_LIFECYCLE_CERTIFICATION.md)) |
| **Kernel Heap** | **CERTIFIED** | [`kernel/core/memory/heap/`](file:///d:/Signatures_OS/kernel/core/memory/heap/) | Stage A basic heap allocator verified on H81 bare metal ([`HEAP_HARDWARE_CERTIFICATION_H81.md`](file:///d:/Signatures_OS/docs/certifications/HEAP_HARDWARE_CERTIFICATION_H81.md)) |
| **Scheduler** | **STABLE (BSP)** | [`kernel/core/scheduler/`](file:///d:/Signatures_OS/kernel/core/scheduler/) | Preemptive/cooperative task queue on CPU 0; AP scheduling deferred |
| **TSS Architecture** | **CERTIFIED** | [`arch/x86_64/gdt/gdt.c`](file:///d:/Signatures_OS/arch/x86_64/gdt/gdt.c) | Dedicated per-CPU TSS (`tss_cpus[8]`), dynamic `RSP0` update ([`SYSCALL_TSS_SMP_FORENSIC_CERTIFICATION.md`](file:///d:/Signatures_OS/docs/certifications/SYSCALL_TSS_SMP_FORENSIC_CERTIFICATION.md)) |
| **Syscall Gateway** | **CERTIFIED** | [`kernel/core/syscall/`](file:///d:/Signatures_OS/kernel/core/syscall/) | `IA32_LSTAR` hardware entry, `SYSRETQ` atomic return, 30 services defined |
| **Syscall Security** | **CERTIFIED** | [`kernel/core/syscall/src/validation.c`](file:///d:/Signatures_OS/kernel/core/syscall/src/validation.c) | VMM PML4 page-table validation, 600-cycle stress pass ([`CERTIFICATION_REPORT.md`](docs/certifications/CERTIFICATION_REPORT.md)) |
| **USB xHCI Stack** | **CERTIFIED** | [`kernel/drivers/usb/host/xhci/`](file:///d:/Signatures_OS/kernel/drivers/usb/host/xhci/) | xHCI 1.0+ controller bring-up, 1024-TRB transfer/event rings ([`INPUT_POWER_CONTROLLER_FORENSIC_AUDIT.md`](file:///d:/Signatures_OS/docs/certifications/INPUT_POWER_CONTROLLER_FORENSIC_AUDIT.md)) |
| **USB HID Input** | **CERTIFIED** | [`kernel/drivers/usb/class/usb_hid.c`](file:///d:/Signatures_OS/kernel/drivers/usb/class/usb_hid.c) | Keyboard/mouse input, 200/200 ACK lock LED sync ([`USB_HID_KEYBOARD_LED_FORENSIC_AUDIT.md`](file:///d:/Signatures_OS/docs/certifications/USB_HID_KEYBOARD_LED_FORENSIC_AUDIT.md)) |
| **Graphics HAL** | **STABLE** | [`kernel/display/dgl/`](file:///d:/Signatures_OS/kernel/display/dgl/), [`kernel/graphics/BSPE/`](file:///d:/Signatures_OS/kernel/graphics/BSPE/) | 32-bit linear GOP framebuffer, DGL authority, dirty-rect damage tracker |
| **Compositor (BCM)** | **STABLE** | [`kernel/wm/bcm/`](file:///d:/Signatures_OS/kernel/wm/bcm/) | Window surfaces, cursor compositing plane, present queue |
| **Desktop Shell** | **STABLE** | [`kernel/shell/desktop_shell/`](file:///d:/Signatures_OS/kernel/shell/desktop_shell/) | Taskbar, Start Menu, background wallpaper service, window manager |
| **Filesystem / VFS** | **CERTIFIED** | [`kernel/vfs/vfs_legacy/`](file:///d:/Signatures_OS/kernel/vfs/vfs_legacy/) | FAT32/NTFS mount/unmount dynamic lifecycle, zero leak across 2,050 cycles ([`VFS_UNMOUNT_LIFECYCLE_CERTIFICATION.md`](file:///d:/Signatures_OS/docs/certifications/VFS_UNMOUNT_LIFECYCLE_CERTIFICATION.md)) |
| **Network Stack** | **PARTIAL** | [`kernel/net/`](file:///d:/Signatures_OS/kernel/net/), [`kernel/drivers/net/`](file:///d:/Signatures_OS/kernel/drivers/net/) | E1000 & R8168 PCIe drivers, ARP, IPv4, UDP telemetry/control ([`docs/LAN_CONTROL_SYSTEM.md`](file:///d:/Signatures_OS/docs/LAN_CONTROL_SYSTEM.md)) |
| **Diagnostics (ABDE)**| **CERTIFIED** | [`kernel/debug/abde/`](kernel/debug/abde/) | Real-time on-screen diagnostics, live rotating heartbeat spinner |
| **Screenshot Engine** | **CERTIFIED** | [`kernel/debug/screenshot/`](kernel/debug/screenshot/) | Cooperative non-blocking UDP fragmentation (4 chunks / 5.6KB per tick) |
| **NVMe Gen4 Storage**| **CERTIFIED** | [`kernel/drivers/storage/nvme/`](kernel/drivers/storage/nvme/) | ASUS B750M-K WD Blue SN5000 NVMe M.2 Gen4 + GPT + Windows 11 NTFS ([`ASUS_B750MK_NVME_GPT_NTFS_CERTIFICATION.md`](docs/certifications/ASUS_B750MK_NVME_GPT_NTFS_CERTIFICATION.md)) |

---

## 3. Architecture Overview

```text
                               +-------------------------------------------------+
                               |           UEFI 2.x Firmware (Pure UEFI)         |
                               +-------------------------------------------------+
                                                        |
                                                        v
                               +-------------------------------------------------+
                               |         BOOTX64.EFI (ATOMS Bootloader)          |
                               |  - GOP Framebuffer Negotiation                  |
                               |  - Memory Map Retrieval & Descriptor Sorting    |
                               |  - Silent ExitBootServices() Retry Loop         |
                               |  - Kernel Handoff (System V AMD64 ABI: RDI)     |
                               +-------------------------------------------------+
                                                        |
                                                        v
                               +-------------------------------------------------+
                               |     kernel_entry.asm (_start @ 0x100000)        |
                               |  - Zeroes .bss Section                          |
                               |  - Sets BSP Kernel Stack (16KB .bss Allocation) |
                               |  - Enables SSE / FXSR (CR0 / CR4)               |
                               +-------------------------------------------------+
                                                        |
                                                        v
                               +-------------------------------------------------+
                               |             BOS Kernel (kernel_main)            |
                               +-------------------------------------------------+
                                  |         |         |         |         |
      +---------------------------+         |         |         |         +---------------------------+
      v                                     v         |         v                                     v
+-----------------------+ +-------------------------+ | +-----------------------+ +-----------------------+
|    CPU / SMP Layer    | |    Memory Management    | | |    I/O & Input Stack  | | Graphics & Compositor |
| - GDT (gdt_cpus[8])   | | - PMM (Bitmap Allocator)| | | - xHCI Host Controller| | - GOP Linear VRAM HAL |
| - IDT (256 Descriptors| | - VMM (4-Level Paging)  | | | - USB HID Kbd & Mouse | | - DGL Authority       |
| - PIC Remap & LAPIC   | | - Address Space Reclaim | | | - Lock LED Control    | | - BCM Compositor Task |
| - AP INIT-SIPI Tramp  | | - Kernel Heap (Stage A) | | | - PS/2 Fallback       | | - Desktop Shell & BWE |
+-----------------------+ +-------------------------+ | +-----------------------+ +-----------------------+
                                                      |
                                    +-----------------+-----------------+
                                    v                                   v
                      +---------------------------+       +---------------------------+
                      |     Syscall Boundary      |       |  Diagnostics & Telemetry  |
                      | - IA32_LSTAR Hardware Trap|       | - ABDE Diagnostic Screen  |
                      | - Dedicated Task TSS.RSP0 |       | - Live Heartbeat Spinner  |
                      | - VMM Page-Table Check    |       | - AI-(P)DEBUG Engine      |
                      | - SYSRETQ Fast Return     |       | - Cooperative LAN Capture |
                      +---------------------------+       +---------------------------+
```

### Architectural Subsystem Responsibilities:
1. **Boot**: Negotiates display, extracts firmware memory descriptors, cleanly exits boot services, and transfers execution to the 64-bit kernel entry point.
2. **CPU / SMP**: Detects CPU topology via ACPI MADT, installs per-CPU GDTs and TSS descriptors, and boots secondary Application Processors (APs) into operational idle/heartbeat loops.
3. **Memory**: Manages physical memory frames via bitmap indexing; manages virtual memory via 4-level PML4 page tables with ownership-aware lifecycle reclamation.
4. **Syscalls**: Routes Ring 3 requests to Ring 0 handlers via `IA32_LSTAR`, enforcing strict memory bounds and page-table presence checks before dereferencing user pointers.
5. **Input**: Drives USB peripherals through an xHCI 1.0+ driver and HID parser, updating pointer state and dispatching hardware LED status packets.
6. **Graphics**: Controls pixel display through linear GOP framebuffers, maintaining window surfaces, damage rectangles, and compositing layers.
7. **Diagnostics**: Continuously audits kernel health, renders telemetry data, monitors CPU core heartbeats, and streams cooperative network diagnostic packets.

---

## 4. Boot Architecture

ATOMS OS boots exclusively via **UEFI 2.x 64-bit native firmware**. Legacy BIOS (MBR / CSM) boot is not used or supported.

```
UEFI Firmware ──> BOOTX64.EFI ──> ExitBootServices() ──> _start (kernel_entry.asm) ──> kernel_main()
```

### 1. UEFI Loader Implementation ([`boot/uefi/bootx64.c`](file:///d:/Signatures_OS/boot/uefi/bootx64.c))
- **GOP Video Negotiation**: Queries `EFI_GRAPHICS_OUTPUT_PROTOCOL`, evaluates all supported modes, and locks the highest 32-bit linear framebuffer resolution (`PixelBlueGreenRedReserved8BitPerColor` or `PixelRedGreenBlueReserved8BitPerColor`).
- **Memory Map Acquisition**: Queries `GetMemoryMap()`, translating firmware descriptors (`EfiConventionalMemory`, `EfiLoaderCode`, `EfiLoaderData`, `EfiACPIReclaimMemory`) into a standardized `boot_info_t` memory map containing up to 256 sorted regions.
- **Silent `ExitBootServices()` Handoff Loop**: Adheres to the UEFI specification by eliminating firmware console output (`uefi_print`) between `GetMemoryMap()` and `ExitBootServices()`. On real motherboards (such as Intel H81/B750), the first `ExitBootServices()` call signals events that may invalidate the firmware `MapKey`; the loader automatically catches `EFI_INVALID_PARAMETER`, refreshes the memory map, and executes the final successful handoff.
- **Legacy PIC Masking**: Issues `outb(0x21, 0xFF)` and `outb(0xA1, 0xFF)` to silence legacy 8259A interrupt controllers prior to kernel jump.
- **Kernel Transfer**: Positions kernel payload at physical address `0x100000` (1 MB boundary), sets stack pointer `RSP = 0x70000`, loads the `boot_info_t*` pointer into register `RDI`, and jumps to `_start`.

### 2. Assembly Kernel Entry ([`kernel/kernel_entry.asm`](file:///d:/Signatures_OS/kernel/kernel_entry.asm))
- **Kernel Boot Stack**: Allocates a dedicated 16KB stack in the `.bss` section (`boot_stack_bottom` to `boot_stack_top`) with 16-byte alignment. Low memory addresses (such as `0x90000`) are explicitly avoided because on physical UEFI platforms they intersect the Extended BIOS Data Area (EBDA) and SMM runtime memory.
- **BSS Initialization**: Identifies `_bss_start` and `_bss_end` symbols and clears the entire `.bss` section with `rep stosq` before calling any C code.
- **SIMD / SSE Initialization**: Enables SSE support by clearing `CR0.EM` (bit 2), setting `CR0.MP` (bit 1), and asserting `CR4.OSFXSR` (bit 9) and `CR4.OSXMMEXCPT` (bit 10).
- **Handoff**: Invokes `kernel_main(boot_info)` following the System V AMD64 ABI.

> **Hardware Reality**: ATOMS OS renders through the platform-provided UEFI GOP linear framebuffer. It does **not** load proprietary GPU kernel drivers (such as NVIDIA GeForce or AMD Radeon drivers); hardware GPUs present in testbeds operate strictly in standard linear GOP framebuffer mode.

---

## 5. CPU Architecture & Multi-Processing (SMP)

ATOMS OS operates natively in **64-bit Long Mode**.

### 1. Topology Discovery & AP Bring-Up ([`arch/x86_64/smp/smp.c`](file:///d:/Signatures_OS/arch/x86_64/smp/smp.c))
- **ACPI MADT Parsing**: Traverses the ACPI 2.0+ `RSDP` and `XSDT` tables to locate the Multiple APIC Description Table (`MADT` / signature `'APIC'`). Discovers the Local APIC Base Address (default `0xFEE00000`) and parses Processor Local APIC entries.
- **AP Trampoline (`0x8000`)**: Deploys real-mode startup code to physical page `0x8000`. When secondary cores receive SIPI vectors, they execute 16-bit real mode code, transition to 32-bit protected mode, load CR3 with the kernel PML4, enable paging, and enter 64-bit long mode.
- **IPI Protocol**: The Bootstrap Processor (CPU 0 / BSP) issues an `INIT` Inter-Processor Interrupt followed by two `Startup IPI` (`SIPI`) vectors targeting page `0x08`.
- **Per-CPU Architecture**:
  - Independent 16KB startup stacks per AP (`g_ap_stacks[8][16384]`).
  - Independent GDT arrays per core (`gdt_cpus[8][7]`).
  - Dedicated 64-bit Task State Segment (TSS) per core (`tss_cpus[8]`).

### 2. Multi-Processing Execution State
On physical hardware (such as the Intel Core i3-14100F with 8 logical execution units):
- **Cores Detected**: 8 Logical Cores enumerated via MADT.
- **Cores Online**: 8 Cores transitioned cleanly to 64-bit Long Mode.
- **AP Workload**: Secondary cores (APs 1..7) execute dedicated background diagnostic heartbeat tickers.
- **Scheduler Participation**: Multitasking and user process execution are currently **BSP-focused** (serialized on CPU 0). Cross-CPU parallel task scheduling across APs is deferred to future milestones.

> **Evidence Reference**: [`docs/certifications/SMP_ENGINE_CERTIFICATION.md`](file:///d:/Signatures_OS/docs/certifications/SMP_ENGINE_CERTIFICATION.md) & [`docs/certifications/SYSCALL_TSS_SMP_FORENSIC_CERTIFICATION.md`](file:///d:/Signatures_OS/docs/certifications/SYSCALL_TSS_SMP_FORENSIC_CERTIFICATION.md)

---

## 6. TSS & Syscall Hardware Architecture

System calls transition from user space (Ring 3) to kernel space (Ring 0) using hardware MSR extensions.

### 1. Hardware MSR Configuration ([`kernel/core/syscall/src/syscall.c`](file:///d:/Signatures_OS/kernel/core/syscall/src/syscall.c))
- **`IA32_EFER` (`0xC0000080`)**: Bit 0 (`SCE` — System Call Extensions) enabled via `wrmsr`.
- **`IA32_STAR` (`0xC0000081`)**: Configured with Kernel Segment Selectors (`0x08` code / `0x10` data) in bits 47:32 and User Segment Selectors (`0x18` data / `0x20` code) in bits 63:48.
- **`IA32_LSTAR` (`0xC0000082`)**: Programmed with the absolute 64-bit virtual entry point of `syscall_entry` ([`kernel/core/syscall/src/syscall_entry.asm`](file:///d:/Signatures_OS/kernel/core/syscall/src/syscall_entry.asm)).
- **`IA32_FMASK` (`0xC0000084`)**: Configured to mask CPU flags on entry (`IF`, `TF`, `DF`), ensuring interrupts are disabled immediately upon entering Ring 0.

### 2. Kernel Stack Management & TSS Isolation
- **Dedicated Task Kernel Stacks**: Every user task receives a dedicated 32KB (8-page) kernel stack allocated from PMM.
- **Dynamic `TSS.RSP0` Synchronization**: When the scheduler switches tasks on CPU 0, it dynamically rewrites `tss_cpus[0].rsp0`:
  ```c
  tss_set_kernel_stack((uint64_t)next_task->stack + next_task->stack_size);
  ```
- **Context Switch Safety**: During the syscall trap, `syscall_entry` saves user registers (`RCX`, `R11`, `RSP`, `RBP`, `RBX`, `R12..R15`), processes the dispatch in C, prepares return registers, and executes `SYSRETQ` to atomically restore user privilege.

> **Evidence Reference**: [`docs/certifications/SYSCALL_TSS_SMP_FORENSIC_CERTIFICATION.md`](file:///d:/Signatures_OS/docs/certifications/SYSCALL_TSS_SMP_FORENSIC_CERTIFICATION.md)

---

## 7. Memory Management Architecture

ATOMS OS uses a clean separation between physical allocation and virtual address space management.

```
Physical Memory (RAM) <──[PMM: Bitmap Allocator]──> Pages (4KB) <──[VMM: 4-Level Paging]──> Virtual Address Spaces
```

### Physical Memory Manager (PMM) ([`kernel/core/memory/pmm/`](file:///d:/Signatures_OS/kernel/core/memory/pmm/))
- **Data Structure**: Linear physical bitmap where each bit represents one 4096-byte (4KB) page frame.
- **Memory Ingestion**: Ingests UEFI memory descriptors at boot, identifying conventional memory vs reserved firmware ranges.
- **Low Memory Protection**: Permanently reserves physical memory from `0x0` to `0x200000` (low 2 MB), protecting real-mode IVT/BDA, AP startup trampolines (`0x8000`), and kernel image binaries.
- **Allocation Primitives**: Single-page allocation (`pmm_alloc_page`) and contiguous multi-page allocation (`pmm_alloc_pages`).
- **Safety Checks**: Implements alignment verification and double-free validation (panicking/logging if a freed page was already marked unallocated).
- **Physical Stress Evidence**: Verified on a physical 32 GB DDR5 test system (PMM frame ceiling: 8,388,608 frames). Tested across 1,920 allocation/free cycles ranging from 1 page (4KB) to 1,024 contiguous pages (4MB) with an exact **Net Page Delta of 0** (zero memory drift).

> **Evidence Reference**: [`docs/certifications/PMM_FORENSIC_AUDIT.md`](file:///d:/Signatures_OS/docs/certifications/PMM_FORENSIC_AUDIT.md)

### Virtual Memory Manager (VMM) ([`kernel/core/memory/vmm/`](file:///d:/Signatures_OS/kernel/core/memory/vmm/))
- **Structure**: Standard x86_64 4-level paging hierarchy:
  - Page Map Level 4 (PML4)
  - Page Directory Pointer Table (PDPT)
  - Page Directory (PD)
  - Page Table (PT)
- **User Address Space Partitioning**: User processes operate strictly within the window `[0x40000000, 0x80000000)` (1GB to 2GB virtual). Higher-half kernel PML4 entries (`256..511`) are mapped globally into every address space to maintain continuous kernel execution upon interrupt or syscall.
- **Resolved Historical Leak Milestone**: Prior to certification, process teardown only reclaimed the top-level PML4 frame, stranding intermediate tables (`PDPT`, `PD`, `PT`) and user memory in physical RAM (a leak of 16 pages per process cycle).
- **Hierarchical Ownership-Aware Teardown**: The VMM was refactored to recursively inspect process address spaces, distinguish process-owned tables from shared kernel structures, and reclaim all process page tables, user text/stack/heap physical pages, and 32KB kernel task stacks.
- **CR3 Safety Verification**: If a process being destroyed is currently active in `%cr3`, the VMM switches `%cr3` to `g_kernel_pml4` before reclaiming any frames.
- **Lifecycle Certification**: Validated on physical hardware across **100 continuous spawn-terminate-reap cycles** with **0 pages leaked** (baseline free pages: `7,639,611`; ending free pages: `7,639,611`).

> **Evidence Reference**: [`docs/certifications/VMM_MEMORY_LIFECYCLE_CERTIFICATION.md`](file:///d:/Signatures_OS/docs/certifications/VMM_MEMORY_LIFECYCLE_CERTIFICATION.md)

---

## 8. Process, Task & Scheduling Infrastructure

### 1. Task Representation ([`kernel/core/scheduler/include/task.h`](file:///d:/Signatures_OS/kernel/core/scheduler/include/task.h))
- **`Task` Structure**: Contains unique task ID (`id`), execution state (`TASK_READY`, `TASK_RUNNING`, `TASK_BLOCKED`, `TASK_TERMINATED`), CPU context (`registers_t`), priority level, time-slice quantum, dedicated kernel stack pointer (`stack`), and associated page directory pointer (`pml4`).
- **`ProcessImage`**: Manages user binary execution domains, mapping text segments, data segments, user stacks, and tracked heap allocations.

### 2. Context Switching & Scheduling ([`kernel/core/scheduler/src/scheduler.c`](file:///d:/Signatures_OS/kernel/core/scheduler/src/scheduler.c))
- **Preemptive Time-Slicing**: Driven by timer interrupts (PIT / LAPIC timer) invoking `scheduler_tick()`.
- **Cooperative Yielding**: Tasks invoke `sys_service_yield()` to voluntarily surrender their execution quantum.
- **Scheduler Queue**: Implements prioritized round-robin task queues.
- **CR3 Address Space Switching**: During context switches, if the outgoing task's `pml4` differs from the incoming task's `pml4`, `%cr3` is updated, and the CPU TLB is flushed.

---

## 9. Syscall ABI Specification

System calls are invoked using the `syscall` instruction. Parameters follow the System V AMD64 syscall calling convention:
- **Syscall ID**: Register `RAX`
- **Arguments 1–6**: Registers `RDI`, `RSI`, `RDX`, `R10`, `R8`, `R9`
- **Return Value**: Register `RAX` (`0` on success, negative error codes on failure)
- **Clobbered Registers**: `RCX` (user RIP) and `R11` (user RFLAGS)

### Authoritative Syscall Table ([`kernel/core/syscall/include/syscall.h`](file:///d:/Signatures_OS/kernel/core/syscall/include/syscall.h))

| ID | Name | Signature / Arguments | Purpose | Security Validation | Status |
| :---: | :--- | :--- | :--- | :--- | :---: |
| `0` | `SYS_WRITE` | `(const char *buf, size_t len)` | Writes buffer to display/COM1 | User range + read-perm check | **CERTIFIED** |
| `1` | `SYS_EXIT` | `(int exit_code)` | Terminates active task | None (Value argument) | **CERTIFIED** |
| `2` | `SYS_GETPID` | `(void)` | Returns active process/task ID | None | **CERTIFIED** |
| `3` | `SYS_YIELD` | `(void)` | Surrenders remaining CPU quantum | None | **CERTIFIED** |
| `4` | `SYS_UPTIME` | `(void)` | Returns system uptime in timer ticks | None | **CERTIFIED** |
| `5` | `SYS_ALLOC` | `(size_t size)` | Allocates user heap memory | Size limit validation | **CERTIFIED** |
| `6` | `SYS_FREE` | `(void *ptr)` | Frees allocated user memory | User bounds check | **CERTIFIED** |
| `7` | `SYS_DEBUG_PRINT`| `(const char *msg)` | Emits string to debug COM1 log | Page-boundary safe string scan | **CERTIFIED** |
| `8` | `SYS_MMAP` | `(void *addr, size_t len, int prot, ...)`| Maps virtual memory pages | Alignment & range checks | **STABLE** |
| `9` | `SYS_MUNMAP` | `(void *addr, size_t len)` | Unmaps virtual memory pages | Address space bounds check | **STABLE** |
| `10`| `SYS_MPROTECT` | `(void *addr, size_t len, int prot)` | Modifies page permissions | Page table access check | **STABLE** |
| `11`| `SYS_FUTEX` | `(uint32_t *uaddr, int op, uint32_t val, ...)` | Fast user-space synchronization | User pointer read check | **STABLE** |
| `12`| `SYS_CLOCK_GETTIME`| `(int clk_id, struct timespec *tp)` | Returns precision timestamps | Writable user pointer check | **CERTIFIED** |
| `13`| `SYS_NANOSLEEP` | `(const struct timespec *req, ...)` | Suspends task for interval | User pointer read check | **STABLE** |
| `14`| `SYS_OPEN` | `(const char *path, int flags, int mode)` | Opens file via VFS | Page-boundary safe string scan | **STABLE** |
| `15`| `SYS_READ` | `(int fd, void *buf, size_t count)` | Reads data from file descriptor | Writable user pointer check | **CERTIFIED** |
| `16`| `SYS_GUI_CREATE_WINDOW`| `(int x, int y, int w, int h, ...)` | Creates desktop window surface | Title string check | **STABLE** |
| `17`| `SYS_GUI_DESTROY_WINDOW`| `(uint32_t win_id)` | Destroys window surface | Handle validation | **STABLE** |
| `18`| `SYS_GUI_SHOW_WINDOW`| `(uint32_t win_id, uint32_t show)` | Toggles window visibility | Handle validation | **STABLE** |
| `19`| `SYS_GUI_SET_BOUNDS`| `(uint32_t win_id, int x, y, w, h)` | Repositions/resizes window | Geometry bounds check | **STABLE** |
| `20`| `SYS_GUI_MAP_SURFACE`| `(uint32_t win_id, uint64_t *fb, ...)` | Maps window surface framebuffer | Writable user pointer check | **CERTIFIED** |
| `21`| `SYS_GUI_INVALIDATE`| `(uint32_t win_id, int x, y, w, h)` | Marks dirty rectangle for BCM | Coordinate bounds check | **STABLE** |
| `22`| `SYS_GUI_POLL_EVENT`| `(uint32_t win_id, BOS_GUIEvent *ev, ...)`| Pulls input events for window | Writable user pointer check | **CERTIFIED** |
| `23`| `SYS_GUI_GET_SCREEN_INFO`| `(uint32_t *w, uint32_t *h, uint32_t *bpp)`| Queries display dimensions | Writable user pointer check | **CERTIFIED** |
| `24`| `SYS_GUI_DRAW_WALLPAPER`| `(uint32_t wp_id, int x, y, w, h)`| Blits wallpaper surface | Bounds validation | **STABLE** |
| `25`| `SYS_CLOSE` | `(int fd)` | Closes file descriptor | Handle validation | **STABLE** |
| `26`| `SYS_SEEK` | `(int fd, int64_t offset, int whence)`| Adjusts file offset | Bounds check | **STABLE** |
| `27`| `SYS_THREAD_SPAWN`| `(void (*entry)(void *), void *arg, ...)`| Spawns thread within process | Stack & entry range checks | **EXPERIMENTAL** |
| `28`| `SYS_THREAD_EXIT` | `(int code)` | Terminates active thread | None | **EXPERIMENTAL** |
| `29`| `SYS_WRITE_FILE` | `(int fd, const void *buf, size_t count)` | Writes buffer to VFS file | User range + read-perm check | **STABLE** |

---

## 10. Syscall Security & Pointer Validation

### 1. Root Cause Analysis: The Unmapped Pointer Vulnerability
During an architectural security audit of the Ring 3 $\rightarrow$ Ring 0 boundary, a critical structural weakness was identified:
- **The Defect**: Syscall handlers originally verified pointer arguments using a superficial numerical range check (e.g. `(uint64_t)ptr < 0x80000000`).
- **The Exploit / Crash Vector**: A user application could pass an address that was within the user range, but **unmapped** in the active PML4 page tables. When kernel code subsequently dereferenced this address, a page fault (`#PF`) occurred in **Ring 0 (CPL 0)**, resulting in an unhandled kernel panic.
- **Secondary Vectors Discovered**:
  - *Read-Only Output Targets*: Passing write-protected user memory to output syscalls (`SYS_CLOCK_GETTIME`, `SYS_READ`) caused write-violation page faults in Ring 0.
  - *Cross-Page Boundary Holes*: Buffers beginning in a mapped page but ending in an unmapped page caused page faults mid-copy.
  - *Unterminated Strings*: Malicious strings without null termination caused `strlen` loops to run off the end of user space.

### 2. Surgical VMM-Backed Hardening Architecture ([`kernel/core/syscall/src/validation.c`](file:///d:/Signatures_OS/kernel/core/syscall/src/validation.c))
The validation engine was rewritten with active page-table verification:
1. **Canonical Form Check**: Verifies bits 48 through 63 match bit 47 (`vmm_address_canonical`).
2. **User Window Enforcement**: Enforces `[0x40000000, 0x80000000)` user bounds.
3. **Integer Overflow Wrap Detection**: Guards against `(uint64_t)ptr + size < (uint64_t)ptr`.
4. **VMM Page-Table Walk (`vmm_validate_user_range`)**: Iterates through every 4096-byte page boundary spanned by the buffer, inspecting PML4 $\rightarrow$ PDPT $\rightarrow$ PD $\rightarrow$ PT entries to verify:
   - `PAGE_PRESENT` is asserted.
   - `PAGE_USER` is asserted (preventing Ring 3 pointers from targeting kernel mappings).
   - `PAGE_WRITABLE` is asserted if the syscall writes output to the buffer (`syscall_validate_user_ptr_writable`).
5. **Page-Safe String Scanning (`syscall_validate_user_string`)**: Reads strings character-by-character while checking page-table presence **before** crossing each 4KB page boundary. Unterminated strings are rejected safely with `SYSCALL_BAD_ADDRESS` without triggering `#PF`.

### 3. Forensic Test Matrix & Stress Results
Validated against the controlled 10-test vulnerability reproduction matrix:

| Test ID | Scenario | Pointer / Target | Expected Behavior | Observed Result | Verdict |
| :--- | :--- | :--- | :--- | :--- | :---: |
| **TEST A** | Valid User Pointer | `0x40020000` (`SYS_WRITE`) | Kernel processes payload | `PASS (SYSCALL_OK) Code=0` | 🟢 **PASS** |
| **TEST B** | NULL Pointer | `0x00000000` (`SYS_WRITE`) | Reject safely | `REJECTED SAFELY Code=-4` | 🟢 **PASS** |
| **TEST C** | Unmapped User Pointer | `0x45000000` (`SYS_WRITE`) | Detect missing frame in PML4 | `REJECTED SAFELY Code=-4` | 🟢 **PASS** |
| **TEST D** | Read-Only User Page | `0x40030000` (`SYS_CLOCK_GETTIME`) | Check writable bit, reject | `REJECTED SAFELY Code=-4` | 🟢 **PASS** |
| **TEST E** | Cross-Page Unmapped | `0x40040FF0` (`SYS_WRITE`, 64B) | Detect second 4KB page unmapped | `REJECTED SAFELY Code=-4` | 🟢 **PASS** |
| **TEST F** | Huge Buffer Size | `0x40020000` (2GB buffer) | Reject exceeding 1GB user window | `REJECTED SAFELY Code=-4` | 🟢 **PASS** |
| **TEST G** | Address Overflow Wrap | `0xFFFFFFFFFFFFFFF0` (`SYS_WRITE`) | Detect integer overflow wrap | `REJECTED SAFELY Code=-4` | 🟢 **PASS** |
| **TEST H** | Kernel-Space Address | `0xC0001000` (`SYS_WRITE`) | Reject Ring 0 memory address | `REJECTED SAFELY Code=-4` | 🟢 **PASS** |
| **TEST I** | Non-Canonical Address | `0x800000000000` (`SYS_WRITE`) | CPU canonical bit check rejects | `REJECTED SAFELY Code=-4` | 🟢 **PASS** |
| **TEST J** | Unterminated String | `0x40050FF0` (`SYS_DEBUG_PRINT`) | Scan halts at unmapped boundary | `REJECTED SAFELY Code=-4` | 🟢 **PASS** |

- **Interleaved Heavy Stress Suite (600 Cycles)**:
  - **100 Valid Syscalls**: Interleaved normal execution (`SYS_WRITE` with valid payload) $\rightarrow$ **100/100 Succeeded (`SYSCALL_OK`)**
  - **500 Malicious Probes**: Interleaved probes targeting unmapped memory, read-only pages, cross-page holes, and unterminated strings $\rightarrow$ **100% Rejected Safely (`SYSCALL_BAD_ADDRESS`)**
  - **Telemetry**: **0 Kernel Panics**, **0 CPL 0 Faults**, continuous live heartbeat ticking.

> **Formal Claim Scope**: Certified PASS for the tested syscall pointer-validation and fault-containment matrix. (This does not constitute a claim that the entire operating system is free from all potential software defects).  
> **Evidence Reference**: [`CERTIFICATION_REPORT.md`](docs/certifications/CERTIFICATION_REPORT.md) & [`FORENSIC_REPORT.md`](file:///D:/Signatures_OS/docs/history/FORENSIC_REPORT.md)

---

## 11. Input Architecture & USB xHCI Subsystem

Input is processed through native hardware drivers.

```
USB Keyboard/Mouse Peripherals ──> xHCI Controller (PCI MMIO) ──> Transfer TRB ──> Event Ring ──> USB HID Core ──> Input Queue
```

### 1. USB xHCI Host Controller Driver ([`kernel/drivers/usb/host/xhci/`](file:///d:/Signatures_OS/kernel/drivers/usb/host/xhci/))
- **PCI Discovery**: Locates the USB xHCI Host Controller (`Class 0x0C`, `Subclass 0x03`, `ProgIF 0x30`).
- **MMIO & Extended Capabilities**: Maps Base Address Registers (`BAR0`), negotiates `USBLEGSUP` bios-handoff semantics, resets controller, sets `MaxSlotsEn`, and configures the Device Context Base Address Array (`DCBAA`).
- **Ring Architecture**:
  - Control Endpoint 0 Transfer Ring: Configured to **1024 TRBs**.
  - Primary Event Ring: Configured to **1024 TRBs** via Event Ring Segment Table (`ERST`).
- **Resolved Hardware Event Ring Defect**: An event ring corruption issue was resolved where `xhci_poll()` invoked event handlers before popping the TRB or updating the Event Ring Dequeue Pointer (`ERDP`). Nested control transfers within event handlers were re-reading stale events and writing corrupted ERDP addresses. The loop was restructured to pop the TRB, advance `ring->dequeue`, toggle `ring->cycle`, and update the physical `ERDP` register **before** dispatching transfer completion handlers.

### 2. USB HID Keyboard & Hardware Lock LED Subsystem ([`kernel/drivers/usb/class/usb_hid.c`](file:///d:/Signatures_OS/kernel/drivers/usb/class/usb_hid.c))
- **Descriptor Boundary Enforcement**: Parses USB configuration descriptors with strict interface boundary checking (`hdr->bDescriptorType == USB_DESC_INTERFACE`), preventing endpoint descriptors from bleed-assigning secondary composite interfaces.
- **Hardware Lock LED Synchronization**:
  - Parses the HID Report Descriptor natively, locating Output Report Usage Page `0x08` (LEDs) and mapping bit offsets for Num Lock (Bit 0), Caps Lock (Bit 1), and Scroll Lock (Bit 2).
  - Submits non-blocking EP0 control transfers: `SET_REPORT` (`bRequest = 0x09`, `wValue = 0x0200`, `wLength = 1`).
- **Bare-Metal Hardware Certification**:
  - Verified on physical **ASUS B750M-K** (Intel Core i3-14100F) with physical USB keyboard (VID `0xC0F4`, PID `0x0201`).
  - **200/200 Transfer ACKs Verified**: Caps Lock, Num Lock, and Scroll Lock toggle bidirectionally on hardware with zero dropped key reports.
  - **Concurrency**: Concurrent rapid typing and high-frequency mouse movements operate smoothly without starvation.
  - **Milestone Commit**: `c51e9ea`, Tag: `v2.7-stable-usb-hid-keyboard-led`.

> **Evidence Reference**: [`docs/certifications/USB_HID_KEYBOARD_LED_FORENSIC_AUDIT.md`](file:///d:/Signatures_OS/docs/certifications/USB_HID_KEYBOARD_LED_FORENSIC_AUDIT.md)

---

## 12. Graphics & Framebuffer Architecture

ATOMS OS uses a high-performance software graphics pipeline rendering to linear VRAM.

### 1. Framebuffer Abstraction
- **Linear VRAM**: Direct pixel access via the 32-bit linear address space negotiated by the UEFI GOP driver (`boot_info->vbe_framebuffer`).
- **Color Depth & Geometry**: 32 bits per pixel (BPP), BGRA/RGBA pixel arrangement.
- **Hardware Tested Resolutions**:
  - `2560x1600` @ 32bpp (Tested on Intel Core i3-14100F + RTX 4060 platform via UEFI GOP).
  - `1920x1080` @ 32bpp (Tested on Intel H81 testbed via UEFI GOP).

### 2. Display Governance & HAL Layers
- **Display Governance Layer (DGL)** ([`kernel/display/dgl/`](file:///d:/Signatures_OS/kernel/display/dgl/)): Coordinates display state transitions (`DGL_STATE_BOOT`, `DGL_STATE_LOGIN`, `DGL_STATE_DESKTOP`), arbitrating VRAM access authority.
- **BSPE Display HAL** ([`kernel/graphics/BSPE/`](file:///d:/Signatures_OS/kernel/graphics/BSPE/)): Manages presentation queues, double-buffering damage tracking, hardware cursor planes, and swapchain frame pacer.

> **Note on GPU Drivers**: Rendering is currently performed via software rasterization over the platform-provided UEFI GOP linear framebuffer. ATOMS OS does not load proprietary 3D hardware-accelerated GPU kernel drivers.

---

## 13. Compositor & Desktop Shell

### 1. BOS Composition Manager (BCM) ([`kernel/wm/bcm/`](file:///d:/Signatures_OS/kernel/wm/bcm/))
- **Window Surfaces**: Maintains an ordered list of composited window surfaces with alpha blending and dirty-region damage tracking.
- **Cursor Plane**: Composited independently on top of window surfaces to prevent cursor trail artifacts.
- **Thread Safety**: Runs as an independent kernel task (`BCM_StartCompositorTask`) synchronized with input events.

### 2. Desktop Shell Architecture ([`kernel/shell/desktop_shell/`](file:///d:/Signatures_OS/kernel/shell/desktop_shell/))
- **Taskbar & Start Menu**: Rendered natively via BOS Window Engine (BWE) with interactive button widgets, system tray clock, and application launcher menus.
- **Wallpaper Service**: Supports wallpaper rendering from raw background buffers and decoding services.
- **BOS Boot Experience (ROOK Engine)**: Manages boot splash sequence with smooth spinning progress indicators before transitioning authority to the desktop.

---

## 14. Filesystem & VFS Architecture

The Virtual File System ([`kernel/vfs/vfs_legacy/`](file:///d:/Signatures_OS/kernel/vfs/vfs_legacy/)) exposes standard file operations (`open`, `read`, `write`, `seek`, `close`).

### Implemented Filesystem Drivers:
1. **FAT32** ([`kernel/vfs/vfs_legacy/fs/fat32/`](file:///d:/Signatures_OS/kernel/vfs/vfs_legacy/fs/fat32/)):
   - Reads/writes directory tables, short/long filenames, and cluster chains.
   - Primary filesystem utilized for UEFI boot media and EFI system partitions.
2. **NTFS** ([`kernel/vfs/vfs_legacy/fs/ntfs/`](file:///d:/Signatures_OS/kernel/vfs/vfs_legacy/fs/ntfs/)):
   - Native Master File Table (MFT) parsing, resident and non-resident attribute navigation, data runlist decoding, and record write-back.
   - Certified against Windows XP-created NTFS images containing fragmented multi-extent files up to 200MB.

### Current Limitations:
- Unix filesystems (such as ext4, ext3, or btrfs) are **not implemented**.
- Mount/unmount lifecycle management across dynamic removable media is basic and currently under development.

---

## 15. Networking Subsystem

ATOMS OS includes native PCIe NIC drivers designed primarily for **bare-metal diagnostic telemetry, remote testing, and PXE boot infrastructure**.

### 1. Hardware NIC Drivers
- **Intel E1000 Driver** ([`kernel/drivers/net/e1000/`](file:///d:/Signatures_OS/kernel/drivers/net/e1000/)): PCI device initialization, circular TX/RX descriptor rings, packet transmission.
- **Realtek R8168 / R8111 Driver** ([`kernel/drivers/net/r8168/`](file:///d:/Signatures_OS/kernel/drivers/net/r8168/)): Physical PCIe Gigabit NIC driver with 256 physical TX/RX descriptor rings, register unlocking (`9346CR`), and interrupt mitigation.

### 2. Supported Protocol Stack
- **Layer 2 / Ethernet**: Ethernet II framing, MAC filtering, ARP address resolution.
- **Layer 3 / IPv4**: Static IP assignment and DHCP lease negotiation.
- **Layer 4 / UDP**: Raw UDP datagram dispatch and reception.

### 3. Dedicated Telemetry & Control Channels ([`docs/LAN_CONTROL_SYSTEM.md`](file:///d:/Signatures_OS/docs/LAN_CONTROL_SYSTEM.md))
- **Port `9999` (Control & Telemetry)**: Bidirectional command channel (remote reset, ACPI shutdown, heartbeat telemetry).
- **Port `9998` (Video Streaming)**: Uncompressed raw framebuffer streaming for remote visual diagnostics.
- **Port `9997` (Binary Telemetry)**: High-speed structured forensic data packets.

> **Operational Distinction**: The network stack is built for kernel debugging, remote lab automation, and telemetry. It is not currently a general-purpose Internet transport stack (TCP stream reassembly is experimental).

---

## 16. Debug & Forensic Engineering (AI-(P)DEBUG)

A core design characteristic of ATOMS OS is its forensic observability architecture. The operating system includes internal instrumentation designed to avoid guesswork during hardware bring-up.

### 1. The Epistemological Evidence Model
Diagnostic assertions and forensic findings are classified under a strict truth model:
- **`OBSERVED`**: Empirically measured directly from physical hardware registers, bus sniffers, memory dumps, or serial output.
- **`DERIVED`**: Mathematically or logically calculated from verified `OBSERVED` facts (e.g. subtracting base addresses).
- **`INFERRED`**: Proposed based on high-probability architectural correlation, requiring validation before code modification.
- **`UNKNOWN`**: Explicitly unverified parameters; never assumed.

### 2. Forensic Infrastructure Components
- **Advanced Bare-Metal Diagnostic Engine (ABDE)** ([`kernel/debug/abde/`](file:///d:/Signatures_OS/kernel/debug/abde/)): Renders on-screen status tables and hardware metrics.
- **Live Heartbeat Spinner**: Dedicated rotating character ticker (`| / - \`) rendered directly from timer interrupts and thread loops, proving continuous kernel execution.
- **COM1 Serial Logging**: Unbuffered character output via I/O Port `0x3F8` at 115200 baud, independent of video memory or interrupts.
- **Cooperative Screenshot Streaming Engine** ([`kernel/debug/screenshot/`](file:///d:/Signatures_OS/kernel/debug/screenshot/)):
  - Captures 32-bit linear framebuffer contents and transmits fragmented UDP packets to a remote listener station.
  - **Non-Blocking Architecture**: Replaced an earlier synchronous loop (which sent ~6,011 packets in a single blocking call and stalled the CPU for ~4.5 seconds) with a cooperative state machine (`atoms_screenshot_step`). It transmits up to 4 chunks (~5.6KB) per scheduler iteration, completing transfers smoothly in the background without starving input or display threads.

---

## 17. Real Hardware Validation Matrix

ATOMS OS has undergone physical bare-metal hardware testing across multiple generations of x86_64 architecture.

### 1. Primary Subsystem-Certified Platforms

| Motherboard / Platform | CPU Architecture | Memory | Subsystems Formally Certified |
| :--- | :--- | :--- | :--- |
| **ASUS B750M-K** (LGA1700) | **Intel Core i3-14100F** (4P/8T @ 3.50GHz) | 32 GB DDR5 | - **VMM Lifecycle** (100-cycle zero-leak pass)<br>- **PMM Stress** (1,920 cycles zero-drift pass)<br>- **Syscall / TSS / SMP** (8 cores online, BSP syscall)<br>- **Syscall Security** (600-cycle malicious pointer containment)<br>- **USB HID Keyboard / Lock LED** (200/200 ACK pass) |
| **Intel H81 Motherboard** (LGA1150) | **Intel Core i3-4130** (2C/4T @ 3.40GHz) | 8 GB DDR3 | - **CPU Features Engine** (CPUID / SSE / LDMXCSR)<br>- **GDT Engine** (Long-mode descriptors)<br>- **SMP Engine** (4 cores online via INIT-SIPI)<br>- **IDT Engine** (Exception handling)<br>- **PIC / APIC Remap** (Timer & keyboard IRQs)<br>- **Heap Stage A** (Basic heap bring-up) |

### 2. Boot-Tested Platforms (Empirical Compatibility)
ATOMS OS has been successfully boot-tested on platforms including:
- Intel Core i3-3220 (Ivy Bridge)
- Intel Core i7-3770 (Ivy Bridge)
- Intel Core i5-12HX (Alder Lake Mobile)
- Discrete GPUs present in testbeds: NVIDIA GeForce RTX 4060, RTX 3050, GTX 1650, Intel HD/UHD Graphics (all operating via platform UEFI GOP framebuffer mode).

---

## 18. Certified Engineering Milestones

The following milestones represent verified engineering accomplishments backed by formal certification reports in the repository:

| Milestone | Hardware Testbed | Test Scope & Metrics | Verdict | Formal Documentation |
| :--- | :--- | :--- | :---: | :--- |
| **CPU Certification** | Intel H81 / i3-4130 | Boot stack, CPUID, SSE enable, return to C | **PASS** | [`docs/certifications/CPU_ENGINE_CERTIFICATION.md`](file:///d:/Signatures_OS/docs/certifications/CPU_ENGINE_CERTIFICATION.md) |
| **GDT Certification** | Intel H81 / i3-4130 | Long-mode code/data selectors, GDTR load | **PASS** | [`docs/certifications/GDT_ENGINE_CERTIFICATION.md`](file:///d:/Signatures_OS/docs/certifications/GDT_ENGINE_CERTIFICATION.md) |
| **SMP Certification** | Intel H81 / i3-4130 | 4 logical cores booted via AP trampoline | **PASS** | [`docs/certifications/SMP_ENGINE_CERTIFICATION.md`](file:///d:/Signatures_OS/docs/certifications/SMP_ENGINE_CERTIFICATION.md) |
| **IDT Certification** | Intel H81 / i3-4130 | 256 IDT descriptors, exception vectors | **PASS** | [`docs/certifications/IDT_ENGINE_CERTIFICATION.md`](file:///d:/Signatures_OS/docs/certifications/IDT_ENGINE_CERTIFICATION.md) |
| **PIC/APIC Certification** | Intel H81 / i3-4130 | 8259A remap, IRQ0/IRQ1 unmasking | **PASS** | [`docs/certifications/PIC_APIC_ENGINE_CERTIFICATION.md`](file:///d:/Signatures_OS/docs/certifications/PIC_APIC_ENGINE_CERTIFICATION.md) |
| **Heap Stage A** | Intel H81 / i3-4130 | Initial heap pool allocation | **PASS** | [`docs/certifications/HEAP_HARDWARE_CERTIFICATION_H81.md`](file:///d:/Signatures_OS/docs/certifications/HEAP_HARDWARE_CERTIFICATION_H81.md) |
| **PMM Stress Audit** | ASUS B750M-K / i3-14100F | 1,920 allocation cycles, 0 net page delta | **PASS** | [`docs/certifications/PMM_FORENSIC_AUDIT.md`](file:///d:/Signatures_OS/docs/certifications/PMM_FORENSIC_AUDIT.md) |
| **VMM Lifecycle** | ASUS B750M-K / i3-14100F | 100 spawn/reap cycles, 0 bytes leaked | **PASS** | [`docs/certifications/VMM_MEMORY_LIFECYCLE_CERTIFICATION.md`](file:///d:/Signatures_OS/docs/certifications/VMM_MEMORY_LIFECYCLE_CERTIFICATION.md) |
| **Syscall/TSS/SMP** | ASUS B750M-K / i3-14100F | 8 cores online, per-CPU TSS, SYSRETQ | **PASS** | [`docs/certifications/SYSCALL_TSS_SMP_FORENSIC_CERTIFICATION.md`](file:///d:/Signatures_OS/docs/certifications/SYSCALL_TSS_SMP_FORENSIC_CERTIFICATION.md) |
| **Input Power Controller**| ASUS B750M-K / i3-14100F | xHCI U0 link state vs USB bus suspend | **PASS** | [`docs/certifications/INPUT_POWER_CONTROLLER_FORENSIC_AUDIT.md`](file:///d:/Signatures_OS/docs/certifications/INPUT_POWER_CONTROLLER_FORENSIC_AUDIT.md) |
| **USB HID Lock LED** | ASUS B750M-K / i3-14100F | 200/200 control transfer ACKs, LED sync | **PASS** | [`docs/certifications/USB_HID_KEYBOARD_LED_FORENSIC_AUDIT.md`](file:///d:/Signatures_OS/docs/certifications/USB_HID_KEYBOARD_LED_FORENSIC_AUDIT.md) |
| **Syscall Security** | ASUS B750M-K & QEMU UEFI | 10-case attack matrix + 600 stress cycles | **PASS** | [`CERTIFICATION_REPORT.md`](docs/certifications/CERTIFICATION_REPORT.md) |

---

## 19. Forensic Engineering Timeline

```text
[Phase 1: Boot & CPU]
   ├── Early stack located at 0x90000 collided with EBDA/SMM on physical H81 hardware
   └── Stack relocated to 16KB .bss section; CPU/GDT/SMP certified on bare metal

[Phase 2: VMM Lifecycle & Memory Reclamation]
   ├── Process teardown leaked 16 pages per process cycle (unfreed PDPT/PD/PTs)
   └── Engineered Hierarchical Ownership-Aware VMM Teardown; 100-cycle 0-leak certified

[Phase 3: Syscall & Multi-Core Isolation]
   ├── Audited multi-core TSS.RSP0 dynamics on 8-core Intel i3-14100F
   └── Certified per-CPU TSS arrays and confirmed safe isolation of userspace on CPU 0

[Phase 4: USB HID Keyboard & Hardware Lock LEDs]
   ├── Root cause: Blocking screenshot UDP stream monopolized CPU for ~4.5 seconds;
   │   xHCI poll recursion double-incremented event ring dequeue pointers;
   │   Descriptor parser crossed interface boundaries
   └── Surgical fixes: Cooperative screenshot state machine; strict ERDP commit ordering;
       HID descriptor parsing locked to Interface 0; 200/200 ACKs certified (commit c51e9ea)

[Phase 5: Syscall Security Hardening]
   ├── User pointer validation checked only numerical address bounds without PML4 check;
   │   Unmapped pointers and read-only targets induced Ring 0 Page Faults
   └── Rewrote validation to query active VMM PML4 page tables page-by-page;
       Engineered page-safe string scanning; 600-cycle attack stress certified
```

---

## 20. Engineering Principles

1. **Evidence Before Modification**: No source code is edited without passive sampling, register tracing, or forensic proof of root cause.
2. **Mandatory Phase Isolation**: Development proceeds strictly through isolated phases: `Investigate` $\rightarrow$ `Plan` $\rightarrow$ `Patch` $\rightarrow$ `Certify`.
3. **Preserve Stable Subsystems**: Working subsystems (such as USB xHCI, VMM lifecycle, and compositor) are frozen and never modified while debugging unrelated features.
4. **Hardware Validation Matters**: Software verification in QEMU serves as a pre-flight sanity check; physical bare-metal hardware validation is mandatory for milestone certification.
5. **No Universal Guarantees**: Certifications are documented with exact test parameters, hardware specifications, cycle counts, and scopes. Universal claims (such as "100% secure" or "universal hardware support") are rejected.

---

## 21. Current Architectural Limitations

Transparency regarding limitations is essential for serious engineering:

- **SMP Scheduling Scope**: All 8 logical cores are brought online and execute live heartbeat loops, but userspace scheduling and syscall dispatch are currently serialized on CPU 0 (BSP). True multi-core parallel task scheduling across APs is not yet enabled.
- **GPU Driver Scope**: Graphics rendering is performed via software rasterization over the platform-provided UEFI GOP linear framebuffer. Native proprietary GPU acceleration (NVIDIA/AMD/Intel kernel DRM/KMS drivers) is not implemented.
- **Filesystem Support**: FAT32 and NTFS are supported; standard Unix filesystems (such as ext4 or btrfs) are not implemented.
- **Network Protocol Breadth**: Network drivers support ARP, IPv4, and UDP datagrams. A full production-grade TCP transport stack with window management and retransmission is not yet completed.
- **Application Ecosystem**: Supports custom native ELF binaries, `.sll` shared libraries, and ported applications (such as Doomgeneric). General third-party POSIX or Win32 binary compatibility is not supported out of the box.

---

## 22. Development Status Classification

- 🟢 **STABLE / CERTIFIED**: UEFI Bootloader (`BOOTX64.EFI`), Kernel Entry (`kernel_entry.asm`), CPU/GDT/IDT/PIC, PMM Allocator, VMM Lifecycle Engine, Syscall Gateway & Security Validation, USB xHCI Driver, USB HID Keyboard & Lock LED Driver, ABDE Telemetry Dashboard.
- 🟡 **IN ACTIVE DEVELOPMENT / FUNCTIONAL**: Desktop Shell & Taskbar, BWE Window Manager, BCM Compositor, Realtek R8168 Driver, Intel E1000 Driver, NTFS Driver.
- 🔴 **EXPERIMENTAL / PLANNED**: Multi-core AP userspace scheduling, full TCP transport stack, ext4 filesystem, native GPU hardware acceleration.

---

## 23. Repository Structure

```text
d:\Signatures_OS\
├── boot/
│   └── uefi/               # Standalone UEFI Bootloader (bootx64.c, EFI protocols)
├── kernel/
│   ├── core/
│   │   ├── cpu/            # GDT, IDT, CPUID, SMP multi-core bring-up
│   │   ├── memory/         # PMM (bitmap), VMM (paging, lifecycle), Heap
│   │   ├── scheduler/      # Task management, queues, context switching
│   │   ├── syscall/        # LSTAR entry, syscall dispatch, VMM security validation
│   │   └── interrupt/      # PIC 8259A, Local APIC, ISR exception handlers
│   ├── drivers/
│   │   ├── usb/            # xHCI 1.0+ host controller, USB HID class driver
│   │   ├── display/        # VBE, linear VRAM acceleration routines
│   │   ├── net/            # Intel E1000 and Realtek R8168 PCI NIC drivers
│   │   └── keyboard/       # Keyboard input queue and LED handlers
│   ├── display/            # Display Governance Layer (DGL), geometry
│   ├── graphics/           # BSPE Display HAL, cursor plane, surface damage tracker
│   ├── wm/bcm/             # BOS Composition Manager (surfaces, present queue)
│   ├── shell/              # Desktop shell, taskbar, start menu, ROOK boot splash
│   ├── vfs/                # Virtual File System, FAT32, NTFS driver
│   ├── net/                # Network packet abstraction, UDP datagrams
│   ├── debug/              # ABDE dashboard, AI-(P)DEBUG engine, cooperative screenshot
│   └── kernel.c            # Kernel main entry point and subsystem orchestrator
├── userspace/              # User applications (desktop shell, Doom, SDK demos, .sll libraries)
├── tools/                  # Build & test tools (gpt_image_builder, pxe_server, qemu test scripts)
├── docs/
│   └── certifications/     # Formal bare-metal forensic certification reports
├── build.ps1               # Master automated build script
├── FORENSIC_REPORT.md      # Syscall security forensic root-cause analysis
├── PATCH_PLAN.md           # Syscall security architectural patch plan
├── PATCH_REPORT.md         # Syscall security source code modification report
├── CERTIFICATION_REPORT.md # Formal Syscall Security Milestone Certification Report
└── README.md               # Authoritative project technical documentation
```

---

## 24. Build & Verification Instructions

### Prerequisites
- **LLVM / Clang Toolchain**: `clang`, `llvm-mc`, `lld-link` (supporting `x86_64-unknown-none` and `x86_64-unknown-windows` targets).
- **PowerShell**: Version 5.1 or later.
- **QEMU**: `qemu-system-x86_64` with OVMF pure UEFI firmware (`edk2-x86_64-code.fd`).

### 1. Build the Operating System Image
Execute the automated build script from the repository root:
```powershell
powershell -ExecutionPolicy Bypass -File .\build.ps1
```
This builds:
- `build/BOOTX64.EFI` — Standalone UEFI bootloader.
- `build/kernel.bin` — Compiled 64-bit BOS Kernel payload.
- `build/OS.img` — Master FAT32 raw disk image.
- `build/SignaturesOS.vmdk` — VMware / VirtualBox disk container.

### 2. Generate Pristine UEFI GPT Test Image
```powershell
& .\build\gpt_image_builder.exe build\BOOTX64.EFI build\kernel.bin build\atoms_uefi_test.img
```

### 3. Automated QEMU UEFI Pre-Flight Validation
Run the verified pre-flight script:
```powershell
powershell -ExecutionPolicy Bypass -File .\tools\run_qemu_test.ps1
```
Launches QEMU in pure UEFI mode with OVMF, attaches virtual xHCI USB devices, enables COM1 serial capture to `build/uefi_forensic_serial.log`, executes the security and functional test harness, and terminates cleanly.

### 4. Bare-Metal Hardware Deployment

#### Option A: UEFI Network Boot (PXE / TFTP)
1. Start the included PXE server on the host machine (`192.168.2.1`):
   ```powershell
   python tools/pxe_server.py
   ```
2. Connect the target PC (e.g. ASUS B750M-K) via Cat6 Ethernet cable.
3. Configure the target BIOS to **UEFI Network Boot (IPv4)**. The motherboard will download `BOOTX64.EFI` and `kernel.bin` and boot cleanly into ATOMS OS.

#### Option B: Physical USB Flash Drive
1. Identify the target USB drive number using PowerShell (`Get-Disk`).
2. Write the raw GPT image to the USB drive using raw image writing tools (e.g. `dd if=build/atoms_uefi_test.img of=\\.\PhysicalDriveN bs=1M`).
3. Boot the target PC with UEFI mode enabled.

---

## 25. Debug Builds vs. Production Builds

ATOMS OS isolates diagnostic subsystems via preprocessor directives in [`kernel/kernel.c`](file:///d:/Signatures_OS/kernel/kernel.c):
```c
#define ATOMS_DEBUG_MODE_NONE             0
#define ATOMS_DEBUG_MODE_VMM              1
#define ATOMS_DEBUG_MODE_SYSCALL_TSS      2
#define ATOMS_DEBUG_MODE_PMM              3
#define ATOMS_DEBUG_MODE_KEYBOARD_LED     4
#define ATOMS_DEBUG_MODE_SYSCALL_SECURITY 5

#define ATOMS_ACTIVE_DEBUG_MODE           ATOMS_DEBUG_MODE_NONE
```
- When `ATOMS_ACTIVE_DEBUG_MODE` is set to `ATOMS_DEBUG_MODE_NONE`, the kernel boots normally through the official ROOK boot splash sequence and transitions directly to the desktop shell.
- When set to a diagnostic mode (e.g. `ATOMS_DEBUG_MODE_SYSCALL_SECURITY`), the boot splash is bypassed, and the kernel runs the dedicated full-screen ABDE diagnostic harness and telemetry stream.

---

## 26. Formal Security Statement

ATOMS OS includes explicit hardware and page-table validation at the Ring 3 $\rightarrow$ Ring 0 syscall boundary. It has undergone forensic attack testing and heavy stress validation against invalid, unmapped, cross-page, overflowed, and write-protected user pointer configurations.

**Scope of Certification**:  
The documented certification establishes that the tested syscall pointer-validation and fault-containment matrix safely rejected 100% of invalid user-pointer inputs with zero kernel panics and zero CPL 0 page faults under the documented test conditions. This certification does not constitute a claim that the entire operating system is universally free from all vulnerabilities, side channels, or software defects.

---

## 27. Primary Certification Documents

Clickable repository references to authoritative milestone reports:

- 📄 [**Syscall Security Formal Certification Report**](docs/certifications/CERTIFICATION_REPORT.md)
- 📄 [**Syscall Security Forensic Root-Cause Analysis**](file:///D:/Signatures_OS/docs/history/FORENSIC_REPORT.md)
- 📄 [**Syscall Security Architectural Patch Plan**](file:///D:/Signatures_OS/docs/history/PATCH_PLAN.md)
- 📄 [**VMM Memory Lifecycle Hardware Certification**](file:///d:/Signatures_OS/docs/certifications/VMM_MEMORY_LIFECYCLE_CERTIFICATION.md)
- 📄 [**PMM Physical Memory Manager Forensic Audit**](file:///d:/Signatures_OS/docs/certifications/PMM_FORENSIC_AUDIT.md)
- 📄 [**Syscall / TSS / SMP Multi-Core Forensic Certification**](file:///d:/Signatures_OS/docs/certifications/SYSCALL_TSS_SMP_FORENSIC_CERTIFICATION.md)
- 📄 [**USB HID Keyboard & Hardware Lock LED Certification**](file:///d:/Signatures_OS/docs/certifications/USB_HID_KEYBOARD_LED_FORENSIC_AUDIT.md)
- 📄 [**Input Power Controller & xHCI Link State Audit**](file:///d:/Signatures_OS/docs/certifications/INPUT_POWER_CONTROLLER_FORENSIC_AUDIT.md)
- 📄 [**SMP Multi-Core Engine Hardware Certification**](file:///d:/Signatures_OS/docs/certifications/SMP_ENGINE_CERTIFICATION.md)
- 📄 [**CPU Features Engine Hardware Certification**](file:///d:/Signatures_OS/docs/certifications/CPU_ENGINE_CERTIFICATION.md)
- 📄 [**GDT Engine Hardware Certification**](file:///d:/Signatures_OS/docs/certifications/GDT_ENGINE_CERTIFICATION.md)
- 📄 [**IDT Engine Hardware Certification**](file:///d:/Signatures_OS/docs/certifications/IDT_ENGINE_CERTIFICATION.md)
- 📄 [**PIC / APIC Engine Hardware Certification**](file:///d:/Signatures_OS/docs/certifications/PIC_APIC_ENGINE_CERTIFICATION.md)
- 📄 [**Heap Stage A Hardware Certification**](file:///d:/Signatures_OS/docs/certifications/HEAP_HARDWARE_CERTIFICATION_H81.md)
- 📄 [**Bare-Metal LAN Control & Telemetry System**](file:///d:/Signatures_OS/docs/LAN_CONTROL_SYSTEM.md)

---

## 28. Recent Engineering Milestones

- **Syscall Pointer Security Hardened**: Replaced naive range checks with active VMM PML4 page-table walks and safe page-boundary string scanning; certified under 600-cycle stress test.
- **USB HID Lock LED Stabilization**: Resolved xHCI event-ring reentrancy corruption and descriptor parsing boundary leaks; certified bidirectional LED sync with 200/200 control transfer ACKs on bare-metal hardware.
- **Cooperative UDP Screenshot Pipeline**: Replaced synchronous 6,011-packet blocking transmissions with a non-blocking cooperative state machine, completely eliminating CPU input starvation during remote frame capture.
- **VMM Lifecycle Memory Leak Elimination**: Implemented hierarchical ownership-aware address-space destruction, achieving zero memory leakage across 100 continuous process spawn/reap cycles on physical hardware.
- **PMM Stress Verification**: Confirmed zero frame drift across 1,920 contiguous and multi-page allocation cycles on a 32 GB physical memory system.
- **Multi-Core Topology & TSS Verification**: Booted all 8 logical execution units of an Intel Core i3-14100F into 64-bit Long Mode with dedicated per-CPU TSS and GDT arrays.

---

## 29. Licensing, Attribution & Intellectual Property

### 1. Project Authorship & Core System
- **ATOMS OS & BOS Kernel**: The core kernel architecture, boot protocols, memory managers (PMM/VMM), scheduler, hardware abstraction layer, system call gateway, compositing engine, and device drivers are original designs and implementations authored as part of the ATOMS OS project.
- **Native Filesystem (BOFS)**: BOFS (BOS Operating Filesystem) is the planned native filesystem for ATOMS OS, designed with its own independent architecture, on-disk structures, and implementation. **BOFS is not NTFS, is not Linux NTFS, and is not a copy or derivative of NTFS.**
- **Project License Status**: ATOMS OS is licensed under the **ATOMS OS — Open Source Use, Attribution & Brand Protection License (Version 1.0)**. The source code is publicly accessible for study, modification, and commercial application while preserving core authorship attribution, provenance, and trademark ownership. See [`LICENSE`](file:///d:/Signatures_OS/LICENSE) or [`LICENSE.md`](file:///d:/Signatures_OS/LICENSE.md) for full terms.

### 2. External References & Interoperability Research
- **Filesystem Interoperability (NTFS)**: The legacy NTFS driver in ATOMS OS was developed for partition discovery, telemetry extraction, and filesystem interoperability. In designing and debugging this implementation, public technical documentation (such as Microsoft Open Specifications `[MS-FSCC]` / `[MS-FSA]`) and public open-source implementations (such as Linux kernel `fs/ntfs3` and NTFS-3G) were consulted as reference material for behavioral compatibility, tie-breaking rules, and on-disk invariants.
- **Non-Originality of External Concepts**: Merely studying external specifications or implementations does not incorporate them into the codebase. However, ATOMS OS makes no claim of originality over NTFS concepts, Microsoft specifications, or Linux `ntfs3`/NTFS-3G designs. Any future reuse, porting, or inclusion of external code from such projects must remain strictly subject to the respective upstream licenses (e.g. GNU General Public License v2) and must never be represented as ATOMS-original work.

### 3. Third-Party Open-Source Components & Libraries
Permissive open-source components utilized, adapted, or vendored within userspace, toolchains, or specific subsystems are acknowledged under their respective licenses:
- **musl libc**: MIT License (Rich Felker et al.)
- **LLVM / Clang / LLD / libc++**: Apache 2.0 with LLVM Exception
- **Google Skia**: BSD 3-Clause License
- **Google V8**: BSD 3-Clause License
- **Chromium Subsystems**: BSD 3-Clause License
- **Capstone Disassembly Engine**: BSD 3-Clause License (located in `capstone_src/`)
- For complete third-party notices and full license texts, see [`ATOMS_THIRDPARTY_LICENSES.md`](file:///D:/Signatures_OS/docs/third_party/ATOMS_THIRDPARTY_LICENSES.md).

