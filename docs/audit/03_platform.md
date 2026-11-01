# AUDIT PART 3: PLATFORM & CHIPSET EMULATION

## 1. Guest Physical Memory Layout (GPA Map)

| GPA Range | Size | Component / Purpose | Emulation Mechanism | Evidence |
|:---|:---|:---|:---|:---|
| 0x00000000 - 0x00000FFF | 4 KB | Real Mode IVT & BDA | Identity mapped in EPT | [VERIFIED-BY-CODE: kernel/core/hypervisor/src/ept.c:45] |
| 0x00010000 - 0x00010FFF | 4 KB | Guest IDT (Pre-allocated) | EPT Read/Write | [VERIFIED-BY-CODE: kernel/core/hypervisor/src/hypervisor.c:3235] |
| 0x00011000 - 0x00011FFF | 4 KB | FreeBSD bootinfo_t & Loader Env | EPT Read/Write | [VERIFIED-BY-CODE: kernel/core/hypervisor/src/freebsd_loader.c:116] |
| 0x00012000 - 0x0001FFFF | 56 KB | FreeBSD Preload Metadata Module Stream | EPT Read/Write | [VERIFIED-BY-CODE: kernel/core/hypervisor/src/freebsd_loader.c:202] |
| 0x00020000 - 0x00024FFF | 20 KB | Guest 64-bit Paging Hierarchy (PML4, PDPT, PD) | EPT Read/Write | [VERIFIED-BY-CODE: kernel/core/hypervisor/src/freebsd_loader.c:40] |
| 0x0007FF00 - 0x0007FFFF | 256 B | Early Bootstrap Stack (GUEST_RSP) | EPT Read/Write | [VERIFIED-BY-CODE: kernel/core/hypervisor/src/hypervisor.c:3267] |
| 0x000E0000 - 0x000E0024 | 36 B | ACPI 2.0 RSDP | EPT Read-Only | [VERIFIED-BY-CODE: kernel/core/hypervisor/src/virtual_platform.c:120] |
| 0x000E0100 - 0x000E0180 | 128 B | ACPI RSDT / XSDT | EPT Read-Only | [VERIFIED-BY-CODE: kernel/core/hypervisor/src/virtual_platform.c:290] |
| 0x000E0200 - 0x000E0300 | 256 B | ACPI FADT | EPT Read-Only | [VERIFIED-BY-CODE: kernel/core/hypervisor/src/virtual_platform.c:270] |
| 0x000E0300 - 0x000E0400 | 256 B | ACPI MADT (APIC / IOAPIC structures) | EPT Read-Only | [VERIFIED-BY-CODE: kernel/core/hypervisor/src/virtual_platform.c:230] |
| 0x000E0500 - 0x000E0FFF | 2.8 KB | ACPI DSDT (AML bytecode) | EPT Read-Only | [VERIFIED-BY-CODE: kernel/core/hypervisor/src/virtual_platform.c:215] |
| 0x00400000 - 0x02000000 | 28 MB | FreeBSD Kernel ELF Segments (btext = 0xFFFFFFFF8037C000) | EPT Read/Write/Exec | [VERIFIED-BY-CODE: kernel/core/hypervisor/src/freebsd_loader.c:320] |
| 0x02000000 - 0x7FFFFFFF | ~1.97 GB | FreeBSD Dynamic Kernel Heap, UMA, Buffer Cache | EPT 2MB Large Pages | [VERIFIED-BY-CODE: kernel/core/hypervisor/src/ept.c:50] |
| 0x80000000 - 0xFEBFFFFF | ~2 GB | PCI MMIO Aperture (BAR space) | EPT Unmapped -> EPT Violation traps | [VERIFIED-BY-CODE: kernel/core/hypervisor/src/virtual_platform.c:190] |
| 0xFEC00000 - 0xFEC00FFF | 4 KB | Virtual IOAPIC Register Window | EPT Violation -> MMIO trap | [VERIFIED-BY-CODE: kernel/core/hypervisor/src/virtual_platform.c:1010] |
| 0xFEE00000 - 0xFEE00FFF | 4 KB | Virtual Local APIC Register Window | EPT Violation -> MMIO trap | [VERIFIED-BY-CODE: kernel/core/hypervisor/src/virtual_platform.c:915] |

---

## 2. Hand-Built ACPI Subsystem & AML Tables

- [VERIFIED-BY-CODE: kernel/core/hypervisor/src/virtual_platform.c:110-325]
- **Tables Built**:
  1. **RSDP** (GPA 0xE0000): Signature 'RSD PTR ', OEM 'ATOMS ', Revision 2, Length 36 bytes. Checksum verified.
  2. **RSDT** (GPA 0xE0100) & **XSDT** (GPA 0xE0180): Pointers to FADT, MADT, DSDT.
  3. **FADT** (GPA 0xE0200): PM1a_EVT_BLK = 0x600, PM1a_CNT_BLK = 0x604, PM_TMR_BLK = 0x408. DSDT address pointing to 0xE0500.
  4. **MADT** (GPA 0xE0300):
     - Local APIC Address: 0xFEE00000.
     - Entry 0 (Type 0): Processor Local APIC ID 0, Flags = 1 (Enabled).
     - Entry 1 (Type 1): I/O APIC ID 1, Address = 0xFEC00000, GSI Base = 0.
  5. **DSDT** (GPA 0xE0500):
     - Static AML array (s_dsdt_aml) compiled via Intel ASL compiler (iasl).
     - Declares Scope (\_SB_):
       - Device (PCI0) [_HID PNP0A03]: _PRT routes Slot 1 (VirtIO Block) to GSI 10, Slots 2-4 to GSI 11.
       - Device (SYSR) [_HID PNP0C02]: Declares ACPI PM I/O ports (0x600, 0x408) so FreeBSD acpi_rman allocates them cleanly.
       - Device (KBD_) [_HID PNP0303]: Declares PS/2 Keyboard on ports 0x60, 0x64 with IRQ 1.
       - Device (COM1) [_HID PNP0501]: Declares UART COM1 on ports 0x3F8-0x3FF with IRQ 4.

---

## 3. Peripheral Device Emulation Audit

### 3.1 Virtual 8259 PIC (Dual Cascaded)
- [VERIFIED-BY-CODE: kernel/core/hypervisor/src/virtual_platform.c:605-665]
- **Ports**: 0x20/0x21 (Master), 0xA0/0xA1 (Slave).
- **Implemented**: ICW1, ICW2 (vector base tracking), ICW3, ICW4, and IMR (Interrupt Mask Register read/write).
- **Stubbed / Divergence**: OCW2 (EOI) acknowledges command but does not track in-service ISR priority rotation; OCW3 (read IRR/ISR) returns 0.

### 3.2 Virtual Local APIC (LAPIC)
- [VERIFIED-BY-CODE: kernel/core/hypervisor/src/virtual_platform.c:915-1010]
- **Base GPA**: 0xFEE00000.
- **Implemented Registers**:
  - Offset 0x20: LAPIC ID (returns 0).
  - Offset 0x30: LAPIC Version (returns 0x00050014 = Integrated APIC).
  - Offset 0x80: Task Priority Register (TPR).
  - Offset 0xB0: EOI (clears highest in-service ISR bit).
  - Offset 0xD0: Logical Destination Register (LDR).
  - Offset 0xE0: Destination Format Register (DFR).
  - Offset 0xF0: Spurious Interrupt Vector Register (SVR, bit 8 enables APIC).
  - Offset 0x100-0x170: In-Service Registers (ISR0-ISR7).
  - Offset 0x200-0x270: Interrupt Request Registers (IRR0-IRR7).
  - Offset 0x320: LVT Timer (Timer Mode, Mask bit 16, Vector).
  - Offset 0x380: Initial Count Register.
  - Offset 0x390: Current Count Register.
  - Offset 0x3E0: Divide Configuration Register.
- **MSR Mirror**:
  - MSR 0x838: IA32_TSC_DEADLINE emulated directly against host RDTSC [VERIFIED-BY-CODE: line 1375].

### 3.3 Virtual I/O APIC (IOAPIC)
- [VERIFIED-BY-CODE: kernel/core/hypervisor/src/virtual_platform.c:1010-1050]
- **Base GPA**: 0xFEC00000.
- **Implemented Registers**:
  - Offset 0x00: IOREGSEL (Index Register).
  - Offset 0x10: IOWIN (Data Window Register).
  - Register 0x00: IOAPIC ID (returns 1).
  - Register 0x01: IOAPIC Version (returns 0x00170020 = 24 redirection table entries).
  - Registers 0x10 to 0x3F: Redirection Table Entries 0 to 23 (64-bit: Vector, Delivery Mode, Polarity, Trigger Mode, Mask bit 16).

### 3.4 Virtual i8042 PS/2 Controller
- [VERIFIED-BY-CODE: kernel/core/hypervisor/src/virtual_platform.c:680-825, 1720-1970]
- **Ports**: 0x60 (Data), 0x64 (Status / Command).
- **Implemented Commands**:
  - 0x20: Read Command Byte (returns 0x47).
  - 0x60: Write Command Byte.
  - 0xAA: Self-test (returns 0x55).
  - 0xAB: Interface Test (returns 0x00).
  - 0xAE: Enable Keyboard.
  - 0xAD: Disable Keyboard.
  - 0xD4: Write to Mouse (returns 0xFA ACK).
  - 0xED: Set LEDs (acknowledges 0xFA, syncs host USB HID keyboard LEDs).
  - 0xF2: Read Keyboard ID (returns 0xFA, 0xAB, 0x83).
  - 0xF4: Enable Scanning (returns 0xFA).
  - 0xFF: Reset Keyboard (returns 0xFA, 0xAA).
- **Critical Architectural Flaw**:
  - Only ONE FIFO queue exists (platform->i8042.fifo).
  - Status bit 5 (0x20 = AUX / Mouse Output Buffer Full) is NEVER set.
  - Mouse ACK bytes (0xFA) and data bytes are placed into the KEYBOARD FIFO and trigger IRQ 1 instead of IRQ 12!

### 3.5 Virtual 16550 UART (COM1)
- [VERIFIED-BY-CODE: kernel/core/hypervisor/src/virtual_platform.c:325-475]
- **Ports**: 0x3F8 to 0x3FF.
- **Implemented**: RBR, THR, IER, IIR, FCR, LCR (DLAB divisor latch), MCR, LSR, MSR, SCR.
- **Transmission**: Guest writes to port 0x3F8 are forwarded to host physical COM1 serial port and buffered in memory log.
- **Reception**: virtual_uart_rx_push() buffers ASCII characters, asserts LSR bit 0 (Data Ready), and raises IRQ 4.

### 3.6 Reset Port 0xCF9 & Port 0xE9
- [VERIFIED-BY-CODE: kernel/core/hypervisor/src/hypervisor.c:1925-1940]
- **Port 0xCF9**: When guest writes value with bits 0x06 set (RST_CPU | SYS_RST), hypervisor stops vCPU with VMEXIT_GUEST_RESET. Automatic 3.0-second recovery timer or key 'R' reinitializes the VM without hard rebooting host hardware.
- **Port 0xE9**: Debug port. Guest writes are forwarded directly to host UDP telemetry network stream.

---

## 4. CPUID & MSR Emulation Audit

### 4.1 Faked / Synthetic Values
- **CPUID Leaf 0x00000001**:
  - EAX: 0x000306C3 (Intel Core i3 Haswell).
  - ECX: 0xF7DA3203 (Hypervisor present bit 31 set; x2APIC bit 21 explicitly cleared to force 32-bit MMIO APIC mode).
- **CPUID Leaf 0x40000000**:
  - EBX: 'ATOM', ECX: 'SSVM' (Hypervisor signature string).
- **MSR 0x1B (IA32_APIC_BASE)**:
  - Returns 0xFEE00900 (Base 0xFEE00000, BSP bit 8 set, APIC Global Enable bit 11 set).

### 4.2 Hardware Leaks (Pass-Through)
- **RDTSC / RDTSCP**: Passes through directly to real CPU TSC counter.
- **CR0 / CR4**: Guest modifications to WP, PG, PAE, OSFXSR, OSXMMEXCPT, FSGSBASE are committed directly to VMCS guest fields without host trap.

---

## 5. Top 10 Claims Least Sure About (Part 3)

1. Whether FreeBSD acpi_timer driver detects PM timer rollovers correctly at I/O port 0x408.
2. Whether PCI slot 1 GSI 10 in DSDT conflicts with real physical motherboard GSI 10 if shared.
3. If FreeBSD attempts to switch the IOAPIC to level-triggered active-low mode, whether virtual_platform handles polarity inversion.
4. Whether virtual 8254 PIT counter latches work correctly if guest reads port 0x40 twice without reading control word.
5. Exact behavior of RTC CMOS periodic interrupt (IRQ 8), which is not explicitly wired to IOAPIC redirection table.
6. Whether synthetic CPUID cache parameters (Leaf 4) mislead FreeBSD UMA allocator regarding L3 cache line sharing.
7. Whether guest writing to port 0x92 (Fast A20 / System Control) is trapped or silently falls through to physical host chipset.
8. If FreeBSD enables x2APIC via MSR 0x800 despite CPUID bit 21 being cleared, whether hypervisor properly injects #GP.
9. Whether virtual UART baud rate divisor changes affect real physical COM1 UART timing.
10. Behavior if FreeBSD guest attempts to configure PCI BARs above 0xFEBFFFFF in 64-bit PCI space.
