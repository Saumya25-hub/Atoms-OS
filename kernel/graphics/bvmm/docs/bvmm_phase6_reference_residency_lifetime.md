# BOS VRAM Memory Manager (BVMM) Phase 6 Architecture
## Production Reference, Residency & Lifetime Engine (BRRLE V1.0) Specification

### 1. Overview & Architectural Role
The **BOS Reference, Residency & Lifetime Engine (BRRLE V1.0)** is the permanent GPU resource lifetime authority of BOS. It synchronizes CPU/GPU reference counters, validates residency state machine transitions, enforces resource pinning, tracks migration latency, and integrates timeline fences across all GPU objects (Textures, Surfaces, Buffers, Render Targets, Shader Resources, and Video Surfaces).

```
+-------------------------------------------------------+
|  Applications / OpenGL / BOCompositor / Decoders      |
+-------------------------------------------------------+
                           |
                           v
+-------------------------------------------------------+
|  Reference, Residency & Lifetime Engine (BRRLE V1.0)  |
|  - Global Lifetime Registry (64-Bit Lifetime ID + Gen) |
|  - Dual Reference Engine (CPU & GPU Independent Refs) |
|  - Residency State Machine (VRAM, GTT, System, Evicted)|
|  - Resource Pinning Engine (Scanout, Cursor, System)  |
|  - Timeline Fence Integration & Migration Tracker     |
+-------------------------------------------------------+
                           |
                           v
+-------------------------------------------------------+
|  Phase 5 BOS Texture & Format Engine (BTFE)           |
+-------------------------------------------------------+
                           |
                           v
+-------------------------------------------------------+
|  Phase 4 BOSurface Manager Engine (BSME)              |
+-------------------------------------------------------+
```

---

### 2. Dual CPU/GPU Reference Synchronization Engine
Resource destruction is strictly deferred as long as any active reference or pending timeline fence exists:

$$\text{DestructionAllowed} = (\text{CPU\_RefCount} == 0) \land (\text{GPU\_RefCount} == 0) \land (\text{Fence}_{\text{Signaled}} \lor \neg\text{Fence}_{\text{Attached}})$$

```
  [brrle_cpu_release()]              [brrle_gpu_release()]           [brrle_fence_signal()]
          │                                  │                                  │
          ▼                                  ▼                                  ▼
  CPU_RefCount == 0                 GPU_RefCount == 0                 Fence Signaled == True
          │                                  │                                  │
          └──────────────────────────────────┼──────────────────────────────────┘
                                             │
                                             ▼
                          [Zero-Ref Auto Destruction Triggered]
```

---

### 3. Residency State Machine

Every GPU resource transitions strictly through legal residency states. Illegal transitions (e.g. evicting a scanout-pinned surface) are rejected.

$$\text{Created} \longrightarrow \text{Resident} \longrightarrow \text{Mapped} \longrightarrow \text{Referenced} \longrightarrow \text{Idle} \longrightarrow \text{Migration Pending} \longrightarrow \text{Migrating} \longrightarrow \text{Evicted} \longrightarrow \text{Destroyed}$$

| Residency | Description |
| :--- | :--- |
| **VRAM** | High-bandwidth dedicated video memory |
| **GTT** | Host system memory mapped into GPU aperture |
| **SYSTEM_MEMORY** | CPU main system memory |
| **UPLOAD** | CPU $\rightarrow$ GPU write-combined staging pool |
| **READBACK** | GPU $\rightarrow$ CPU cached readback pool |
| **EVICTED** | Paged out to disk / secondary storage |

---

### 4. Integration Dependencies
- **Phase 5 Texture Engine**: Texture lifetime registration.
- **Phase 4 Surface Manager**: Surface lifetime registration.
- **Phase 7 Eviction & Defragmentation Engine (Next)**: Phase 7's eviction controller will consume BRRLE priority classes (`CRITICAL`, `HIGH`, `NORMAL`, `LOW`, `BACKGROUND`, `STREAMING`) and pin states to execute live VRAM defragmentation.
