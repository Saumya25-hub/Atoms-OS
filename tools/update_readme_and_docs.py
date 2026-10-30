# tools/update_readme_and_docs.py
"""
Script to update README.md and documentation with the FreeBSD 14.1 Interactive Keyboard & ACPI DSDT Milestone.
"""
import re

def main():
    with open("README.md", "r", encoding="utf-8") as f:
        lines = f.readlines()

    new_lines = []
    for line in lines:
        new_lines.append(line)
        # 1. Update Subsystems Status Table
        if "ASUS B750MK NVMe GPT NTFS CERTIFICATION" in line or ("NVMe Gen4 Storage" in line and "ASUS B750M-K" in line):
            new_lines.append('| **Type-1 Hypervisor** | **CERTIFIED** | [`kernel/core/hypervisor/`](file:///d:/Signatures_OS/kernel/core/hypervisor/) | Bare-metal Intel VT-x (VMX) Root Operation, EPT SLAT, 500k+ exits ([`ATOMS_VMX_HARDWARE_CERTIFICATION.md`](file:///d:/Signatures_OS/docs/hypervisor/ATOMS_VMX_HARDWARE_CERTIFICATION.md)) |\n')
            new_lines.append('| **FreeBSD Guest VM** | **CERTIFIED** | [`kernel/core/hypervisor/`](file:///d:/Signatures_OS/kernel/core/hypervisor/) | FreeBSD 14.1 Live Shell, VirtIO-Blk UFS2, ACPI DSDT (`PNP0303`/`PNP0501`), Interactive atkbd0 & USB LED Sync |\n')
        
        # 2. Update Certified Engineering Milestones
        if "| **Syscall Security** | ASUS B750M-K & QEMU UEFI" in line:
            new_lines.append('| **FreeBSD Guest Shell** | Intel H81 / i3-4130 | VirtIO-Blk root mount, UFS2 /sbin/init, Ring 3 sh (# ) | **PASS** | [`docs/hypervisor/ATOMS_TYPE1_VMX_WORKFLOW_ARCHITECTURE.md`](file:///d:/Signatures_OS/docs/hypervisor/ATOMS_TYPE1_VMX_WORKFLOW_ARCHITECTURE.md) |\n')
            new_lines.append('| **FreeBSD Interactive Kbd** | ASUS B760M-K / i3-14100F | ACPI DSDT PNP0303/0501, i8042 IRQ1, USB HID LED sync | **PASS** | [`docs/hypervisor/ATOMS_TYPE1_VMX_WORKFLOW_ARCHITECTURE.md`](file:///d:/Signatures_OS/docs/hypervisor/ATOMS_TYPE1_VMX_WORKFLOW_ARCHITECTURE.md) |\n')

        # 3. Update Recent Engineering Milestones
        if line.strip() == "## 28. Recent Engineering Milestones":
            new_lines.append("- **FreeBSD 14.1 Interactive Keyboard & ACPI DSDT Certification**: Engineered synthetic ACPI DSDT with `PNP0303` (PS/2 keyboard) and `PNP0501` (COM1 UART) compiled via Intel IASL, virtual i8042 PS/2 controller, VMCS interrupt window IRQ1 injection, and bidirectional USB HID LED state synchronization, achieving live interactive typing in FreeBSD 14.1 guest shell on physical Intel LGA1700 silicon.\n")

    content = "".join(new_lines)

    # 4. Insert dedicated Section 16 for Type-1 Micro-Hypervisor & FreeBSD 14.1 Guest Virtualization
    hypervisor_section = r"""
---

## 16. Hardware Type-1 Micro-Hypervisor & FreeBSD 14.1 Guest Virtualization

ATOMS OS includes a native, bare-metal **Type-1 Micro-Hypervisor** (`kernel/core/hypervisor/`) operating directly in Ring 0 VMX root operation on bare-metal silicon (Intel Core i3-14100F / LGA1700 and Haswell H81 / LGA1150) without any intermediate host operating system (no Linux, no KVM, no bhyve, no QEMU).

### 1. Architectural Foundation
- **VMX Root Executive**: Direct hardware VMCS lifecycle control via native x86_64 assembly (`__vmx_on`, `__vmx_clear`, `__vmx_ptrld`, `__vmx_launch`, `__vmx_resume`).
- **EPT Paging (SLAT)**: 4-level Extended Page Tables mapping guest physical addresses directly to host frames with write-back caching (`EPT_WB`).
- **Paravirtualized VirtIO Storage**: High-throughput VirtIO-Block device (`/dev/vtbd0`) backed by a zero-overhead UFS2 filesystem image.
- **Full Ring 0 to Ring 3 Execution**: Unmodified FreeBSD 14.1-RELEASE amd64 (GENERIC) kernel boots to `/sbin/init` and spawns `/bin/sh` single-user live root shell (`# `).

### 2. The Keyboard & Input Challenge: Root Cause Forensic Analysis
During bare-metal bring-up, FreeBSD booted cleanly to the `# ` shell prompt, but guest keystrokes were completely ignored. Forensic investigation identified:
1. **Suppression of Legacy ISA Probing under ACPI**: FreeBSD amd64 boots in ACPI mode (`acpi0` attached to `nexus0`). In pure ACPI mode, FreeBSD suppresses legacy ISA hardware probing unless enumerated by ACPI or attached to a child ISA bus.
2. **Missing `isa0` Bus**: An `isa0` bus was never created because our micro-hypervisor PCI bus lacked a PCI-to-ISA bridge (`class 06-01`, `isab_pci`) and the synthetic DSDT lacked an ACPI ISA bus node (`PNP0A00`). As a result, static ISA hints (`hint.atkbdc.0.at="isa"`, `hint.uart.0.at="isa"`) had no bus to attach to.
3. **Missing DSDT HID Nodes**: FreeBSD's `atkbdc_acpi` driver specifically requires ACPI device nodes with `_HID = "PNP0303"` or `"PNP030B"`, and `uart_acpi` looks for `_HID = "PNP0501"`. Because the synthetic DSDT only defined `\_SB.PCI0` and `\_SB.SYSR`, FreeBSD never instantiated `atkbdc0`, `atkbd0`, or `uart0`.
4. **IOAPIC Vector Traps**: Initial IRQ fallback logic attempted to inject vector `0x21` before the guest unmasked IOAPIC RTE[1], triggering FreeBSD kernel "Fatal trap 30: reserved (unknown) fault".
5. **Lock LED Disconnect**: Local host USB HID lock LED toggling bypassed the guest, and bit encodings differed between PS/2 (`0xED`: bit 0 Scroll, bit 1 Num, bit 2 Caps) and USB HID (`0x01` Num, `0x02` Caps, `0x04` Scroll).

### 3. Engineering Solution & Implementation
1. **Synthetic ACPI DSDT Device Nodes**: Dynamically generated and validated with Intel IASL compiler (zero errors, zero warnings). Injected into guest ACPI namespace under `\_SB`:
   - `Device (KBD)`: `_HID = EisaId("PNP0303")`, `_STA = 0x0F`, IO ports `0x60`/`0x64`, `IRQNoFlags() { 1 }`.
   - `Device (COM1)`: `_HID = EisaId("PNP0501")`, `_STA = 0x0F`, IO ports `0x3F8`-`0x3FF`, `IRQNoFlags() { 4 }`.
2. **Virtual i8042 PS/2 Controller (`kernel/core/hypervisor/src/virtual_platform.c`)**:
   - Implements full i8042 state machine on ports 0x60/0x64 with 64-byte circular FIFO, status bit 0 (`OBF`), and controller commands (`0x20`, `0x60`, `0xAA`, `0xAB`, `0xAD`, `0xAE`, `0xD0`, `0xD1`, `0xD4`).
   - Converts host USB HID input into Scancode Set 1 and Set 2 with make/break handling.
   - Clean IRQ1 injection via VMCS interrupt windows only after the guest programs IOAPIC RTE[1] or 8259 PIC ICW2 base vector.
3. **Bi-Directional USB HID Lock LED Synchronization**:
   - Captures guest `0xED` command arguments, translates PS/2 LED bits to USB HID Boot Keyboard report format, and updates physical keyboard LEDs.
   - Saves and restores host and guest lock LED states on focus transitions (`Ctrl+Alt+G`).
4. **Host/Guest Focus Router**:
   - Unified state machine with hardware release chord `Ctrl+Alt+G`.
   - Dedicated dashboard telemetry line displaying active focus domain, FIFO depth, and IRQ injection counters.

### 4. Bare-Metal Hardware Proof
- **Hardware Testbed**: ASUS PRIME B760M-K (Intel Core i3-14100F LGA1700) and Intel H81 (Core i3-4130 LGA1150).
- **Interrupt Handler Verification**: `vmstat -i` inside FreeBSD shell confirms active interrupt servicing:
  ```text
  interrupt               total        rate
  irq1: atkbd0               14           0
  irq10: virtio_pci0        913           2
  cpu0:timer               2840           7
  Total                    3767           9
  ```
- **Live Shell Keystroke Verification**:
  ```text
  # echo hellow
  hellow
  # echo dishant like shock mt hona
  dishant like shock mt hona
  ```
"""

    # Insert Section 16 before Section 16. Debug & Forensic Engineering
    target_section = "\n## 16. Debug & Forensic Engineering"
    assert target_section in content, "Target section not found"
    parts = content.split(target_section, 1)

    # Renumber strictly ## 16..29 to ## 17..30 at the start of a line
    remainder = target_section + parts[1]
    def renumber(match):
        num = int(match.group(1))
        return f"## {num + 1}."

    remainder = re.sub(r"(?m)^## (\d+)\.", renumber, remainder)

    final_content = parts[0] + hypervisor_section + remainder

    with open("README.md", "w", encoding="utf-8", newline="\r\n") as f:
        f.write(final_content)

    print("README.md updated with full Hypervisor Section and renumbered cleanly!")

if __name__ == "__main__":
    main()
