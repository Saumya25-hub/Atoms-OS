# Chapter 09: Physical Memory Manager (PMM)

The **PMM** allocates and frees physical 4 KB page frames across system memory.

## 1. Bitmap Allocator Design
- **Resolution**: 1 bit represents 1 physical 4 KB frame (`0x1000` bytes).
- **Capacity**: Manages up to 32 GB of physical RAM using a contiguous 1 MB bitmap located in low kernel memory.
- **Frame State**:
  - `0`: Frame is free and available for allocation.
  - `1`: Frame is reserved, occupied by kernel code, ACPI, MMIO, or assigned to a page table.

## 2. UEFI Sanitization
The PMM sanitizes the UEFI memory map descriptor table:
- Preserves `EfiLoaderCode`, `EfiLoaderData`, `EfiBootServicesCode`, and `EfiBootServicesData` as freeable once boot services exit.
- Marks `EfiRuntimeServicesCode`, `EfiRuntimeServicesData`, `EfiACPIReclaimMemory`, and `EfiMemoryMappedIO` as permanently reserved.
- Zeroes all newly allocated physical pages before assignment to prevent information disclosure across privilege rings.
