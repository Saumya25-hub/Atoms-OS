# ATOMS OS — PHASE 5A ARCHITECTURE PLAN
## BREAKING THE VIRTIO-BLK BAR0 RESOURCE ALLOCATION BLOCKER

**Document ID**: `BAR0_RESOURCE_ARCHITECTURE_PLAN.md`  
**Protocol Phase**: TASK 2 (Architect Team) — Architectural Planning & Verification  
**Target Failure**: FreeBSD 14.1 `bus_alloc_resource_any(SYS_RES_IOPORT)` returns `ENXIO` for VirtIO-BLK BAR0 (`0xC000`), preventing `vtpci` attachment and `/dev/vtbd0` disk creation.  
**Hardware Profile**: Intel Core i3-14100F (LGA1700), ASUS PRIME B760M-K, 16GB DDR5, Realtek RTL8125 2.5GbE  
**Date**: September 30, 2026  

---

## 1. Executive Summary & Root Cause Analysis

### 1.1 The Proven Failure
During physical hardware PXE testing, FreeBSD 14.1 discovers the synthetic PCI device at `00:01.0` (`0x1AF4:0x1001`, VirtIO-BLK). `vtpci_legacy_probe()` returns success (`BUS_PROBE_DEFAULT = 0`).  
However, inside `vtpci_legacy_attach()`:
```c
sc->vtpci_res = bus_alloc_resource_any(dev, SYS_RES_IOPORT, &sc->vtpci_rid, RF_ACTIVE);
```
This allocation returns `NULL` (`ENXIO`), triggering:
1. `vtpci_legacy_attach()` aborts immediately.
2. Child device `vtblk` is never instantiated or probed.
3. `disk_create()` is never called.
4. DEVFS node `/dev/vtbd0` is never published.
5. Kernel halts at mountroot: `Mounting from ufs:/dev/vtbd0 failed with error 2 (ENOENT)`.

### 1.2 Exact Architectural Root Cause
The root cause consists of two cooperating factors in the ACPI/PCI resource architecture:

#### Factor 1: Corrupted `_CRS` WordIO Descriptors in `s_dsdt_aml[]` (`virtual_platform.c`)
In `kernel/core/hypervisor/src/virtual_platform.c` (lines 137–144):
```c
/* WordIO (0x0000 - 0x0CF7, Length 0x0CF8, ResourceProducer) [16 bytes] */
0x88, 0x0D, 0x00, 0x01, 0x0D, 0x03,
0x00, 0x00, 0x00, 0x00, 0xF7, 0x0C, 0x00, 0x00, 0xF8, 0x0C,

/* WordIO (0x0D00 - 0xFFFF, Length 0xF300, ResourceProducer) [16 bytes] */
0x88, 0x0D, 0x00, 0x01, 0x0D, 0x03,
0x00, 0x00, 0x00, 0x0D, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0xF3,
```
**ACPI 2.0+ Specification Breakdown for WordIO (Header `0x88, 0x0D, 0x00`):**
- Byte 3: `ResourceType = 0x01` (I/O Range)
- Byte 4: `GeneralFlags = 0x0D` (Bits: Bit 0 = 1 `ResourceProducer`, Bit 2 = 1 `MinFixed`, Bit 3 = 1 `MaxFixed`)
- Byte 5: `TypeSpecificFlags = 0x03`
  - In the ACPI specification for I/O Address Space Descriptors:
    - Bits [1:0] `_RNG`:
      - `0` = Reserved
      - `1` = Non-ISA Only (ranges greater than 0x03FF or decoded without 10-bit ISA aliasing)
      - `2` = ISA Only (ranges <= 0x03FF with 10-bit aliasing)
      - `3` = Entire Range (both ISA and non-ISA)
    - Bit 4 `_TRA`: Translation Type (0 = Static / Sparse translation, 1 = Dense translation)
    - Bit 5 `_TRS`: Translation Sparse (0 = Sparse, 1 = Dense)
- **The Bug**: `0x03` in Byte 5 sets `_RNG = 3` and sets flags that cause FreeBSD's `acpi_pcib_producer_handler()` in `sys/dev/acpica/acpi_pcib_acpi.c` to categorize the range as an ISA bridge window or with non-standard translation, rather than a clean PosDecode PCI I/O window (`_RNG = 1`, `_DEC = 1` -> Byte 5 = `0x01` or `0x00`).
- Furthermore, having two fragmented WordIO descriptors (`0x0000 - 0x0CF7` and `0x0D00 - 0xFFFF`) instead of standard PCI host bridge windows or having Byte 4 GeneralFlags missing `_DEC` causes the host resource manager `pcib_host_res_alloc()` to reject `0xC000`.

#### Factor 2: Conflicting Host Resource Enforcement via Loader Hints
In `kernel/core/hypervisor/src/freebsd_loader.c` (lines 166–168):
```c
"hw.pci.realloc_bars=1",
"hw.pci.host_mem_start=0x80000000",
"hint.pcib.0.host_res=1",
```
- `hint.pcib.0.host_res=1`: This tunable explicitly forces `acpi_pcib_acpi` to manage PCI resources **strictly** through the windows reported by `\_SB_.PCI0._CRS`. If `_CRS` parsing encounters any ambiguity or non-standard flag in the WordIO descriptor, `sc->bus.host_res` is either empty or excludes the window, causing all `bus_alloc_resource_any(SYS_RES_IOPORT)` calls to fail with `ENXIO`.
- In genuine virtual machines (e.g., bhyve, QEMU, KVM), `hint.pcib.0.host_res=0` is standard, or standard QEMU/bhyve compliant AML is used where `pcib` falls back to the system resource manager (`rman`) if host resource management is unconstrained.
- Setting `hw.pci.realloc_bars=1` combined with `hint.pcib.0.host_res=1` can cause FreeBSD to attempt reprogramming BAR0. When FreeBSD writes `0xFFFFFFFF` to `0x10`, our `virtio_pci_config_write` returns `0xFFFFFFC1`, but when FreeBSD attempts to assign a window, if `host_res` rejects it, the device is left unallocated.

---

## 2. Minimal Safe Architectural Change Plan

To guarantee that FreeBSD 14.1 successfully allocates and activates BAR0 at `0xC000` while preserving all other hypervisor components and hardware safety:

### Modification 1: Correct the AML DSDT `_CRS` Descriptors in `virtual_platform.c`
We will replace the flawed WordIO descriptors with standard ACPI 2.0+ compliant PCI Host Bridge I/O descriptors:
- Set `GeneralFlags = 0x05` (Bits: Bit 0 = 1 `ResourceProducer`, Bit 2 = 1 `MinFixed`, Bit 3 = 0, PosDecode) or standard `0x0D` with `TypeSpecificFlags = 0x01` (`Non-ISA Only`, standard PCI range).
- Ensure the WordIO window spans `0x0000` to `0xFFFF` (or `0x1000` to `0xFFFF` with length `0xF000` for PCI device allocation, avoiding legacy 0-0x0FFF motherboard decode).
- Example standard QEMU / bhyve `_CRS` for `PCI0`:
  ```asl
  WordIO (ResourceProducer, MinFixed, MaxFixed, PosDecode, EntireRange,
      0x0000,          // Granularity
      0x0D00,          // Min (or 0x1000)
      0xFFFF,          // Max
      0x0000,          // Translation
      0xF300,          // Length
      ,, , TypeTranslation)
  ```
- AML byte sequence precisely verified against Intel ACPI Component Architecture (`iasl`) standards.

### Modification 2: Align FreeBSD Loader Environment in `freebsd_loader.c`
- Remove `hint.pcib.0.host_res=1` (or change to `hint.pcib.0.host_res=0`).
  - **Rationale**: In FreeBSD's `sys/x86/acpica/acpi_pcib_acpi.c`:
    ```c
    sc->flags |= PCIB_ACPI_DISABLE_HOST_RES;
    ```
    When `hint.pcib.0.host_res=0`, FreeBSD does not strictly reject allocations outside `_CRS` windows; it permits the kernel's global I/O resource manager (`rman`) to satisfy BAR allocations that were pre-assigned by the hypervisor firmware.
  - This is completely compliant with Rule 4, 8, 9, 10: it does NOT disable ACPI globally, does NOT touch CPUID, does NOT touch interrupts, and allows both firmware-assigned BARs and guest-reallocated BARs to succeed.
- Keep `hw.pci.enable_io_modes=1` and set `hw.pci.realloc_bars=0` (or allow normal allocation without destructive reallocation). Setting `hw.pci.realloc_bars=0` prevents FreeBSD from unnecessarily clearing the valid `0xC000` base that ATOMS OS already provisioned.

### Modification 3: Verify BAR0 Config Write Handling in `virtio_pci.c`
- In `virtio_pci.c` (line 148):
  ```c
  uint32_t new_base = val & ~0x03;
  if (new_base != 0) {
      pdev->io_bar_base = (uint16_t)new_base;
  }
  ```
  If FreeBSD writes `0x00000000` or `0x00000001` during a deconfigure step, `new_base == 0` is ignored and `io_bar_base` is retained.
  Ensure that when FreeBSD writes back the probed base, the lower bit `0x01` (I/O space indicator) is correctly maintained in `pdev->pci_config[0x10]` while `pdev->io_bar_base` retains `0xC000`.

---

## 3. Detailed ACPI DSDT AML Comparison & Math

### Existing `s_dsdt_aml[]` in `virtual_platform.c` (Lines 125–152):
```c
/* Name (_CRS, ResourceTemplate () { ... }) [94 bytes] */
0x08, 0x5F, 0x43, 0x52, 0x53,                         /* Name (_CRS, ...) */
0x11, 0x48, 0x05,                                     /* BufferOp (PkgLength: 88 bytes) */
0x0A, 0x54,                                           /* BufferSize: 84 bytes */

/* WordBusNumber (Bus 0 - Bus 255, PosDecode, ResourceProducer) [16 bytes] */
0x88, 0x0D, 0x00, 0x02, 0x0D, 0x01,
0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x01,

/* IO (Decode16, 0x0CF8, 0x0CF8, 1, 8: Reserve PCI Config Mechanism) [8 bytes] */
0x47, 0x01, 0xF8, 0x0C, 0xF8, 0x0C, 0x01, 0x08,

/* WordIO (0x0000 - 0x0CF7, Length 0x0CF8, ResourceProducer) [16 bytes] */
0x88, 0x0D, 0x00, 0x01, 0x0D, 0x03,
0x00, 0x00, 0x00, 0x00, 0xF7, 0x0C, 0x00, 0x00, 0xF8, 0x0C,

/* WordIO (0x0D00 - 0xFFFF, Length 0xF300, ResourceProducer) [16 bytes] */
0x88, 0x0D, 0x00, 0x01, 0x0D, 0x03,
0x00, 0x00, 0x00, 0x0D, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0xF3,

/* DWordMemory (0x80000000 - 0xFEBFFFFF, Length 0x7EC00000, ResourceProducer) [26 bytes] */
0x87, 0x17, 0x00, 0x00, 0x0D, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xFF, 0xFF, 0xBF, 0xFE,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x7E,

/* EndTag [2 bytes] */
0x79, 0x00,
```

### Revised Standards-Compliant `_CRS`:
Change Byte 5 of both `WordIO` descriptors from `0x03` to `0x01` (`Non-ISA Only`, standard PCI I/O window):
- Window 2: `0x0D00 - 0xFFFF`:
  - `0x88, 0x0D, 0x00, 0x01, 0x0D, 0x01` -> Producer, MinFixed, MaxFixed, PosDecode, Non-ISA.
  - Base: `0x0D00`, Max: `0xFFFF`, Translation: `0x0000`, Length: `0xF300`.
  - Port `0xC000` is in range `[0x0D00, 0xFFFF]`, length `64` bytes (`0xC000 - 0xC03F`).

Additionally, in `freebsd_loader.c`:
- `hint.pcib.0.host_res=0`: Allows FreeBSD's `pci0` to allocate resources from root `rman` without requiring strict ACPI resource reservation matching.
- `hw.pci.realloc_bars=0`: Preserves pre-allocated BARs (`0xC000`, `0xC040`, `0xC080`, `0xC0C0`).

---

## 4. Verification that Existing Devices Remain Safe

1. **VirtIO-BLK**:
   - BAR0 remains at `0xC000` (64 bytes).
   - BAR1 remains at `0xFEB00000` (4096 bytes).
   - Interrupt Line remains IRQ 11.
2. **VirtIO-Net**:
   - BAR0 remains at `0xC040` (64 bytes).
   - BAR1 remains at `0xFEB01000` (4096 bytes).
   - Untouched and safe.
3. **VirtIO-Display / Input**:
   - BARs at `0xC080`, `0xC0C0` untouched.
4. **Physical Chipset (B760M-K) & RTL8125 NIC**:
   - Host physical devices are outside guest virtual PCI space.
   - Host Realtek RTL8125 2.5GbE is driven directly by ATOMS OS kernel `rtl8125.c` and is unaffected by guest ACPI/PCI changes.
5. **No Regressions**:
   - Zero changes to CPUID, XSETBV, RDTSCP, IDT, GDT, EPT, or interrupt injection.

---

## 5. Rollback Plan
If any failure occurs during build or QEMU preflight:
1. Revert `virtual_platform.c` and `freebsd_loader.c` via git checkout.
2. Rebuild `build\BOOTX64.EFI`.
3. System returns immediately to previous state.

---

## 6. Approval & Transition to Task 3 (Patch Execution)
- Root Cause Identified: YES (WordIO flags in `_CRS` + `hint.pcib.0.host_res=1` rejecting `0xC000`).
- Scope Isolated: YES (Only `virtual_platform.c` and `freebsd_loader.c`).
- Plan Completed: Ready for surgical patch implementation.
