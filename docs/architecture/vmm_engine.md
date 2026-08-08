# ATOMS OS — Virtual Memory Manager (VMM) Architecture

## Overview

The **ATOMS OS Virtual Memory Manager (VMM)** manages 4-level x86_64 hardware paging (`PML4` -> `PDPT` -> `PD` -> `PT`). It constructs kernel address spaces, manages 4GB physical identity mappings, supports Higher-Half kernel mappings, and activates hardware paging via `CR3` register switching.

---

## 🏗️ 4-Level x86_64 Paging Architecture

```text
Virtual Address (64-bit Canonical)
[ 63 .. 48 ] -> Sign Extension Bits (0x0000 or 0xFFFF)
[ 47 .. 39 ] -> PML4 Index (512 Entries, 512GB per entry)
[ 38 .. 30 ] -> PDPT Index (512 Entries, 1GB per entry)
[ 29 .. 21 ] -> PD Index   (512 Entries, 2MB per entry)
[ 20 .. 12 ] -> PT Index   (512 Entries, 4KB per entry)
[ 11 ..  0 ] -> Page Offset (4096 bytes)
```

---

## 🔒 Memory Layout & Cache Attributes

| Virtual Address Range | Target Physical Memory | Page Size | Cache Attribute |
| :--- | :--- | :--- | :--- |
| `0x0000000000000000` - `0x000000007FFFFFFF` | Physical RAM (0 - 2GB) | 2MB Huge Pages | Write-Back (WB) |
| `0x0000000080000000` - `0x00000000BFFFFFFF` | VBE Framebuffer VRAM (2GB - 3GB) | 2MB Huge Pages | Write-Through (WT) |
| `0x00000000C0000000` - `0x00000000FFFFFFFF` | MMIO / Local APIC / IOAPIC / Heap | 2MB Huge Pages | Uncacheable / Cache-Disable |
| `0xFFFF800000000000`+ | Higher-Half Kernel Mapping | 2MB / 4KB Pages | Write-Back (WB) |

---

## ⚡ Core VMM API

```c
// Initialize VMM, build 4GB identity PML4 tables, activate CR3, and run translation tests
void vmm_init(void);

// Map a 4KB physical page to a virtual address with specified flags
void vmm_map_page(void *pml4, uint64_t phys_addr, uint64_t virt_addr, uint32_t flags);

// Unmap a virtual page
void vmm_unmap_page(void *pml4, uint64_t virt_addr);

// Translate virtual address to underlying physical address
uint64_t vmm_get_physical_address(void *pml4, uint64_t virt_addr);

// Switch active hardware CR3 page table space
void vmm_switch_address_space(void *pml4_phys_addr);
```

---

## 🖼️ Display Corruption Elimination Protocol

Prior to VMM activation, the UEFI handoff protocol was hardened:
1. **Green Bar Elimination**: Explicitly removed premature UEFI bootloader green rectangle write from `boot/uefi/bootx64.c`.
2. **100% VRAM Wipe**: Executed full-framebuffer dark slate blue (`0x000F172A`) wipe post-`ExitBootServices()` to permanently erase leftover firmware console text (`ConOut`).
3. **Clipping & Bounds Safety**: Applied strict coordinate clipping in `abde_renderer.c` preventing out-of-bounds VRAM writes.
