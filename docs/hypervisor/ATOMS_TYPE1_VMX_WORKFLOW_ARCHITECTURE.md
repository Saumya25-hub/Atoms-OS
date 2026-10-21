# ATOMS OS — Type-1 Bare-Metal VMX Hypervisor: Complete Architecture & Workflow Guide

Copyright © 2026 ATOMS OS Project / Saumya Chaudhari  
**Target Hardware**: Intel Haswell LGA1150 (H81 Chipset), Intel Core i3 4th Gen, 8 GB RAM, Native UEFI Mode  
**Guest Operating System**: FreeBSD 14.1-RELEASE amd64 (GENERIC Kernel)  
**Certification Status**: **100% PASS — LIVE INTERACTIVE USERSPACE SHELL (`# `) ACHIEVED ON BARE METAL**

---

## 1. Executive Summary

ATOMS OS features an in-tree, bare-metal **Type-1 Micro-Hypervisor** implemented from scratch without third-party virtualization frameworks (no KVM, no bhyve, no QEMU dependencies on host). 

On October 21, 2026, the ATOMS Hypervisor achieved formal **Bare-Metal End-to-End Milestone Certification** on physical Intel Haswell H81 hardware, booting FreeBSD 14.1-RELEASE from kernel entry through hardware discovery, APIC timer calibration, VirtIO-PCI block device attachment, authentic UFS2 root filesystem mounting, and Ring 0 ➔ Ring 3 userspace execution of `/sbin/init`, successfully spawning an interactive single-user root shell (`# `).

```
+-----------------------------------------------------------------------------------+
|                        PHYSICAL HARDWARE (Haswell LGA1150)                        |
|                                                                                   |
|  +-----------------------------------------------------------------------------+  |
|  |                     ATOMS OS BARE-METAL HOST KERNEL                         |  |
|  |   - VMXON Root Mode Host Executive         - Realtek R8168 GbE LAN Debug    |  |
|  |   - 4-Level EPT Memory Virtualization       - Cooperative Telemetry Loop     |  |
|  |   - Synthetic APIC / LAPIC Timer Engine     - VirtIO-PCI Emulation Engine    |  |
|  +-----------------------------------------------------------------------------+  |
|                                         |                                         |
|                                   VM-Entry / VM-Exit                              |
|                                         v                                         |
|  +-----------------------------------------------------------------------------+  |
|  |                    GUEST: FREEBSD 14.1-RELEASE (x86_64)                     |  |
|  |                                                                             |  |
|  |   [RING 0] FreeBSD amd64 GENERIC Kernel                                     |  |
|  |     - CPUID / GDT / IDT / TSS Configuration                                  |  |
|  |     - LAPIC TSC-Deadline Timer Calibration                                  |  |
|  |     - VirtIO-PCI Driver (`virtio_pci0`) ➔ VirtIO-Blk (`vtbd0`)               |  |
|  |     - Root Mount: `ufs:/dev/vtbd0` (Authentic UFS2 Filesystem)              |  |
|  |                                                                             |  |
|  |   [RING 3] Userspace Execution Environment                                  |  |
|  |     - Process 1: `/sbin/init` (Launched via `kern_execve`)                  |  |
|  |     - Process 2: `/bin/sh` (Single-User Interactive Root Shell: `# `)       |  |
|  +-----------------------------------------------------------------------------+  |
+-----------------------------------------------------------------------------------+
```

---

## 2. VMX Lifecycle & Hardware Foundation

The hypervisor manages Intel VT-x hardware via the following strictly sequenced stages:

### 2.1 CPU Capability Detection & MSR Locks
1. **CPUID Verification**:
   - Executes `cpuid(1)` to verify `ECX.VMX` (Bit 5 = 1).
2. **`IA32_FEATURE_CONTROL` (MSR 0x3A)**:
   - Checks Bit 0 (Lock bit).
   - If unlocked, programs `IA32_FEATURE_CONTROL_LOCK | IA32_FEATURE_CONTROL_VMXON_OUTSIDE_SMX` (0x05) via `wrmsr`.
3. **CR0 / CR4 Fixed Bits**:
   - Programs `CR0` to satisfy `IA32_VMX_CR0_FIXED0` and `IA32_VMX_CR0_FIXED1` (enabling PE, PG, NE).
   - Programs `CR4` to satisfy `IA32_VMX_CR4_FIXED0` and `IA32_VMX_CR4_FIXED1` (enabling `CR4.VMXE` Bit 13).

### 2.2 VMXON Root Mode
- Allocates a 4 KB page aligned to 4 KB for the host **VMXON region**.
- Writes the 32-bit VMCS Revision Identifier (read from `IA32_VMX_BASIC` MSR 0x480) into the first 4 bytes of the VMXON page.
- Executes `vmxon [vmxon_phys]` to enter VMX Root Operation.

### 2.3 VMCS (Virtual Machine Control Structure)
- Allocates a 4 KB page aligned to 4 KB for the guest **VMCS region**.
- Writes the VMCS Revision ID into the first 4 bytes.
- Executes `vmclear [vmcs_phys]` to initialize the VMCS state machine.
- Executes `vmptrld [vmcs_phys]` to make it the active VMCS on the current logical processor.

---

## 3. VMCS Execution Controls Architecture

The hypervisor configures the VMCS control fields based on the true capability MSRs (`IA32_VMX_TRUE_PINBASED_CTLS`, `IA32_VMX_TRUE_PROCBASED_CTLS`, etc.):

| Control Field | Value / Configuration | Purpose |
| :--- | :--- | :--- |
| **Pin-Based Controls** | `EXTERNAL_INTR_EXITING` (Bit 0) | Host physical interrupts (e.g. NIC, PIT) intercept to host to maintain host telemetry and LAN polling. |
| **Primary Processor Controls** | `HLT_EXITING` (Bit 7), `UNCONDITIONAL_IO_EXITING` (Bit 24), `SECONDARY_CONTROLS` (Bit 31) | Intercepts `HLT` to keep CPU cool, intercepts all port I/O for virtual platform emulation (VirtIO, PIC, UART). |
| **Secondary Processor Controls** | `ENABLE_EPT` (Bit 1), `ENABLE_RDTSCP` (Bit 3), `ENABLE_VPID` (Bit 5), `UNRESTRICTED_GUEST` (Bit 7), `ENABLE_INVPCID` (Bit 12) | Hardware-accelerated memory virtualization via EPT; unencumbered real/protected/long mode execution. |
| **VM-Exit Controls** | `HOST_ADDR_SPACE_SIZE` (Bit 9 = 1) | Forces host return in 64-bit Long Mode. |
| **VM-Entry Controls** | `IA32E_MODE_GUEST` (Bit 9 = 1) | Launches guest directly in Long Mode (64-bit). |
| **Exception Bitmap** | `0x00000000` | Exceptions (#PF, #GP, #UD, #DB) delivered directly to guest IDT for full native OS performance. |

---

## 4. EPT (Extended Page Tables) 4-Level Paging

Memory virtualization is implemented via 4-level Extended Page Tables:

```
Guest Physical Address (GPA)
   [47:39] PML4 Index  ──>  EPT PML4 Table
   [38:30] PDPT Index  ──>  EPT Page Directory Pointer Table
   [29:21] PD Index    ──>  EPT Page Directory
   [20:12] PT Index    ──>  EPT Page Table
   [11:00] Offset      ──>  Host Physical Address (HPA)
```

- **Paging Setup**:
  - Full physical RAM (up to 4 GB / 8 GB) identity-mapped with Read/Write/Execute permissions (`0x07`).
  - Cache type configured as Write-Back (WB = 0x06) in EPT leaf entries.
  - EPTP programmed into VMCS with: `EPT_PML4_PHYS | 0x1E` (4-Level Walk, WB Cache).
- **MMIO Interception**:
  - Physical ranges for LAPIC (`0xFEE00000`), IOAPIC (`0xFEC00000`), and PCI apertures (`0xFEB00000`) are monitored.
  - Accesses outside RAM trigger `VMX_EXIT_REASON_EPT_VIOLATION` (Exit Reason 48) and are routed to `virtual_platform_handle_mmio()`.

---

## 5. Synthetic FreeBSD Bootloader & Kernel Handoff

Instead of running a heavy legacy BIOS or slow secondary UEFI bootloader inside the guest, ATOMS implements a **zero-overhead long-mode direct kernel loader** ([kernel/core/hypervisor/src/freebsd_loader.c](file:///d:/Signatures_OS/kernel/core/hypervisor/src/freebsd_loader.c)):

1. **Kernel ELF Parsing**:
   - Parses the embedded 64-bit ELF kernel binary (`kernel.bin`).
   - Copies `PT_LOAD` segments into Guest Physical Memory at `0x00200000` (physical) / `0xFFFFFFFF80200000` (virtual).
2. **Page Tables Construction**:
   - Builds a 4-level PML4/PDPT/PD hierarchy in guest memory:
     - Identity maps low 4 GB (`0x0000000000000000 - 0x00000000FFFFFFFF`).
     - Maps higher-half kernel space: `0xFFFFFFFF80000000` maps to physical `0x00000000`.
     - Maps direct physical map (`DMAP`): `0xFFFF800000000000` maps to physical `0x00000000`.
3. **Boot Metadata Injection**:
   - Populates `struct bootinfo`:
     - `bi_version`: 1
     - `bi_kernelname`: `/boot/kernel/kernel`
     - `bi_modulep`: Metadata module linked list containing:
       - `MODINFO_NAME`: `kernel`
       - `MODINFO_TYPE`: `elf kernel`
       - `MODINFO_METADATA | MODINFOMD_HOWTO`: `RB_VERBOSE | RB_SINGLE`
       - `MODINFO_METADATA | MODINFOMD_ENVP`: Static loader environment (`kern.init_path=/sbin/init`, `vfs.root.mountfrom=ufs:/dev/vtbd0`)
4. **vCPU Initial State**:
   - `RIP`: FreeBSD kernel entry point `btext` (`0xFFFFFFFF80200000`).
   - `RDI`: Pointer to `modulep` metadata block in low memory.
   - `RSP`: Pre-allocated initial boot stack (`0x0007E000`).
   - `CR0`: `0x80010033` (PE, MP, ET, NE, WP, PG).
   - `CR3`: Guest PML4 base (`0x00070000`).
   - `CR4`: `0x000006B8` (DE, PSE, PAE, PGE, OSFXSR, OSXMMEXCPT, FSGSBASE).
   - `EFER`: `0x00000D01` (SCE, LME, LMA, NXE).

---

## 6. Virtual Platform & APIC Timer Engine

Modern operating systems (FreeBSD, Linux) require precise interrupt delivery and timers to calibrate schedulers and time-of-day clocks:

### 6.1 Guest LAPIC & TSC-Deadline Timer
- **MSR `0x6E0` (IA32_TSC_DEADLINE)**:
  - FreeBSD calibrates kernel clocks using the TSC-Deadline timer.
  - When the guest writes an expiration target TSC to MSR `0x6E0`, the hypervisor arms the virtual timer.
  - On every VM-Exit (and host timer interrupt), the hypervisor executes `atoms_hypervisor_try_inject_lapic_timer()`:
    ```c
    uint64_t current_tsc = __rdtsc();
    if (current_tsc >= vcpu->tsc_deadline && vcpu->tsc_deadline != 0) {
        // Inject vector (typically 0xEF / 239) into VMCS
        vmx_vmwrite(VMCS_VM_ENTRY_INTR_INFO_FIELD, vector | INTR_TYPE_EXT_INTR | INTR_VALID);
        vcpu->tsc_deadline = 0; // Disarm
    }
    ```
- **x2APIC Mode (MSRs 0x800 - 0x8FF)**:
  - Supports x2APIC MSR reads/writes.
  - Intercepts MSR `0x80B` (x2APIC EOI) and MSR `0x832` (x2APIC Self-IPI).
  - Calling EOI clears the In-Service Register (ISR) bit to permit subsequent timer ticks.

### 6.2 IOAPIC & Interrupt Routing
- Provides a 24-pin virtual IOAPIC at physical address `0xFEC00000`.
- Routes legacy IRQ 10 / IRQ 11 (used by VirtIO-PCI) to LAPIC interrupt vector `0x31` (49).

---

## 7. VirtIO-PCI Storage Engine (`/dev/vtbd0`)

Root disk backing is implemented as a standard VirtIO 0.9.5 Legacy Block Device:

```
[VirtIO-PCI Device (PCI 0:3:0)]
  │
  ├── BAR0: Legacy I/O Ports (0xC000 - 0xC03F)
  │     ├── 0x00: Device Features (VIRTIO_BLK_F_SIZE_MAX | VIRTIO_BLK_F_SEG_MAX)
  │     ├── 0x04: Guest Features
  │     ├── 0x08: Queue Address (Page Frame Number of Virtqueue)
  │     ├── 0x0C: Queue Size (128 Descriptors)
  │     ├── 0x0E: Queue Select (Queue 0 = Request Queue)
  │     ├── 0x10: Queue Notify (Doorbell port)
  │     └── 0x12: Device Status (ACK ➔ DRIVER ➔ DRIVER_OK)
  │
  └── Virtqueue Split Ring (in Guest Memory):
        ├── Descriptor Table [128 entries]
        │     - Desc 0: Request Header (`type`, `sector`)
        │     - Desc 1: Data Buffer (Host RAM Disk transfer via EPT translation)
        │     - Desc 2: Status Byte (0x00 = VIRTIO_BLK_S_OK)
        ├── Available Ring: Guest announces queued requests
        └── Used Ring: Hypervisor marks completed requests and fires IRQ
```

- When FreeBSD writes to Queue Notify (`0xC010`), the hypervisor:
  1. Parses the available ring descriptors.
  2. Resolves Guest Physical Addresses to Host addresses.
  3. Copies disk sectors from the in-memory `media.img` storage pool.
  4. Writes `VIRTIO_BLK_S_OK` to the status byte.
  5. Updates the used ring and asserts IRQ 10 via the virtual IOAPIC.

---

## 8. UFS2 Minimal Rootfs Layout & Fix

FreeBSD mounts its root filesystem using the Berkeley Fast File System (UFS2). The geometry is strictly structured in units of 32 KB blocks:

- **Superblock**: Offset 64 KB (`0x10000`), magic `0x19540119`.
- **Cylinder Group 0**: Offset 128 KB (`0x20000`), magic `0x090255`.
- **Inode Table**: Offset 160 KB (`0x28000`), 2048 inodes of 256 bytes.
- **Data Blocks**:
  - `fs_bsize` = 32,768 bytes (32 KB).
  - `fs_fsize` = 4,096 bytes (4 KB).
  - `fs_frag`  = 8 fragments per block.
- **Block Addressing Rule**:
  - Direct blocks `di_db[0..11]` point to fragments separated by **8** (`file_frag_start + lbn * 8`).
  - Files larger than 384 KB use single indirect block `di_ib[0]`, where each 64-bit pointer points to a 32 KB block (`file_frag_start + lbn * 8`).
  - Directory tables are formatted in 512-byte `DIRBLKSIZ` records with `reclen` alignments.

---

## 9. Ring 0 to Ring 3 Userspace Shell Execution

Once the FreeBSD kernel mounts the root filesystem:

1. Kernel spawns `start_init` kernel thread.
2. `start_init` calls `kern_execve("/sbin/init")`.
3. `imgact_elf` loads `/sbin/init` (statically linked ELF binary).
4. Kernel executes `doreti_iret` (IRETQ) loading:
   - `CS`: User Code Selector (`0x43`, RPL=3)
   - `SS`: User Data Selector (`0x3B`, RPL=3)
   - `RFLAGS`: `0x202` (IF enabled)
   - `RSP`: Userspace top of stack
5. CPU executes in **Guest Ring 3 (CPL=3)**.
6. `/sbin/init` parses `/etc/ttys`, forks, and launches `/bin/sh`.
7. `/bin/sh` detects standard input on console and presents the prompt:
   ```text
   Cannot read termcap database;
   using dumb terminal settings.
   # █
   ```

---

## 10. Verification Checklist

| Checkpoint | Forensic Metric | Verdict |
| :--- | :--- | :---: |
| **UEFI PXE Boot** | Bootloader downloaded and executed cleanly from host TFTP | **PASS** |
| **VMX Host State** | VMXON and VMCS loaded with zero instruction errors | **PASS** |
| **EPT 4-Level Map** | Identity map up to 8 GB physical memory cleanly traversed | **PASS** |
| **Guest Kernel Entry** | Direct handoff into FreeBSD kernel entry point `btext` | **PASS** |
| **Guest Timer** | TSC-Deadline MSR 0x6E0 periodic interrupt injection | **PASS** |
| **VirtIO-PCI & Disk** | Storage controller recognized as `vtbd0` (16 MB) | **PASS** |
| **Rootfs Mount** | Root mounted from `ufs:/dev/vtbd0` | **PASS** |
| **Userspace Shell** | Single-user root shell (`# `) running on physical screen | **PASS** |
