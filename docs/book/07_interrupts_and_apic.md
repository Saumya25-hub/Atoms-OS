# Chapter 07: Interrupt Architecture & APIC

## 1. Interrupt Descriptor Table (IDT)
The kernel configures a 256-entry IDT supporting CPU exceptions, hardware IRQs, and software interrupts:
- Vectors `0..31`: Architectural CPU exceptions (`#DE`, `#DB`, `#BP`, `#OF`, `#BR`, `#UD`, `#NM`, `#DF`, `#TS`, `#NP`, `#SS`, `#GP`, `#PF`, `#MF`, `#AC`, `#MC`, `#XM`).
- Vectors `32..47`: Legacy remapped 8259 PIC IRQs (masked after APIC enable).
- Vectors `48..254`: I/O APIC and MSI/MSI-X device interrupts.
- Vector `255`: Local APIC Spurious Interrupt vector.

## 2. Advanced Programmable Interrupt Controller (APIC)
- **Local APIC (LAPIC)**: Initialized via MSR `IA32_APIC_BASE` (physical base address `0xFEE00000`). Configured for flat physical delivery mode.
- **LAPIC Timer**: Calibrated against the 8254 PIT to generate periodic 1000 Hz system timer ticks without legacy IRQ0 jitter.
- **I/O APIC**: Discovered via ACPI MADT table; routes PCI Express interrupts directly to target CPU cores.
