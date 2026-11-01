# ATOMS OS Audit - Part 8: Risk Register & Safe Remediation Plan
**Document**: `docs/audit/08_risks.md`  
**Classification**: READ-ONLY AUDIT - EVIDENCE-BASED  
**Date**: 2026-11-01  
**Scope**: Risk ranking (Top 20), impact classification, smallest safe remediation, VMX-critical flagging, sequential staging plan, and uncertainties.

---

## 1. Top 20 Architectural & Operational Risks

| Rank | Subsystem | Risk Description | Evidence Reference | Evidence Label | Impact Class | Smallest Safe Fix | VMX Critical? |
|---|---|---|---|---|---|---|---|
| **1** | USB / HID | Mouse displacement bytes misclassified as keyboard reports and sent to guest console as keystrokes | `usb_hid.c:280, 603`, `virtual_platform.c:1740` | `VERIFIED-BY-CODE`, `VERIFIED-ON-HARDWARE` | Functional / Usability | Check interface descriptor protocol; never route mouse HID data to keyboard buffer | **NO** |
| **2** | Guest Kernel | Embedded kernel is FreeBSD 14.1-RELEASE (`1401000`) while rootfs userland is FreeBSD 14.5 (`1405000`) | `kernel.c:75`, `BOOTX64.EFI`, AMDE telemetry `#17` | `VERIFIED-BY-CODE`, `VERIFIED-ON-HARDWARE` | Stability / Compatibility | Replace embedded kernel with 14.5 kernel from USB image or build 14.5 matching payload | **NO** |
| **3** | VirtIO / USB | Synchronous 5000 ms blocking wait in VMCS exit handler on USB MSC sector read | `usb_msc.c:125`, `virtio_blk.c:175` | `VERIFIED-BY-CODE` | Latency / Host Freeze | Clamp USB timeout to 150 ms; implement non-blocking split-phase VirtIO request completion | **YES** |
| **4** | i8042 / Input | Virtual i8042 lacks PS/2 AUX status bit 5 (`0x20`) and IRQ 12 emulation | `virtual_platform.c:1757-1790` | `VERIFIED-BY-CODE` | Functional / Missing Hardware | Implement AUX data queue, set status bit 5 on mouse byte, assert IRQ 12 | **NO** |
| **5** | VMX Core | Host GDT base and TR base leaked directly into guest VMCS fields | `hypervisor.c:3232, 3234` | `VERIFIED-BY-CODE` | Security / Hypervisor Isolation | Allocate separate guest GDT/TSS pages in GPA space and load guest bases | **YES** |
| **6** | VMX Core | Unhandled VM-exit causes silent guest halt or infinite loop without diagnostic dump | `hypervisor.c:3547` | `VERIFIED-BY-CODE` | Reliability / Debuggability | Send emergency UDP packet with full register dump (`RIP`, `RSP`, `CR0`, `CR3`, `ExitReason`) and halt cleanly | **YES** |
| **7** | PCI / VirtIO | Dead VirtIO-Input device (`00:03.0`) registered on PCI bus with zero functional callers | `virtio_input.c:45`, `virtual_platform.c:116` | `VERIFIED-BY-CODE`, `VERIFIED-ON-HARDWARE` | Bloat / Driver Confusion | Either hook `virtio_input_send_key/mouse` to host HID or remove device from PCI table | **NO** |
| **8** | Hypervisor | Single-threaded cooperative execution loop on Core 0 starves host background tasks | `hypervisor.c:3710` | `VERIFIED-BY-CODE` | Latency / Starvation | Enforce periodic preemption timer exit or yield to host polling after N exits | **YES** |
| **9** | Platform | Port 0xCF9 guest reset puts VM into `VM_STATE_STOPPED` and floods UDP with duplicate logs | `virtual_platform.c:1865` | `VERIFIED-BY-CODE`, `VERIFIED-BY-LOG` | Network / Telemetry Flood | Rate-limit STOPPED log to 1 per 5s; add auto-restart countdown or manual 'R' key trigger | **NO** |
| **10** | VMX Core | Priority interrupt injection drops/defers VirtIO and UART IRQs under heavy timer load | `hypervisor.c:2860-2895` | `VERIFIED-BY-CODE` | I/O Throughput / Latency | Implement round-robin servicing for secondary non-timer interrupt vectors | **YES** |
| **11** | VirtIO / VFS | VirtIO-BLK read-only mode returns `VIRTIO_BLK_S_IOERR` on writes, risking guest panic | `virtio_blk.c:220` | `VERIFIED-BY-CODE` | Stability / Guest Panic | Ensure FreeBSD mounts root as read-only (`ro`) or add RAM-backed write overlay | **NO** |
| **12** | Storage / NTFS | Fixed-size runlist extents array (`MAX_EXTENTS = 128`) risks truncation on fragmented images | `runtime_storage.c:512` | `VERIFIED-BY-CODE` | Data Integrity | Dynamically allocate extent table or fail loudly with diagnostic if extents > 128 | **NO** |
| **13** | EPT / Memory | Full identity mapping without physical RAM bounds validation | `hypervisor.c:1420` | `VERIFIED-BY-CODE` | System Stability / MCE | Parse EFI memory map and omit unmapped or proprietary MMIO memory regions | **YES** |
| **14** | VMX Core | Suboptimal CR0/CR4 shadow register masks cause excessive VM-exits on guest task switches | `hypervisor.c:3180` | `VERIFIED-BY-CODE` | Performance Overhead | Set proper guest/host masks and shadows for paging/protection bits per SDM Vol 3C | **YES** |
| **15** | Input / HID | Focus switch chord (Ctrl+Alt+G) leaks make scancodes to guest without sending break codes | `usb_hid.c:340` | `VERIFIED-BY-CODE` | Usability / Stuck Keys | Track depressed key state; send break scancodes on all active keys when focus changes | **NO** |
| **16** | LAPIC / Timer | Virtual LAPIC LVT timer computes tick deltas without invariant TSC verification | `virtual_platform.c:640` | `VERIFIED-BY-CODE` | Guest Time Drift | Verify invariant TSC via CPUID and synchronize against ACPI PM timer | **NO** |
| **17** | ACPI / AML | Hand-crafted AML bytecode contains manual length calculations prone to offset errors | `virtual_platform.c:380` | `VERIFIED-BY-CODE` | Maintainability / Boot Crash | Generate AML tables from standard ASL using `iasl` compiler during build | **NO** |
| **18** | USB / XHCI | Transfer Error (`#1127`) logged without formal endpoint halt recovery sequence | `xhci.c:480` | `VERIFIED-BY-CODE`, `VERIFIED-ON-HARDWARE` | USB Bus Recovery | Implement standard ClearFeature(ENDPOINT_HALT) and set TR Dequeue Pointer | **NO** |
| **19** | UART / Serial | Virtual 16550 FIFO size (16 bytes) overflows under high baud burst debug output | `virtual_platform.c:1420` | `VERIFIED-BY-CODE` | Log Integrity | Expand internal ring buffer to 4096 bytes and flush in chunks | **NO** |
| **20** | Hypervisor | Absence of runtime CPUID capability check for VT-x before initializing VMXON structures | `hypervisor.c:420` | `VERIFIED-BY-CODE` | Boot Diagnostics | Display clear BIOS setup instructions if CPUID.1:ECX.VMX is disabled | **NO** |

---

## 2. Safe Staging Plan & Remediation Sequence

To ensure hypervisor stability without introducing regressions, remediations must follow strict phase isolation:

```mermaid
flowchart TD
    Phase1["Phase 1: Input & Console Isolation (Risks 1, 15, 4) - NON-VMX"] --> Phase2["Phase 2: Guest Kernel & ABI Parity (Risk 2) - NON-VMX"]
    Phase2 --> Phase3["Phase 3: Storage & Telemetry Hardening (Risks 9, 11, 12, 19) - NON-VMX"]
    Phase3 --> Phase4["Phase 4: VMX Core & In-Exit Latency Reductions (Risks 3, 5, 6, 8, 10, 13, 14) - VMX CRITICAL"]
```

### Stage 1: Input Routing & Virtual PS/2 Cleanup (Non-VMX)
- **Target Files**: `kernel/drivers/usb/usb_hid.c`, `kernel/core/hypervisor/virtual_platform.c`.
- **Action**:
  1. In `usb_hid.c`: If `dev->protocol == 2` (Mouse), drop packet or route to mouse accumulator; never enter keyboard scancode translation.
  2. In `usb_hid.c`: On focus loss (`INPUT_FOCUS_HOST`), transmit break scancodes (`0x9D`, `0xB8`) for Ctrl and Alt.
  3. In `virtual_platform.c`: Add AUX queue handling to i8042; set bit 5 (`0x20`) on reading port 0x64 when AUX byte is available.
- **Regression Verification**: Boot FreeBSD, move USB mouse rapidly; verify no random characters appear on console prompt `#`.
- **Rollback Plan**: Revert git commit; restore prior single-buffer i8042 logic.

### Stage 2: Guest Kernel & Userland Alignment (Non-VMX)
- **Target Files**: `tools/freebsd_payload/`, `kernel/kernel.c`.
- **Action**:
  1. Extract native `kernel` directly from FreeBSD 14.5 UFS2 partition on the USB stick (`vtbd0p2`).
  2. Embed the 14.5 kernel into `BOOTX64.EFI` replacing the legacy 14.1-RELEASE image.
- **Regression Verification**: Boot guest; confirm `init` executes without `getting pipebuf resource limit: invalid argument` warning.
- **Rollback Plan**: Revert embedded binary pointer in `kernel.c` back to `freebsd14.1_kernel_bin`.

### Stage 3: Telemetry Rate-Limiting & Storage Safety (Non-VMX)
- **Target Files**: `kernel/core/hypervisor/virtual_platform.c`, `kernel/vfs/vfs_legacy/storage/src/runtime_storage.c`.
- **Action**:
  1. In `virtual_platform.c:1865`: Rate-limit `VM_STATE_STOPPED` log emission to once every 5 seconds.
  2. Implement guest reboot trigger on keypress 'R'.
  3. In `runtime_storage.c:512`: Add bounds check on runlist extent count; warn if `count >= MAX_EXTENTS`.
- **Regression Verification**: Trigger port 0xCF9 reset; verify AMDE timeline is not flooded with thousands of identical lines.
- **Rollback Plan**: Revert rate-limit conditional.

### Stage 4: VMX Core & In-Exit Latency Refactoring (VMX-Critical)
- **Target Files**: `kernel/core/hypervisor/hypervisor.c`, `kernel/core/hypervisor/virtio_blk.c`, `kernel/drivers/usb/usb_msc.c`.
- **Action**:
  1. Reduce USB MSC command timeout from 5000 ms to 150 ms in `usb_msc.c:125`.
  2. Allocate isolated GPA pages for guest GDT and TSS; remove host `GDTR_BASE` leakage.
  3. Enhance default exit dispatcher handler to dump registers via UDP before halting.
- **Regression Verification**: Full QEMU pre-flight UEFI boot followed by single-step exit certification on ASUS B760M-K.
- **Rollback Plan**: Complete rollback to verified baseline commit `e98ace6f`.

---

## 3. Top 10 Uncertainties & Open Architectural Questions

1. **XHCI Transfer Error Trigger**: Does USB MSC Transfer Error (`#1127`) stem from concurrent polling or an unhandled Babble/Short-Packet condition on the USB 3.0 port?
2. **VirtIO-Input Deprecation vs Implementation**: Should `virtio_input.c` be completely removed to simplify the PCI device tree, or wired to host mouse packets to bypass legacy i8042 limitations?
3. **Guest Rootfs Write Strategy**: If FreeBSD daemons (syslogd, dhclient) attempt writes to `/var` or `/tmp`, will the current read-only return code `VIRTIO_BLK_S_IOERR` cause kernel panics once multi-user mode starts?
4. **Invariant TSC Stability on Raptor Lake**: Does the virtual LAPIC timer frequency scale correctly under Intel Speed Shift / E-core / P-core frequency transitions on the Core i3-14100F?
5. **Secondary Vector Starvation**: In high-throughput disk workloads, does the strict Timer > VirtIO priority hierarchy cause VirtIO descriptor ring starvation?
6. **Guest GDT Shadow Isolation**: Will loading guest-allocated GDT/TSS structures require additional VMCS segment descriptor attribute validation to avoid `VM_ENTRY_FAILURE_GUEST_STATE`?
7. **Single-Threaded Frame Latency**: How many microseconds does a 64KB VirtIO-BLK read stall the host display refresh loop when running at 1080p?
8. **USB HID Composite Receiver Endpoint Assignment**: On keyboards with integrated touchpads, does endpoint order vary across BIOS revisions, necessitating descriptor parsing instead of endpoint indexing?
9. **AML ASL Recompilation Feasibility**: Can the ACPI bytecode table be generated at build-time using `iasl` on Windows without requiring Cygwin or WSL dependencies?
10. **Dynamic Runlist Growth**: If FreeBSD_Runtime.img is re-written on NTFS and reaches >500 extents, what is the memory footprint impact of dynamic extent allocation in pre-kernel runtime storage?
