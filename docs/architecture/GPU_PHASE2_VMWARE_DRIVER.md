# BOS GPU Driver V2 — Phase 2 Architecture: VMware SVGA II Hardware Driver

## Subsystem Architecture Overview

Phase 2 introduces the first real hardware-accelerated graphics driver in BOS OS: the **VMware SVGA II Driver** (`0x15AD:0x0405`).

The VMware driver operates completely behind the Phase 1 GPU HAL (`bos_gpu_driver_ops_t`), replacing CPU software framebuffer presentation with hardware FIFO command packets, hardware rectangle fill acceleration (`SVGA_CMD_RECT_FILL`), and hardware rectangle copy acceleration (`SVGA_CMD_RECT_COPY`).

Existing desktop components—including the Window Manager, BOImage, BOSPECTRA, and Compositor—remain 100% untouched while gaining direct hardware acceleration.

```
+-------------------------------------------------------------------+
|               BOS Compositor / Window Manager / Apps              |
+-------------------------------------------------------------------+
                                  |
                                  v
+-------------------------------------------------------------------+
|               BOS GPU Subsystem HAL (bos_gpu_*)                   |
+-------------------------------------------------------------------+
                                  |
         +------------------------+------------------------+
         | (Primary Active HW)                             | (Fallback)
         v                                                 v
+------------------------------------+          +--------------------+
|  VMware SVGA II Hardware Driver    |          | Software Renderer  |
|  - Register Engine (Index/Value)   |          | Fallback Driver    |
|  - Command FIFO (2D Acceleration)  |          +--------------------+
|  - VRAM Framebuffer (Scanout)      |
+------------------------------------+
```

---

## PCI Binding & BAR Discovery

The driver automatically detects and binds to VMware PCI Display Adapters:
- **Vendor ID**: `0x15AD`
- **Device ID**: `0x0405` (SVGA II) / `0x0406` (SVGA III) / `0x1234` (VGA compat)

### BAR Layout

| BAR Index | Type | Purpose | Subsystem Mapping |
|---|---|---|---|
| **BAR 0** | Port I/O | Index & Value Registers (`SVGA_INDEX_PORT`, `SVGA_VALUE_PORT`) | Base Port (e.g. `0x1500`) |
| **BAR 1** | Memory | Linear Framebuffer (VRAM) | Physical VRAM Base Address |
| **BAR 2** | Memory | Command FIFO Ring Buffer | Physical FIFO Base Address |

---

## Register Access & SVGA Protocol

Register access is handled via 32-bit I/O ports relative to BAR 0:
- `SVGA_INDEX_PORT` = `io_base + 0x00`
- `SVGA_VALUE_PORT` = `io_base + 0x01`

```c
void gpu_svga_write_reg(svga, index, value) {
    io_out32(svga->io_base + SVGA_INDEX_PORT, index);
    io_out32(svga->io_base + SVGA_VALUE_PORT, value);
}

uint32_t gpu_svga_read_reg(svga, index) {
    io_out32(svga->io_base + SVGA_INDEX_PORT, index);
    return io_in32(svga->io_base + SVGA_VALUE_PORT);
}
```

### Device Negotiation

During bring-up, the driver negotiates the highest supported SVGA ID version:
1. Write `SVGA_ID_2` (`0x90000002`) to `SVGA_REG_ID`.
2. Read `SVGA_REG_ID`. If it returns `SVGA_ID_2`, negotiation succeeds.
3. If not, fallback sequentially to `SVGA_ID_1` (`0x90000001`) and `SVGA_ID_0` (`0x90000000`).

---

## Command FIFO Engine

The command FIFO is initialized in BAR 2 memory to allow asynchronous 2D command submission to the GPU:

### FIFO Header Registers

- `SVGA_FIFO_MIN` (Offset 0): Header size boundary (`64` bytes / `16` words).
- `SVGA_FIFO_MAX` (Offset 1): FIFO capacity in bytes (`fifo_size`).
- `SVGA_FIFO_NEXT_CMD` (Offset 2): Head write pointer.
- `SVGA_FIFO_STOP` (Offset 3): Tail execution pointer.
- `SVGA_FIFO_CAPABILITIES` (Offset 4): Bitmask of hardware acceleration capabilities (`SVGA_FIFO_CAP_RECT_FILL`, `SVGA_FIFO_CAP_RECT_COPY`).

After header setup, `SVGA_REG_CONFIG_DONE` is set to `1` to enable hardware command execution.

---

## Hardware 2D Operations

### 1. Hardware Present (`bos_gpu_present`)

Sends an `SVGA_CMD_UPDATE` packet over FIFO to synchronize the display region:

```c
cmd[0] = SVGA_CMD_UPDATE;
cmd[1] = x;
cmd[2] = y;
cmd[3] = width;
cmd[4] = height;
fifo_commit(20);
gpu_svga_write_reg(svga, SVGA_REG_SYNC, 1);
```

### 2. Hardware FillRect (`fill_rect`)

Sends an `SVGA_CMD_RECT_FILL` packet over FIFO:

```c
cmd[0] = SVGA_CMD_RECT_FILL;
cmd[1] = color;
cmd[2] = x;
cmd[3] = y;
cmd[4] = width;
cmd[5] = height;
fifo_commit(24);
```

### 3. Hardware Copy (`copy`)

Sends an `SVGA_CMD_RECT_COPY` packet over FIFO:

```c
cmd[0] = SVGA_CMD_RECT_COPY;
cmd[1] = srcX;
cmd[2] = srcY;
cmd[3] = destX;
cmd[4] = destY;
cmd[5] = width;
cmd[6] = height;
fifo_commit(28);
```

---

## Diagnostic Report Example

```
====================================
 VMWARE SVGA II DIAGNOSTICS REPORT  
====================================
 PCI Vendor          : 0x15AD
 PCI Device          : 0x90000002
 IO Port Base        : 0x1500
 Framebuffer Phys    : 0xFD000000
 Framebuffer Size    : 16 MB
 FIFO Phys Base      : 0xFEBF0000
 FIFO Size           : 256 KB
 FIFO Ready          : YES
 Mode Set            : 1920x1080 @ 32bpp
 Hardware Present    : READY
 Hardware FillRect   : READY
 Hardware Copy       : READY
 Hardware Stretch    : READY
 STATUS              : PASS
====================================
```

---

## Phase 3 Roadmap

- **Phase 3A**: VirtIO GPU Driver implementation (`0x1AF4:0x1050`) for QEMU/KVM guests.
- **Phase 3B**: VRAM Allocator Subsystem for multi-surface memory pools and hardware cursor planes.
- **Phase 3C**: Hardware Cursor Plane integration with HIDA input engine.
