# BOS VRAM Memory Manager (BVMM) Phase 10 Architecture
## Production GPU Hardware Abstraction Layer (BGHAL V1.0) Specification

### 1. Overview & Architectural Role
The **BOS GPU Hardware Abstraction Layer (BGHAL V1.0)** is the permanent hardware gateway between BVMM and physical GPU drivers. It provides a unified, vendor-neutral virtual function table interface for Intel (Xe/i915), AMD (AMDGPU), NVIDIA (Open GPU), VirtIO GPU, VMware SVGA II, and Software Renderers.

```
+-------------------------------------------------------+
|  Applications / OpenGL / BOCompositor / Decoders      |
+-------------------------------------------------------+
                           |
                           v
+-------------------------------------------------------+
|  BOS VRAM Memory Manager Subsystems (Phases 1 - 9)    |
+-------------------------------------------------------+
                           |
                           v
+-------------------------------------------------------+
|  GPU Hardware Abstraction Layer (BGHAL V1.0)          |
|  - GPU Device Discovery & PCI BAR Management          |
|  - Unified DMA Copy Engine (VRAM <-> System/GTT)      |
|  - GPU Page Table Engine (Virtual -> Physical VRAM)   |
|  - Hardware Fence Integration (BFSE Gateway)          |
|  - Cache Flush & TLB Invalidation Engine              |
|  - Capability Matrix Enumeration Engine               |
|  - Virtual Table Dispatcher (bghal_backend_vtbl_t)    |
+-------------------------------------------------------+
       │           │           │           │           │
       v           v           v           v           v
  [Intel Xe]  [AMDGPU]   [NVIDIA]   [VirtIO]   [VMware]   [Software]
```

---

### 2. Vendor Backend Virtual Function Table Interface
Every vendor backend implements `bghal_backend_vtbl_t`:
- `init()` / `shutdown()`: Hardware backend lifecycle.
- `allocate_vram()` / `free_vram()`: Direct VRAM aperture management.
- `map_bar()` / `unmap_bar()`: PCI BAR MMIO mapping.
- `dma_copy()`: Asynchronous hardware DMA transfer engine.
- `fence_signal()` / `fence_wait()`: Hardware timeline fence synchronization.
- `page_table_update()`: GPU virtual memory page table mapping.
- `cache_flush()` / `tlb_invalidate()`: Memory visibility & TLB invalidation.
- `handle_interrupt()`: Hardware IRQ vector dispatcher.
- `query_capabilities()`: Vendor feature matrix enumeration.

---

### 3. Supported Vendor Matrix

| Vendor | Backend Module | Vendor ID | Device Target Example |
| :--- | :--- | :--- | :--- |
| **Intel** | `bghal_intel.c` | `0x8086` | Intel Iris Xe / DG2 / i915 |
| **AMD** | `bghal_amd.c` | `0x1002` | AMD Radeon RX 6800 XT / RDNA2 |
| **NVIDIA** | `bghal_nvidia.c` | `0x10DE` | NVIDIA RTX 3080 / Ampere |
| **VirtIO** | `bghal_virtio.c` | `0x1AF4` | VirtIO GPU 3D Passthrough |
| **VMware** | `bghal_vmware.c` | `0x15AD` | VMware SVGA II 3D Accelerator |
| **Software** | `bghal_swrender.c` | `0xFFFF` | ATOMS Software Rasterizer |

---

### 4. Integration Dependencies
- **Phase 9 Live Migration Engine (BLMDE)**: Hardware DMA copy execution.
- **Phase 7 Sync Engine (BFSE)**: Hardware fence signal/wait integration.
- **Phase 11 Production Cross-Process Sharing Engine (Next)**: Phase 11 will use BGHAL's page table and DMA engines to share VRAM surfaces across process boundaries via dma-buf.
