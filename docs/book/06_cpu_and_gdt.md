# Chapter 06: CPU Architecture & GDT/TSS

ATOMS OS operates exclusively in 64-bit Long Mode with paging enabled.

## 1. Global Descriptor Table (GDT)
The kernel initializes a 64-bit Flat GDT with dedicated segment descriptors:
- Index 0: Null Descriptor (`0x00`)
- Index 1: Ring 0 64-bit Code Segment (`0x08`, Base 0, Limit 0, Read/Execute, DPL 0, L=1)
- Index 2: Ring 0 Data Segment (`0x10`, Base 0, Limit 0, Read/Write, DPL 0)
- Index 3: Ring 3 64-bit Data Segment (`0x18 | 3 = 0x1B`, DPL 3)
- Index 4: Ring 3 64-bit Code Segment (`0x20 | 3 = 0x23`, DPL 3, L=1)
- Index 5 & 6: 16-byte Task State Segment (TSS) Descriptor (`0x28`)

## 2. Task State Segment (TSS) & Privilege Stacks
Privilege isolation requires hardware stack switching:
- **`RSP0`**: Points to the top of the kernel supervisor stack. When an interrupt occurs in Ring 3, the CPU automatically reloads `RSP` from `TSS.RSP0`.
- **Interrupt Stack Table (IST)**:
  - `IST1`: Dedicated 16 KB Double Fault (`#DF`) stack.
  - `IST2`: Dedicated 16 KB Non-Maskable Interrupt (`#NMI`) stack.
  - `IST3`: Dedicated 16 KB Machine Check (`#MC`) stack.
This guarantees that stack overflows in user mode or kernel mode cannot cause a triple-fault on fault handling.
