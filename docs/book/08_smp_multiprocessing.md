# Chapter 08: Symmetric Multiprocessing (SMP)

ATOMS OS supports multi-core CPU architectures using ACPI MADT discovery and the standard x86_64 inter-processor interrupt (IPI) boot protocol.

## 1. Topology Discovery
During boot, the kernel traverses the ACPI Root System Description Pointer (RSDP) to find the Multiple APIC Description Table (MADT / APIC). It parses:
- Type 0: Processor Local APIC entries (recording APIC ID and enabled status).
- Type 1: I/O APIC entries (recording physical MMIO base and Global System Interrupt base).

## 2. AP Bring-Up Sequence
1. **Trampoline Allocation**: A 16-bit real-mode startup routine is copied to a page-aligned physical address below 1 MB (e.g. `0x08000`).
2. **INIT IPI**: The Bootstrap Processor (BSP) issues an INIT IPI via its LAPIC Interrupt Command Register (ICR) to reset Application Processors (APs).
3. **SIPI Sequence**: The BSP sends two Startup IPIs (SIPI) with the page vector of the real-mode trampoline.
4. **Transition to Long Mode**: Each AP enables A20, loads temporary 32-bit GDT, enables PAE and Long Mode in `EFER.LME`, sets CR3 to the kernel PML4, and jumps to 64-bit C entry point.
