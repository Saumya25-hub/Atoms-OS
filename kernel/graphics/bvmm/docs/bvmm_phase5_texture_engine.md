# BOS VRAM Memory Manager (BVMM) Phase 5 Architecture
## Production Texture & Format Engine (BTFE V1.0) Specification

### 1. Overview & Architectural Role
The **BOS Texture & Format Engine (BTFE V1.0)** is the permanent GPU texture subsystem of BOS. It operates as the single texture authority for OpenGL, Vulkan, BOCompositor, Window Engine, Video Decoders, and Image Engines.

BTFE sits directly above the Phase 4 Surface Manager Engine (BSME V1.0). Every texture object owns **exactly one BSME surface**, which in turn requests backing memory from PMPE and the Hybrid Heap.

```
+-------------------------------------------------------+
|  Applications / OpenGL / BOCompositor / Video Decoder |
+-------------------------------------------------------+
                           |
                           v
+-------------------------------------------------------+
|       BOS Texture & Format Engine (BTFE V1.0)         |
|  - Global Texture Registry (64-Bit Texture ID + Gen) |
|  - Mipchain Packing Engine (1x1 to 8192x8192)         |
|  - Swizzle Matrix & Format Descriptors (20+ Formats)  |
|  - Tile Layout Engine (Linear, Tile-X, Tile-Y, Morton)|
|  - Texture Views & Sampler State Descriptors          |
+-------------------------------------------------------+
                           |
                           v
+-------------------------------------------------------+
|       Phase 4 BOSurface Manager Engine (BSME)         |
+-------------------------------------------------------+
                           |
                           v
+-------------------------------------------------------+
|       Phase 3 Production Memory Pool Engine (PMPE)    |
+-------------------------------------------------------+
```

---

### 2. Mipchain Packing Engine
BTFE automatically generates non-overlapping mipchain layouts. Each mip level is 256-byte aligned to satisfy modern hardware GPU alignment requirements:

$$\text{AlignedOffset}_{i} = \text{Align}_{256}(\text{Offset}_{i-1} + \text{Size}_{i-1})$$

```
Level 0: [ 4096 x 4096 ] (Offset: 0x00000000)
Level 1: [ 2048 x 2048 ] (Offset: 0x01000000)
Level 2: [ 1024 x 1024 ] (Offset: 0x01400000)
...
Level N: [    1 x    1 ]
```

---

### 3. Tile Layout Engine & Morton Z-Order
BTFE converts coordinates between linear scanline layouts and 2D space-filling Morton curves (Z-order) to maximize GPU L1/L2 texture cache locality:

$$Z(x,y) = \text{InterleaveBits}(x, y)$$

Supported Tile Modes: `LINEAR`, `X_TILE`, `Y_TILE`, `MORTON_ZORDER`, `BLOCK_LINEAR`, `VENDOR_NATIVE`.

---

### 4. Texture Upload Pipeline
```
[CPU Pixel Buffer]
       │
       ▼  btfe_texture_upload(texture_id, mip_level, src_pixels)
[BSME Surface BAR Mapping] -> (bvmm_surface_lock())
       │
       ▼  memcpy to MipOffset
[Texture Object Ready for GPU] -> (bvmm_surface_unlock())
```

---

### 5. Integration Dependencies
- **Phase 4 Surface Manager**: Backing surface creation (`bvmm_surface_create()`).
- **Phase 6 Eviction & Residency Engine (Next)**: BTFE textures will register residency priority hints with Phase 6 for dynamic VRAM/GTT paging.
