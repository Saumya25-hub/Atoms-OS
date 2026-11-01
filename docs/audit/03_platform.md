# AUDIT PART 3: PLATFORM & CHIPSET EMULATION
**Document**: `docs/audit/03_platform.md`  
**Classification**: READ-ONLY AUDIT - EVIDENCE-BASED  
**Date**: 2026-11-01  
**Scope**: Guest memory map, ACPI AML bytecode, PCI bus, LAPIC/IOAPIC, 8259 PIC, i8042, UART, and CPUID.

---

## 1. Guest Physical Address (GPA) Memory Layout

- [VERIFIED-BY-CODE: `kernel/core/hypervisor/src/freebsd_loader.c:40-95`, `virtual_platform.h:35-85`]
- **Total Guest RAM**: 2048 MB (`0x00000000` to `0x80000000`).

| GPA Range | Size | Allocation / Assignment | Mapped in EPT? |
|---|---|---|---|
| `0x00000000 - 0x00000FFF` | 4 KB | Real Mode IVT & BDA emulation window | Yes (RWX) |
| `0x00010000 - 0x00010FFF` | 4 KB | Guest Initial IDT (`FREEBSD_GUEST_IDT_GPA`) | Yes (RWX) |
| `0x00011000 - 0x00011FFF` | 4 KB | FreeBSD `bootinfo_t` struct (`FREEBSD_BOOTINFO_GPA`) | Yes (RWX) |
| `0x00012000 - 0x0001FFFF` | 56 KB | FreeBSD Preload Metadata Stream (`FREEBSD_METADATA_GPA`) | Yes (RWX) |
| `0x00020000 - 0x00025FFF` | 24 KB | Guest Page Tables: PML4 (`0x20000`), PDPT (`0x21000`), PD (`0x22000`) | Yes (RWX) |
| `0x000E0000 - 0x000FFFFF` | 128 KB | Virtual ACPI Table Region (RSDP, RSDT, XSDT, MADT, DSDT) | Yes (RWX) |
| `0x00200000 - 0x02000000` | ~30 MB | FreeBSD Kernel ELF Segments (`kernend` ~ `0x01EF3000`) | Yes (RWX) |
| `0x02000000 - 0x7FFFFFFF` | ~2014 MB | Free Usable Guest RAM (FreeBSD PMM pages) | Yes (RWX) |
| `0x80000000 - 0xFEBFFFFF` | ~2027 MB | Guest PCI MMIO Aperture (Declared in ACPI DSDT `_CRS`) | No (EPT Fault) |
| `0xFEC00000 - 0xFEC003FF` | 1 KB | Virtual IOAPIC Register Window (`VIRTUAL_IOAPIC_BASE_GPA`) | Trapped (EPT 0x30) |
| `0xFEE00000 - 0xFEE003FF` | 1 KB | Virtual LAPIC Register Window (`VIRTUAL_LAPIC_BASE_GPA`) | Trapped (EPT 0x30) |

---

## 2. ACPI Tables & Hand-Crafted AML Bytecode

- [VERIFIED-BY-CODE: `kernel/core/hypervisor/src/virtual_platform.c:110-250`]
- **Table Builder**: `virtual_platform_build_acpi_tables(platform)` populates the following structures:
  1. **RSDP** at `0xE0000`: Signature `"RSD PTR "`, OEM ID `"ATOMS "`, revision 2. Checksum calculated via `compute_checksum()`.
  2. **RSDT** at `0xE0100` and **XSDT** at `0xE0200`: Point to MADT and DSDT.
  3. **MADT (APIC)** at `0xE0300`: LAPIC physical base `0xFEE00000`. Declares vCPU 0 (Enabled), IOAPIC at `0xFEC00000` (GSI base 0), and dual 8259 PC-AT PIC flag.
  4. **DSDT** at `0xE0500` (`virtual_platform.c:139-161, 220-228`):
     - Table length and checksum dynamically computed at lines 220-228.
     - Contains compiled AML bytecode defining:
       - `_SB.PCI0`: PCI Host Bridge (`PNP0A03`) with `_PRT` routing Slot 1 Pin 0 to GSI 10 (VirtIO Block).
       - `_SB.SYSR`: System Board Resources (`PNP0C02`) reserving ACPI PM I/O ports `0x0600-0x0605` and PM Timer `0x0408-0x040B`.
       - `_SB.KBD_`: PS/2 Keyboard Controller (`PNP0303`) claiming ports `0x60`, `0x64`, and IRQ 1.
       - `_SB.COM1`: 16550A Serial Port (`PNP0501`) claiming ports `0x3F8-0x3FF` and IRQ 4.

---

## 3. Peripheral Device Emulation & Hardware Divergence

### 3.1 Virtual 8259 Dual-PIC
- [VERIFIED-BY-CODE: `kernel/core/hypervisor/src/virtual_platform.c:595-660`]
- Implements ICW1-ICW4 initialization sequence tracking on ports `0x20`, `0x21`, `0xA0`, `0xA1`.
- Tracks `master_imr` and `slave_imr`. `pic_mode` is active when master IMR != `0xFF` and LAPIC is disabled.
- **Divergence**: Fully functional for basic boot; edge/level triggered modes beyond default edge are stubbed.

### 3.2 Virtual LAPIC & IOAPIC
- [VERIFIED-BY-CODE: `kernel/core/hypervisor/src/virtual_platform.c:480-590, 890-980`]
- IOAPIC MMIO base `0xFEC00000`. Emulates `IOREGSEL` (`0x00`) and `IOWIN` (`0x10`).
- Supports 24 Redirection Table Entries (RTE 0 to 23).
  - RTE[1]: Keyboard IRQ 1 -> Vector 0x31.
  - RTE[4]: Serial COM1 IRQ 4 -> Vector 0x34.
  - RTE[10]: VirtIO-BLK IRQ 10 -> Vector 0x31 / 0x32.
- **Trap Mechanism**: Faulting GPA dispatched via EPT Violation (`ExitReason = 0x30` / Dec 48).

### 3.3 Virtual i8042 Keyboard Controller
- [VERIFIED-BY-CODE: `kernel/core/hypervisor/src/virtual_platform.c:665-885, 1720-1795`]
- **Port 0x64 (Command/Status)**:
  - Read: Returns status byte (`kbd->status`). Bit 0 (`0x01` OBF) set when FIFO has data.
  - Write: Handles `0x20` (Read Command Byte), `0x60` (Write Command Byte), `0xA7` (Disable Aux), `0xA8` (Enable Aux), `0xA9` (Test Aux), `0xAA` (Self-Test), `0xAB` (Interface Test), `0xD4` (Write to Mouse).
- **Port 0x60 (Data)**:
  - Read: Pops scancode byte from internal 16-byte FIFO via `virtual_i8042_pop_byte()`.
  - Write: Handles keyboard commands (`0xFF` Reset -> returns `0xFA` then `0xAA`; `0xF2` ID -> returns `0xFA`, `0xAB`, `0x83`).
- **Defect in Command `0xD4` Handling** (`virtual_platform.c:741-744`):
  - When guest sends mouse command via `0xD4`, line 743 pushes `0xFA` directly into the KEYBOARD buffer using `virtual_i8042_push_byte()`, asserting IRQ 1.
  - Virtual i8042 completely lacks an AUX FIFO queue and never asserts bit 5 (`0x20`) in status or IRQ 12.

### 3.4 Virtual 16550 UART (COM1 0x3F8)
- [VERIFIED-BY-CODE: `kernel/core/hypervisor/src/virtual_platform.c:356-385`]
- Emulates DLL, DLM, THR, RBR, IER, IIR, LCR, MCR, LSR, and MSR registers.
- Line 376-378 polls physical UART with timeout `uint32_t uart_to = 100000; while ((inb(0x3F8 + 5) & 0x20) == 0 && --uart_to);` before outputting, preventing CPU spinlock stalls.
- Mirrors single characters to physical COM1 and broadcasts lines via UDP 9995.

### 3.5 Port 0xCF9 Reset Interception
- [VERIFIED-BY-CODE: `kernel/core/hypervisor/src/hypervisor.c:1925-1936`]
- **Direct VMCS I/O Intercept**: Handled inside `hypervisor.c` during `EXIT_REASON_IO_INSTRUCTION` (`0x1E` / Dec 30).
- When guest writes byte with bits `(val & 0x06) != 0` to port `0xCF9`:
  - Logs: `[HYPERVISOR] Guest requested system reset via port 0xCF9. Re-initializing in 3s (or press 'R').`
  - Sets `vcpu->state = VM_STATE_STOPPED;`
  - Sets `vcpu->last_exit.disposition = VMEXIT_GUEST_RESET;`

### 3.6 CPUID Emulation & Passthrough
- [VERIFIED-BY-CODE: `kernel/core/hypervisor/src/virtual_platform.c:1220-1310`]
- Intercepts `EXIT_REASON_CPUID` (`0x0A` / Dec 10).
- **Leaf 0x1**: Returns synthetic Stepping, Model, Family (Haswell profile); reports hypervisor present (bit 31 of ECX = 1).
- **Leaf 0x40000000**: Returns hypervisor signature `"ATOMS OS VMM"`.

---

## 4. Top 10 Claims Least Sure About (Part 3)

1. Whether FreeBSD `psm` driver ever sends `0x60` to configure Command Byte bit 2 (System Flag) and fails if bit 2 is 0.
2. The exact response expected by FreeBSD when writing `0xF3` (Typematic Rate) if rate parameter byte is not explicitly ACKed.
3. If guest writes to IOAPIC `EOI` register (`0x40`), whether hypervisor correctly clears `TMR` bit for level-triggered VirtIO interrupts.
4. Whether virtual 8259 master/slave cascade handling operates if guest switches back from APIC to PIC mode during panic dump.
5. Exact behavior of port `0xCF8` / `0xCFC` when guest attempts 64-bit PCI MMIO BAR probing.
6. Whether the PNP0C02 `SYSR` AML device declaration completely satisfies FreeBSD `acpi_rman_io` under all BIOS revisions.
7. If FreeBSD boots with multiple vCPUs, how IOAPIC destination routing would handle APIC IDs > 0.
8. Whether virtual LAPIC spurious vector register (SVR) bit 8 (APIC Software Enable) properly gates virtual timer interrupts.
9. Latency impact of synchronous physical COM1 `outb(0x3F8, c)` inside guest UART write exit handler.
10. Whether FreeBSD `uart(4)` driver ever enables FIFO 64-byte mode (`FCR` bit 5) and misinterprets the 16-byte FIFO depth.
