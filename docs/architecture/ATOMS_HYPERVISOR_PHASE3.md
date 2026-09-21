# ATOMS OS — Lightweight Native Micro-Hypervisor
## Phase 3: VirtIO Virtual Hardware Subsystem Architecture & Specification

---

## 1. Executive Summary & Architecture

Phase 3 of the **ATOMS Micro-Hypervisor** implements a clean, high-performance, and fully auditable **VirtIO Virtual Hardware Subsystem** designed to host an isolated **Full FreeBSD amd64 Guest OS** directly on top of the native **BOS Kernel**.

```text
+-----------------------------------------------------------------------------------+
|                                     ATOMS OS                                      |
|            (BWE Desktop / BCM Compositor / Native Ring 3 Applications)            |
+-----------------------------------------------------------------------------------+
                                         │
                                         ▼
+-----------------------------------------------------------------------------------+
|                                    BOS Kernel                                     |
|             (PMM, VMM, Scheduler, VFS/BOFS, AHME, DGL, Network Stack)             |
+-----------------------------------------------------------------------------------+
                                         │
                                         ▼
+-----------------------------------------------------------------------------------+
|                          ATOMS Native Micro-Hypervisor                            |
|  ├── CPU Virtualization (Phase 1: Intel VMX / AMD SVM Core)                       |
|  ├── Memory Virtualization (Phase 2: Intel EPT / AMD NPT Second-Level Tables)     |
|  └── Virtual Hardware Layer (Phase 3: VirtIO & Virtual PCI Subsystem):            |
|       ├── Virtual PCI Bus & Configuration Space Emulation (Vendor 0x1AF4)         |
|       ├── VirtIO Standard Split Queues (Descriptor, Available, Used Rings)        |
|       ├── VirtIO-Block (`virtio-blk`): 16 MB Isolated RAM-Disk Backing            |
|       ├── VirtIO-Net (`virtio-net`): Sandboxed RX/TX Network Engine               |
|       ├── VirtIO-Input (`virtio-input`): Keyboard & Mouse Event Injection         |
|       └── VirtIO-Display (`virtio-gpu` 2D Mode): Shared Linear Framebuffer        |
+-----------------------------------------------------------------------------------+
                                         │
                                         ▼
+-----------------------------------------------------------------------------------+
|                      Future FreeBSD amd64 Guest Container                         |
|  ├── FreeBSD Kernel Drivers: `virtio_pci.ko`, `vtnet.ko`, `virtio_blk.ko`         |
|  ├── FreeBSD Linear Framebuffer Console (`vt_efifb` / `xf86-video-scfb`)          |
|  └── Sandboxed Chromium / Browser Engine (Phase 8+)                               |
+-----------------------------------------------------------------------------------+
```

---

## 2. Architecture & Design Principles

1. **Native BOS Independence**:
   - ATOMS OS is an independent operating system. The hypervisor is a built-in BOS subsystem.
   - FreeBSD is strictly an isolated guest workload running inside an EPT/NPT hardware container.
2. **Deterministic Host Safety & Isolation**:
   - The virtual block device operates strictly on pre-allocated kernel RAM (`void *storage_backing`). Under zero circumstances is physical host storage or the host filesystem accessible to guest drivers.
   - The network subsystem operates in an isolated loopback queue and will only bridge through verified BOS network filters.
3. **VirtIO Split Queue Standard Compliance**:
   - Implements VirtIO Legacy Split Queues (v0.9.5 / v1.0 legacy mode) compatible with standard FreeBSD `virtio_pci` kernel drivers.
   - Translates Guest Physical Addresses (GPA) to Host Virtual Addresses (HVA) with strict guest RAM boundary enforcement.
   - Cycle/loop detection prevents malformed or malicious guest descriptor chains from hanging host hypervisor threads.

---

## 3. Subsystem Breakdown

### 3.1 Virtual PCI Bus & VirtIO PCI Transport

- **Emulated PCI Topology**: Bus 0, Device/Slot 1..4, Function 0.
- **PCI Identifiers**:
  - Vendor ID: `0x1AF4` (Red Hat / VirtIO)
  - Device IDs:
    - Slot 1: `0x1001` (VirtIO Block, Class `0x018000`)
    - Slot 2: `0x1000` (VirtIO Network, Class `0x020000`)
    - Slot 3: `0x1052` (VirtIO Input, Class `0x098000`)
    - Slot 4: `0x1050` (VirtIO Display, Class `0x030000`)
- **BAR Layout**:
  - BAR0: I/O Port Window (base allocated from `0xC000`, size 64 bytes per device)
  - BAR1: MMIO Window (base allocated from `0xFEB00000`, size 4 KB per device)
- **VirtIO PCI Header Register Map (BAR0)**:
  - `0x00`: Host Features (`uint32_t`, R)
  - `0x04`: Guest Features (`uint32_t`, W)
  - `0x08`: Queue PFN (`uint32_t`, RW)
  - `0x0C`: Queue Size (`uint16_t`, R)
  - `0x0E`: Queue Select (`uint16_t`, RW)
  - `0x10`: Queue Notify (`uint16_t`, W)
  - `0x12`: Device Status (`uint8_t`, RW)
  - `0x13`: ISR Status (`uint8_t`, R - Read-to-Clear)
  - `0x14`: Device Specific Configuration

---

### 3.2 VirtIO Split Queue Architecture

The VirtIO Split Queue consists of three memory areas:
1. **Descriptor Table (`virtq_desc_t`)**: 16-byte descriptors containing `addr` (GPA), `len`, `flags` (`NEXT`, `WRITE`, `INDIRECT`), and `next` index.
2. **Available Ring (`virtq_avail_t`)**: 16-bit ring containing `flags`, `idx`, and an array of head descriptor indices queued by the guest.
3. **Used Ring (`virtq_used_t`)**: 16-bit ring containing `flags`, `idx`, and an array of `virtq_used_elem_t` (`id`, `len`) populated by the hypervisor upon I/O completion.

**Queue Processing Invariants**:
- PFN registration verifies queue memory falls completely inside the registered `GuestMemory` container.
- Ring pointers wrap naturally via `(ring_idx & (queue_size - 1))`.
- Descriptors are traversed with a hop counter capped at `queue_size` to eliminate chain loops.

---

### 3.3 VirtIO-Block (`virtio-blk`)

- **Backing Store**: 16 MB pre-allocated kernel RAM disk (32,768 sectors @ 512 bytes/sector).
- **Supported Operations**:
  - `VIRTIO_BLK_T_IN` (0): Read sectors from RAM disk to guest buffers.
  - `VIRTIO_BLK_T_OUT` (1): Write sectors from guest buffers to RAM disk.
  - `VIRTIO_BLK_T_FLUSH` (4): Cache barrier synchronization (instant ACK).
  - `VIRTIO_BLK_T_GET_ID` (8): Returns device serial `"ATOMS-VBLK-001"`.
- **Status Returns**: `VIRTIO_BLK_S_OK` (0), `VIRTIO_BLK_S_IOERR` (1), `VIRTIO_BLK_S_UNSUPP` (2).

---

### 3.4 VirtIO-Net (`virtio-net`)

- **Queue Architecture**:
  - Queue 0: RX (Receive) Ring
  - Queue 1: TX (Transmit) Ring
- **Device Properties**:
  - MAC Address: `52:54:00:12:34:56`
  - Link Status: `0x0001` (Active / Carrier Up)
  - Maximum Packet Size: 1514 bytes
  - Internal Packet Buffer Depth: 64 packets
- **Processing**:
  - TX requests parse the 10-byte `virtio_net_hdr_t` followed by ethernet payload.
  - RX populates guest buffers with incoming packets and writes 10-byte header with zero flags.
  - Triggers Queue ISR (`0x01`) to notify guest driver.

---

### 3.5 VirtIO-Input (`virtio-input`)

- **Queue Architecture**:
  - Queue 0: Event Ring (Guest supplies empty buffers for hypervisor events)
  - Queue 1: Status Ring (Device status / LED feedback)
- **Supported Linux/FreeBSD Event Types**:
  - `EV_SYN` (`0x00`): Event delimiter synchronization
  - `EV_KEY` (`0x01`): Key presses and releases (keyboard & mouse buttons)
  - `EV_REL` (`0x02`): Relative movement (`REL_X`, `REL_Y`, `REL_WHEEL`)
- **Injection Pipeline**:
  - Hypervisor captures host mouse/keyboard inputs and enqueues them via `virtio_input_inject_key()` and `virtio_input_inject_rel_mouse()`.
  - `virtio_input_flush_events()` completes guest event buffers and signals ISR.

---

### 3.6 VirtIO-Display / 2D Framebuffer Subsystem

- **Resolution**: 1024 × 768 @ 32 bpp (BGRA / 4 MB Framebuffer).
- **Shared Memory Mapping**:
  - GPA: High guest physical memory (e.g. `0xE0000000`).
  - HVA: Directly accessible to BOS Window Compositor (BCM).
- **Dirty Region Tracking**:
  - Hypervisor records `(dirty_x, dirty_y, dirty_w, dirty_h)` whenever the guest triggers a display flush.
  - Enables zero-copy BCM composition with negligible host overhead.
- **3D GPU Acceleration**: Explicitly marked `BLOCKED / DEFERRED TO PHASE 6` pending FreeBSD `drm-kmod` driver stack integration.

---

## 4. Phase 3 Synthetic Acceptance Test Suite (25 Tests)

| Test ID | Test Description | Verification Target | Status |
| :--- | :--- | :--- | :--- |
| **TEST 01** | `virtio_pci_bus_create` | Virtual PCI bus initialization and memory allocation | **PASS** |
| **TEST 02** | Device Registration (4 Devices) | VirtIO-BLK, NET, INPUT, DISPLAY registration to slots 1-4 | **PASS** |
| **TEST 03** | PCI Configuration Space Identification | Vendor ID `0x1AF4` and Device ID verification on all slots | **PASS** |
| **TEST 04** | VirtIO Device Initialization & Reset | Status transitions (`RESET` ➔ `ACKNOWLEDGE` ➔ `DRIVER`) | **PASS** |
| **TEST 05** | VirtIO Feature Negotiation | Host/Guest feature bitmask handshakes | **PASS** |
| **TEST 06** | VirtIO Queue Allocation | Queue initialization, size bounds, and descriptor table setup | **PASS** |
| **TEST 07** | VirtIO Queue PFN Configuration | GPA ➔ HVA translation and address range bounding | **PASS** |
| **TEST 08** | VirtIO-BLK Creation | 16 MB RAM disk creation and capacity verification (32,768 sectors) | **PASS** |
| **TEST 09** | VirtIO-BLK Device Registration | PCI config space attachment & BAR configuration | **PASS** |
| **TEST 10** | VirtIO-BLK Feature Bits | `VIRTIO_BLK_F_SIZE_MAX`, `SEG_MAX`, `FLUSH` feature exposure | **PASS** |
| **TEST 11** | VirtIO-BLK Configuration Space | Sector count readback (`0x8000` = 32,768 sectors) | **PASS** |
| **TEST 12** | VirtIO-BLK Write Operation | Synchronous sector write to RAM disk payload buffer | **PASS** |
| **TEST 13** | VirtIO-BLK Read Verification | Data integrity verification of written sectors | **PASS** |
| **TEST 14** | VirtIO-BLK Flush Command | `VIRTIO_BLK_T_FLUSH` command execution and completion ACK | **PASS** |
| **TEST 15** | VirtIO-BLK Bounds Violation | Out-of-bounds sector request properly returns `IOERR` | **PASS** |
| **TEST 16** | VirtIO-NET Creation & MAC Init | MAC `52:54:00:12:34:56` and carrier link up state | **PASS** |
| **TEST 17** | VirtIO-NET Feature Negotiation | `VIRTIO_NET_F_MAC` and `STATUS` feature validation | **PASS** |
| **TEST 18** | VirtIO-NET Packet Transmission (TX) | Ethernet packet ring parsing and buffer processing | **PASS** |
| **TEST 19** | VirtIO-NET Packet Reception (RX) | Packet payload copy into guest RX descriptor ring | **PASS** |
| **TEST 20** | VirtIO-INPUT Creation & Config | Event queue setup and input device status transitions | **PASS** |
| **TEST 21** | VirtIO-INPUT Key Injection | `EV_KEY` (Keycode 30 'A' Press & Release) delivery | **PASS** |
| **TEST 22** | VirtIO-INPUT Mouse Injection | `EV_REL` (+10 X, -5 Y movement) delivery to guest ring | **PASS** |
| **TEST 23** | VirtIO-DISPLAY Creation (1024x768) | Framebuffer geometry and memory allocation verification | **PASS** |
| **TEST 24** | VirtIO-DISPLAY Flush & Dirty Rect | Dirty rect tracking `(100, 100, 200, 200)` and flush ACK | **PASS** |
| **TEST 25** | Complete Virtual Hardware Teardown | Zero memory leaks upon bus and device destruction | **PASS** |

---

## 5. Certification Sign-Off

- **Phase 3 Implementation**: **COMPLETE**
- **Synthetic Test Suite**: **25 / 25 PASS (100%)**
- **Host System Stability**: **CONFIRMED** (Clean build, zero boot regressions)
- **Ready for Next Stage**: **PHASE 4 (Guest Kernel Loading & Execution Foundation)**
