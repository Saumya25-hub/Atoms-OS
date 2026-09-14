# The Book of ATOMS OS — Engineering Manual & Subsystem Specification

**Author**: Saumya Chaudhari  
**Operating System**: ATOMS OS (BOS Kernel)  
**Target Hardware**: x86_64 Pure UEFI Silicon (Haswell H81, Raptor Lake B760M-K)  

---

## Master Table of Contents

### Volume I: Fundamentals & Boot Pipeline
- [**Chapter 01**: ATOMS OS Project Overview](file:///D:/Signatures_OS/docs/book/01_overview.md)
- [**Chapter 02**: 7-Tier System Architecture](file:///D:/Signatures_OS/docs/book/02_architecture.md)
- [**Chapter 03**: Boot Process & Execution Trampoline](file:///D:/Signatures_OS/docs/book/03_boot_process.md)
- [**Chapter 04**: Pure UEFI Bootloader Subsystem](file:///D:/Signatures_OS/docs/book/04_uefi_subsystem.md)

### Volume II: Kernel Core & Processor State
- [**Chapter 05**: BOS Kernel Architecture](file:///D:/Signatures_OS/docs/book/05_bos_kernel.md)
- [**Chapter 06**: CPU Architecture & GDT/TSS](file:///D:/Signatures_OS/docs/book/06_cpu_and_gdt.md)
- [**Chapter 07**: Interrupt Architecture & APIC](file:///D:/Signatures_OS/docs/book/07_interrupts_and_apic.md)
- [**Chapter 08**: Symmetric Multiprocessing (SMP)](file:///D:/Signatures_OS/docs/book/08_smp_multiprocessing.md)

### Volume III: Memory Management & Syscalls
- [**Chapter 09**: Physical Memory Manager (PMM)](file:///D:/Signatures_OS/docs/book/09_physical_memory_manager.md)
- [**Chapter 10**: Virtual Memory & 4-Level Paging](file:///D:/Signatures_OS/docs/book/10_virtual_memory_and_paging.md)
- [**Chapter 11**: Kernel Dynamic Heap Allocator](file:///D:/Signatures_OS/docs/book/11_kernel_heap.md)
- [**Chapter 12**: Hardware Fast Syscalls (IA32_LSTAR)](file:///D:/Signatures_OS/docs/book/12_syscall_interface.md)

### Volume IV: Device Drivers & Hardware Interfaces
- [**Chapter 13**: Process Lifecycle & Ring 3 Runtime](file:///D:/Signatures_OS/docs/book/13_process_and_ring3.md)
- [**Chapter 14**: Unified Driver Architecture](file:///D:/Signatures_OS/docs/book/14_driver_architecture.md)
- [**Chapter 15**: USB 3.0 xHCI & HID Subsystem](file:///D:/Signatures_OS/docs/book/15_usb_and_xhci.md)
- [**Chapter 16**: NVMe & AHCI Storage Subsystems](file:///D:/Signatures_OS/docs/book/16_storage_subsystems.md)

### Volume V: Filesystems, Desktop & Applications
- [**Chapter 17**: BOFS Filesystem & Transaction Engine](file:///D:/Signatures_OS/docs/book/17_bofs_filesystem.md)
- [**Chapter 18**: Graphics Pipeline & BCM Compositor](file:///D:/Signatures_OS/docs/book/18_graphics_and_compositor.md)
- [**Chapter 19**: Rook Login Supervisor & Desktop Shell](file:///D:/Signatures_OS/docs/book/19_rook_and_desktop_shell.md)
- [**Chapter 20**: Native Applications & Roadmap](file:///D:/Signatures_OS/docs/book/20_applications_and_future.md)

---

## Cross-Reference Links
- [Getting Started Guide](file:///D:/Signatures_OS/docs/START_HERE.md)
- [AI-Assisted Development Manifesto](file:///D:/Signatures_OS/docs/AI_ASSISTED_DEVELOPMENT.md)
- [Hardware Testing Matrix](file:///D:/Signatures_OS/docs/TESTING.md)
- [Known Issues & Active Debt](file:///D:/Signatures_OS/docs/KNOWN_ISSUES.md)
- [Repository Maintenance Standards](file:///D:/Signatures_OS/docs/MAINTENANCE.md)
