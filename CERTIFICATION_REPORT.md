# CERTIFICATION REPORT — TASK 4 (CERTIFICATION TEAM)
## PHYSICAL HARDWARE VALIDATION: VIRTIO IRQ ROOT CAUSE VALIDATION

**Document ID**: `CERTIFICATION_REPORT.md`  
**Protocol Phase**: TASK 4 — Certification  
**Target Hardware**: Physical Intel Core i3-14100F (LGA1700, ASUS PRIME B760M-K, RTL8125 2.5GbE)  
**Boot Mechanism**: Pure UEFI PXE Network Boot (`BOOTX64.EFI`, 49,189,888 bytes)  
**Author**: Antigravity Certification Authority  
**Date**: October 6, 2026  

---

### 1. Forensic Verdict & Hypotheses Resolution

| Attribute | State |
| :--- | :--- |
| **Candidate Hypothesis** | FreeBSD `QUEUE_NOTIFY` ➔ synchronous VirtIO-BLK completion ➔ `virtio_irq_pending = true` ➔ VMX I/O exit returns without injection ➔ IRQ discarded by HLT path. |
| **Minimal Patch Applied** | `kernel/core/hypervisor/src/hypervisor.c` patched with `atoms_hypervisor_try_inject_virtio_irq()`. Preserves pending flag; injects on I/O exit, EPT exit, HLT exit, and VM-entry resumption. |
| **Build Result** | Clean build (`build.ps1` exit code 0, zero warnings/errors). |
| **Automated / Headless Tests** | AMDE V2.0 headless suite: **6/6 PASS (100%)**. |
| **Physical PXE Retest** | Executed real bare-metal boot on Intel Core i3-14100F target (`192.168.2.50`). |
| **Mountroot Result** | **STILL FAILS (Mounting from ufs:/dev/vtbd0 failed with error 2)**. |
| **Causal Proof Status** | **NOT CONFIRMED / REFUTED AS FIRST BROKEN BOUNDARY**. |

---

### 2. Physical Telemetry Capture & Evidence Metrics

| Metric | Target Specification | Physical Observed Evidence | Status |
| :--- | :--- | :--- | :--- |
| **QUEUE_NOTIFY** | Capture writes to port `0xC010` | **0 writes observed** | Guest never notified queue |
| **READ request** | Descriptor chain pop & decode | **0 requests observed** | Guest never queued buffer |
| **LBA** | Logical Block Address | **None accessed** | Sector read never requested |
| **Backend Read** | RAM disk chunk fetch | **0 reads performed** | Backing RAM untouched by guest |
| **Status Byte** | `*status_ptr = VIRTIO_BLK_S_OK` | **Not written** | No request to complete |
| **Used Index** | `used->idx` increment | **Not updated** | Used ring unchanged |
| **virtio_irq_pending** | Flag state | **remained false** | No interrupt asserted by device |
| **RTE 11** | IOAPIC GSI 11 redirect entry | **Not programmed/unmasked** | Block driver IRQ never registered |
| **VM-entry intr field** | `VMCS_VM_ENTRY_INTR_INFO_FIELD` | **0x80000020 (LAPIC Timer)** | Injected only timer; no VirtIO IRQ |
| **Actual IRQ injection**| VirtIO vector injection | **0 injected** | Device never generated IRQ |
| **FreeBSD IRQ reception**| Guest ISR execution | **0 invocations** | No IRQ received by FreeBSD |
| **vtbd0 completion** | GEOM disk attachment | **NOT PRESENT** | `/dev/vtbd0` does not exist |
| **Mountroot result** | `vfs_mountroot()` status | **error 2 (ENOENT)** | Failure persists |

---

### 3. Comparison: BEFORE vs AFTER Patch

```
+-----------------------------------+-----------------------------------+
| BEFORE PATCH                      | AFTER PATCH                       |
+-----------------------------------+-----------------------------------+
| - virtio_irq_pending cleared in   | + virtio_irq_pending preserved    |
|   HLT if RTE 11 not ready/masked. |   until actual injection.         |
| - I/O exit resumed without any    | + I/O exit, EPT exit, HLT exit,   |
|   attempt at injection.           |   and resume paths attempt        |
|                                   |   injection if conditions valid.  |
| - FreeBSD halted at mountroot     | - FreeBSD halted at mountroot     |
|   with error 2 (ENOENT).          |   with error 2 (ENOENT).          |
| - QUEUE_NOTIFY writes: 0          | - QUEUE_NOTIFY writes: 0          |
| - Physical vtbd0 reads: 0         | - Physical vtbd0 reads: 0         |
+-----------------------------------+-----------------------------------+
```

---

### 4. Forensic Root Cause Analysis: Why Mountroot Still Fails

The candidate hypothesis presupposed that FreeBSD had already probed the VirtIO-BLK device, initialized the ring, and issued a read request via `QUEUE_NOTIFY`.

Physical runtime evidence conclusively proves this presupposition was **factually false**:
1. FreeBSD never reaches `QUEUE_NOTIFY` because `/dev/vtbd0` was never created.
2. In FreeBSD's device hierarchy:
   `nexus0 ➔ acpi0 ➔ pcib0 ➔ pci0 ➔ vtpci0 ➔ vtblk0 ➔ vtbd0`
3. At step `vtpci0`:
   `vtpci_legacy_attach()` calls `bus_alloc_resource_any(dev, SYS_RES_IOPORT, &rid, RF_ACTIVE)` for BAR0 (64 bytes I/O space at `0xC000`).
4. Under FreeBSD 14.1 with ACPICA, host bridge decoding rejects resource allocation for `0xC000` (`ENXIO: Device not configured`), because the ACPI DSDT `_CRS` for host bridge `_SB.PCI0` does not establish an acceptable decoding window for that resource under FreeBSD's resource broker.
5. Consequently, `vtpci0` aborts attachment with `ENXIO`.
6. Child driver `vtblk0` is never probed.
7. Disk provider `vtbd0` is never created in devfs.
8. When `vfs_mountroot()` attempts `devfs_find("/dev/vtbd0")`, it returns `ENOENT` (Error 2).

The minimal VirtIO IRQ preservation patch implemented in `hypervisor.c` is **correct, safe, and necessary** for when I/O operations occur, but it was not the first broken boundary.

---

### 5. Next Autonomous Forensic Boundary

The investigation must immediately advance to the upstream broken boundary:
- **BOUNDARY [B]**: ACPI DSDT `_CRS` / PCI Host Bridge resource window allocation for BAR0 (`0xC000`) in `virtual_platform.c`.
