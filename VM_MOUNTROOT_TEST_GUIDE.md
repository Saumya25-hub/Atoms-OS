# ATOMS OS — VM Mountroot Test Guide

## 1. Overview & Objective
This guide documents the deterministic VM testing environment for reproducing and diagnosing the ATOMS OS FreeBSD guest `VirtIO-BLK` / `mountroot` execution boundary inside a local virtualized sandbox.

**Core Diagnostic Question:**
> Does the FreeBSD mountroot READ request reach the ATOMS VirtIO-BLK backend inside the VM?
> - If **YES**: print the exact request (sector, lengths, buffer chain) and trace completion.
> - If **NO**: identify the exact queue/notification boundary where it disappears.

---

## 2. Architecture & Pipeline
The VM test environment reproduces the complete production ATOMS hardware virtualization stack:

```mermaid
graph TD
    A[ATOMS BOS Kernel] --> B[Micro-Hypervisor Core]
    B --> C[Virtual Platform & Synthetic PCI Bus]
    C --> D[VirtIO-BLK Emulated Controller (ID 0x1001 / Vendor 0x1AF4)]
    D --> E[FreeBSD 14.1 Guest (virtio_pci -> vtbd0)]
    E --> F[vfs_mountroot: ufs:/dev/vtbd0]
    F --> G[ATOMS VirtIO-BLK Request Processing]
```

---

## 3. Exact Command to Run the VM

### 3.1 PowerShell Automated Test Harness
To run the automated build, image generation, QEMU UEFI boot, and live serial log streaming:

```powershell
powershell -ExecutionPolicy Bypass -File tools\qemu\run_freebsd_mountroot_test.ps1
```

**Parameters available:**
- `-SkipBuild`: Skip the compilation step and reuse existing `build/atoms_uefi_test.img`.
- `-TimeoutSeconds <int>`: Maximum duration before test halts (default: `90`).
- `-LogFile <path>`: Custom destination path for timestamped serial output.

### 3.2 Raw QEMU Execution Command
If launching QEMU directly via command line:

```powershell
& "D:\OS-QEMU-EMU\qemu\qemu-system-x86_64.exe" `
    -machine q35 `
    -cpu max,vmx=on `
    -m 4096M `
    -drive if=pflash,format=raw,readonly=on,file="D:\OS-QEMU-EMU\qemu\share\edk2-x86_64-code.fd" `
    -drive file="D:\Signatures_OS\build\atoms_uefi_test.img",format=raw `
    -device qemu-xhci `
    -device usb-kbd `
    -device usb-mouse `
    -netdev user,id=net0 `
    -device e1000,netdev=net0 `
    -serial file:"D:\Signatures_OS\build\freebsd_mountroot_test.log" `
    -display none `
    -no-reboot
```

---

## 4. Guest Configuration & Disk Images

| Component | Specification |
|:---|:---|
| **Host VM Machine** | `q35` (Intel ICH9 PCIe Chipset emulation) |
| **Firmware** | EDK2 / OVMF Pure UEFI (`edk2-x86_64-code.fd`) |
| **CPU Model** | `max,vmx=on` (4 Cores, 64-bit Long Mode) |
| **Guest Physical RAM** | 4096 MB (`-m 4096M`) |
| **Primary ESP Disk** | `build/atoms_uefi_test.img` (512 MB GPT FAT32 ESP) |
| **Root Disk Backing** | Preloaded `MEDIA.IMG` (96 MB UFS2 image) / Dynamic 4 GB Sparse RAM Disk |
| **FreeBSD Guest Kernel** | `tools/freebsd_payload/freebsd_stripped_kernel.elf` (FreeBSD 14.1-RELEASE ELF64) |
| **Console Output** | COM1 UART (`0x3F8`) mirrored to host serial log |

---

## 5. Serial Log Location
- **Default automated path:** `build/freebsd_mountroot_test_<YYYYMMDD_HHMMSS>.log`
- **Fixed test alias path:** `build/freebsd_hypervisor_serial.log`

---

## 6. VirtIO Forensic Tracing Mode

### 6.1 How to Enable / Disable
Targeted VirtIO-BLK tracing is controlled via the header define in [`kernel/core/hypervisor/include/virtio_blk.h`](file:///D:/Signatures_OS/kernel/core/hypervisor/include/virtio_blk.h):

```c
/* Enable Forensic Tracing (1 = Enabled, 0 = Disabled) */
#define VIRTIO_BLK_FORENSIC_TRACE 1
```

Rebuild kernel using:
```powershell
powershell -ExecutionPolicy Bypass -File build.ps1
```

### 6.2 Forensic Trace Points Emitted
When enabled, the hypervisor emits detailed COM1 events for all 12 VirtIO-BLK lifecycle phases:

1. `[VIRTIO-BLK TRACE] BAR0 QUEUE_NOTIFY Write`: Emitted upon guest write to BAR0 offset `0x10`.
2. `[VIRTIO-BLK TRACE] >>> QUEUE_NOTIFY Callback Entered! Queue Index=0`: Emitted when `blk_queue_notify_cb` executes.
3. `[VIRTIO-BLK TRACE] avail->idx`: Emitted in `virtio_blk_process_queue` comparing `avail->idx` against `last_avail_idx`.
4. `[VIRTIO-BLK TRACE] Descriptor Head`: Emitted showing popped descriptor ring index.
5. `[VIRTIO-BLK TRACE] Request Type`: Distinguishes `VIRTIO_BLK_T_IN` (READ), `VIRTIO_BLK_T_OUT` (WRITE), `VIRTIO_BLK_T_FLUSH`, etc.
6. `[VIRTIO-BLK TRACE] Sector / LBA`: Shows 64-bit target block LBA.
7. `[VIRTIO-BLK TRACE] Descriptor Lengths`: Prints buffer count, GPA, lengths, and `GUEST_READ` / `GUEST_WRITE` flags.
8. `[VIRTIO-BLK TRACE] Backend READ`: Confirms sector transfer from storage backing buffer (`SUCCESS` or `FAIL`).
9. `[VIRTIO-BLK TRACE] Status Byte`: Records completion status (`0 = VIRTIO_BLK_S_OK`).
10. `[VIRTIO-BLK TRACE] used->idx`: Shows updated used ring index in guest memory.
11. `[VIRTIO-BLK TRACE] ISR Status`: Shows `dev->isr_status` transitions and guest read-to-clear events.
12. `[VIRTIO-BLK TRACE] Interrupt Injection Gate`: Confirms `virtio_irq_pending`, IOAPIC RTE 11 vector, and VMCS injection.

---

## 7. Pass / Fail Criteria

| Verification Gate | Expected Outcome | Pass Criteria |
|:---|:---|:---|
| **VM Boot Result** | ATOMS BOS boots cleanly under UEFI firmware | Stage 0–27 completed, persistent runtime reached |
| **PCI / Device Probe** | FreeBSD attaches `vtpci0` and `vtbd0` | FreeBSD device tree registers `/dev/vtbd0` |
| **Mountroot Reached** | FreeBSD executes `vfs_mountroot()` | Emits `Trying to mount root from ufs:/dev/vtbd0` |
| **First READ Received** | ATOMS backend receives `QUEUE_NOTIFY` | `QUEUE_NOTIFY Write: Queue Index=0` logged |
| **First Failing Boundary** | Root cause boundary pinpointed | Exact component where sequence stops is identified |
