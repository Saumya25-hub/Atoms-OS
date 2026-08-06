# BOS GPU Driver V3 — Phase 3 Architecture: VirtIO GPU Hardware Driver

## Subsystem Architecture Overview

Phase 3 introduces the **VirtIO GPU 2D Hardware Acceleration Driver (`0x1AF4:0x1050`)** into the BOS OS graphics subsystem.

VirtIO GPU is the modern virtualization standard for QEMU/KVM, Proxmox, and cloud VMs. Like the VMware SVGA II driver, the VirtIO GPU driver plugs directly behind the Phase 1 GPU HAL (`bos_gpu_driver_ops_t`).

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
         | (Active VirtIO HW)                              | (Fallback)
         v                                                 v
+------------------------------------+          +--------------------+
|  VirtIO GPU 2D Hardware Driver     |          | Software Renderer  |
|  - MMIO Registers & Status Engine  |          | Fallback Driver    |
|  - Control VirtQueue (Queue 0)     |          +--------------------+
|  - 2D Resource & Scanout Engine    |
+------------------------------------+
```

---

## VirtQueue Engine & Command Protocol

Communication with VirtIO GPU hardware occurs asynchronously over **Control VirtQueue 0**:

### Ring Layout

1. **Descriptor Table (`virtq_desc_t`)**: Holds input request buffers (`VIRTQ_DESC_F_NEXT`) and output response buffers (`VIRTQ_DESC_F_WRITE`).
2. **Available Ring (`virtq_avail_t`)**: Indexes of descriptors submitted by the driver.
3. **Used Ring (`virtq_used_t`)**: Indexes of descriptors completed by the device.

---

## 2D Resource & Scanout Pipeline

### Command Flow

```
1. VIRTIO_GPU_CMD_RESOURCE_CREATE_2D   -> Creates 2D Resource ID 1 (1920x1080)
2. VIRTIO_GPU_CMD_RESOURCE_ATTACH_BACKING -> Binds physical memory pages to Resource 1
3. VIRTIO_GPU_CMD_SET_SCANOUT          -> Binds Resource 1 to Scanout Display 0
4. VIRTIO_GPU_CMD_TRANSFER_TO_HOST_2D  -> Transfers guest surface updates to host
5. VIRTIO_GPU_CMD_RESOURCE_FLUSH       -> Flushes host resource region to scanout screen
```

---

## Diagnostic Report Example

```
==================================
 VIRTIO GPU DIAGNOSTICS REPORT   
==================================
 PCI Vendor          : 0x1AF4
 PCI Device          : 0x1050
 MMIO Base           : 0xFEBC0000
 Control Queue       : READY (Size: 64)
 Cursor Queue        : READY
 Framebuffer Phys    : 0x80500000
 Resolution          : 1920x1080 @ 32bpp
 Resource Count      : 1 (Active Res ID: 1)
 Scanout Active      : YES
 Queue Ready         : YES
 Present Ready       : YES
 STATUS              : PASS
==================================
```

---

## Phase 4 Roadmap

- **Phase 4A**: Unified Multi-GPU Dynamic Arbiter & Hotplug Subsystem.
- **Phase 4B**: Hardware Cursor Plane Engine across VirtIO, VMware, and Intel display controllers.
- **Phase 4C**: VirGL 3D Command Buffer Streamer for Vulkan & OpenGL acceleration.
