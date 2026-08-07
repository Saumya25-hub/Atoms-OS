# BOS VRAM Memory Manager (BVMM) Phase 4 Architecture
## BOSurface Manager Engine (BSME V1.0) Specification

### 1. Overview & Architectural Role
The **BOSurface Manager Engine (BSME V1.0)** is the central GPU surface authority inside BVMM. It operates directly above the Production Memory Pool Engine (PMPE) and establishes surface lifetime management, reference counting, locking/unlocking, state machine validation, cloning, resizing, and forensic diagnostics for upper-layer consumers (BOCompositor, Window Engine, and future Texture Engine).

```
+-------------------------------------------------------+
|  Applications / BOCompositor / Window Engine          |
+-------------------------------------------------------+
                           |
                           v
+-------------------------------------------------------+
|       BOSurface Manager Engine (BSME V1.0)            |
|  - Surface Registry & 64-Bit Surface ID Generation    |
|  - Reference Counting Engine (Auto Zero-Ref Destroy)  |
|  - Surface Lock Engine (Lock / Unlock / TryLock)      |
|  - State Machine Enforcement                          |
+-------------------------------------------------------+
                           |
                           v
+-------------------------------------------------------+
|       Production Memory Pool Engine (PMPE)            |
+-------------------------------------------------------+
                           |
                           v
+-------------------------------------------------------+
|       Phase 2 Hybrid Heap Manager (TLSF + Range)     |
+-------------------------------------------------------+
```

---

### 2. Surface Lifetime State Machine

Every surface transitions strictly through legal states. Illegal transitions trigger validation rejections.

$$\text{Created} \longrightarrow \text{Allocated} \longrightarrow \text{Registered} \longrightarrow \text{Ready} \longrightarrow \text{Referenced} \longrightarrow \text{Locked} \longrightarrow \text{Unlocked} \longrightarrow \text{Released} \longrightarrow \text{Destroyed}$$

| State | Description |
| :--- | :--- |
| **CREATED** | Surface descriptor memory allocated; parameters initialized |
| **ALLOCATED** | PMPE memory pool allocation succeeded; valid 64-bit handle obtained |
| **REGISTERED**| Assigned unique 64-bit Surface ID in thread-safe surface registry |
| **READY** | Surface fully prepared for GPU / CPU rendering |
| **REFERENCED**| Active software/hardware reference count $> 1$ |
| **LOCKED** | CPU mapping active (`Lock()` or `TryLock()`) |
| **UNLOCKED** | CPU mapping unmapped (`Unlock()`) |
| **RELEASED** | Zero references remaining; memory return initiated |
| **DESTROYED** | PMPE allocation released; registry slot cleared |

---

### 3. Key APIs & Surface Operations

- `bvmm_surface_create(info, &surface_id)`: Requests backing memory strictly through PMPE.
- `bvmm_surface_destroy(surface_id)`: Releases surface memory to PMPE and unregisters ID.
- `bvmm_surface_lookup(surface_id, &desc)`: Thread-safe descriptor lookup.
- `bvmm_surface_clone(src_id, &cloned_id)`: Allocates new backing surface with identical properties.
- `bvmm_surface_resize(surface_id, width, height)`: Dynamically re-allocates PMPE backing memory.
- `bvmm_surface_ref_inc()` / `bvmm_surface_ref_dec()`: Automatic zero-ref lifecycle cleanup.
- `bvmm_surface_lock()` / `bvmm_surface_unlock()` / `bvmm_surface_trylock()`: Safe CPU BAR mapping.

---

### 4. Integration Dependencies
- **Phase 2 Hybrid Heap Manager**: Underpinning range allocators.
- **Phase 3 PMPE**: Classified memory pools (`Small`, `Medium`, `Large`, `Huge`, `Transient`, `Upload`, `Readback`).
- **Phase 5 Texture Manager (Next)**: BSME surfaces form the direct backing store for texture descriptors, mipchains, and swizzling layouts.
