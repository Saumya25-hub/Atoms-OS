# ATOMS OS — Physical Memory Manager (PMM) Architecture

## Overview

The **ATOMS OS Physical Memory Manager (PMM)** is a high-performance 4KB physical page frame allocator and memory accounting engine. It parses the UEFI Memory Map provided by `BOOTX64.EFI`, builds a dynamic physical page bitmap, and manages all usable physical RAM with zero LibC dependencies and O(1) allocation performance.

---

## 🏗️ Core Architecture & Data Structures

### 1. Dynamic Page Frame Bitmap
The PMM uses a bit-array representation where each bit represents a single 4096-byte (4KB) physical page:
- `0`: Frame is **FREE** and available for allocation.
- `1`: Frame is **USED** or **RESERVED** (Kernel Image, Stack, Framebuffer, ACPI, MMIO, Low Memory).

```text
Physical Memory (RAM)
[ 0x00000000 - 0x00200000 ] -> RESERVED (Low 2MB: SMP Trampolines, IVT, BDA)
[ 0x00100000 - 0x00XXXXXX ] -> KERNEL IMAGE + PAGE BITMAP
[ 0x00XXXXXX - MAX_RAM    ] -> USABLE PHYSICAL PAGE POOL (Managed by PMM)
```

The bitmap array is dynamically allocated immediately following `&_kernel_end` (aligned to 4KB) to guarantee zero memory fragmentation.

---

## ⚡ PMM API Functions

```c
// Initialize PMM from UEFI memory map and run stress test suite
void pmm_init(boot_info_t *boot_info);

// Allocate a single 4KB physical page (returns physical address)
void* pmm_alloc_page(void);

// Allocate contiguous 4KB physical pages
void* pmm_alloc_pages(size_t count);

// Free a single 4KB physical page
void pmm_free_page(void *phys_addr);

// Free contiguous 4KB physical pages
void pmm_free_pages(void *phys_addr, size_t count);
```

---

## 🛡️ Safety & Forensic Panic Integration

The PMM integrates directly with ABDE V2.5 Forensic Engine:
- **Out of Memory Protection**: Triggers `OUT_OF_MEMORY` diagnostic panic if free physical pages reach 0.
- **Double Free Protection**: Triggers `DOUBLE_FREE` diagnostic panic if `pmm_free_page` is called on an already unreserved page.
- **Alignment Protection**: Validates that all addresses passed to `pmm_free_page` are 4KB-aligned.

On any fault, the system enters an active ABDE diagnostic halt loop with full telemetry preserved, preventing silent reboots or triple faults.
