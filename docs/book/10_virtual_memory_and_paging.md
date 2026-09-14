# Chapter 10: Virtual Memory & 4-Level Paging

The **Virtual Memory Manager (VMM)** implements 4-level PML4 paging on x86_64, creating distinct virtual address spaces for the kernel and userspace processes.

## 1. Canonical Address Space Split
```
0x0000000000000000 - 0x00007FFFFFFFFFFF : User Space (128 TB, DPL 3 accessible)
0x0000800000000000 - 0xFFFF7FFFFFFFFFFF : Non-Canonical Range (Triggers #GP)
0xFFFF800000000000 - 0xFFFFFFFFFFFFFFFF : Kernel Space (128 TB, Supervisor only)
  • 0xFFFF800000000000 : Direct Physical Map (Identity mapped RAM)
  • 0xFFFFFFFF80000000 : Kernel Code, Data, and Core Stacks (0x100000 physical)
  • 0xFFFFFF8000000000 : Recursive Page Directory Self-Mapping (Entry 510)
```

## 2. Page Directory Hierarchy
- **PML4 (Page Map Level 4)**: 512 entries, each pointing to a PDPT.
- **PDPT (Page Directory Pointer Table)**: 512 entries, each pointing to a PD.
- **PD (Page Directory)**: 512 entries, each pointing to a PT (or 2 MB huge page).
- **PT (Page Table)**: 512 entries, each mapping a 4 KB physical frame.

## 3. Entry Attributes
- `Bit 0 (P)`: Present (1 = in RAM, 0 = page fault).
- `Bit 1 (R/W)`: Read/Write (1 = writable, 0 = read-only).
- `Bit 2 (U/S)`: User/Supervisor (1 = Ring 3 accessible, 0 = Ring 0 only).
- `Bit 63 (XD)`: Execute-Disable (prevents code execution from data/heap pages).
