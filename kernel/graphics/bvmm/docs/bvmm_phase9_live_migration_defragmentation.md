# BOS VRAM Memory Manager (BVMM) Phase 9 Architecture
## Production Live Migration & Defragmentation Engine (BLMDE V1.0) Specification

### 1. Overview & Architectural Role
The **BOS Live Migration & Defragmentation Engine (BLMDE V1.0)** is the permanent GPU Memory Movement Authority of BOS. It performs live VRAM relocation, online heap compaction, zero-downtime migration, atomic pointer fix-up, fragmentation recovery, and relocation rollback while applications continue running.

```
+-------------------------------------------------------+
|  Applications / OpenGL / BOCompositor / Decoders      |
+-------------------------------------------------------+
                           |
                           v
+-------------------------------------------------------+
|  Memory Policy Engine (BMPRE V1.0)                    |
+-------------------------------------------------------+
                           |
                           v
+-------------------------------------------------------+
|  Fence & Synchronization Engine (BFSE V1.0)           |
+-------------------------------------------------------+
                           |
                           v
+-------------------------------------------------------+
|  Live Migration & Defragmentation Engine (BLMDE)      |
|  - Global Migration Job Manager ($O(1)$ Lookup)       |
|  - 9-Step Zero-Downtime Migration Pipeline          |
|  - Atomic Pointer Fix-up Engine (BTFE, BSME, BRRLE)   |
|  - Relocation Rollback Engine (Pre-Swap Recovery)     |
|  - Online TLSF + Range Heap Compaction Engine         |
|  - Fragmentation Analyzer Engine                      |
+-------------------------------------------------------+
                           |
                           v
+-------------------------------------------------------+
|  Phase 6 Reference, Residency & Lifetime Engine       |
+-------------------------------------------------------+
```

---

### 2. Zero-Downtime 9-Step Migration Pipeline

```
  1. Acquire Fence (bfse_fence_create())
           │
           v
  2. Freeze Resource (brrle_pin(BRRLE_PIN_TEMPORARY))
           │
           v
  3. Allocate Destination Memory
           │
           v
  4. Copy Memory (GPU DMA Copy)
           │
           v
  5. Validate Copy Integrity
           │
           v
  6. Atomic Pointer Swap (blmde_fixup_pointers())  <─── [ Rollback Point ]
           │
           v
  7. Update Registries (brrle_transition_residency())
           │
           v
  8. Release Fence & Unfreeze (bfse_signal())
           │
           v
  9. Free Old Allocation & Complete
```

---

### 3. Supported Relocation Paths

| Source | Destination | Description |
| :--- | :--- | :--- |
| **VRAM** | **VRAM** | Online heap compaction and defragmentation |
| **VRAM** | **GTT** | Demoting cold assets to GTT aperture |
| **VRAM** | **SYSTEM** | Evicting inactive resources to RAM |
| **GTT** | **VRAM** | Promoting active streaming assets to fast VRAM |
| **SYSTEM** | **VRAM** | Paging in evicted objects back to VRAM |
| **UPLOAD** | **VRAM** | Flushing staging upload buffers to VRAM |

---

### 4. Integration Dependencies
- **Phase 8 Policy Engine (BMPRE)**: Migration trigger notifications.
- **Phase 7 Sync Engine (BFSE)**: Timeline fence acquisition during migration.
- **Phase 10 Vendor HAL Engine (Next)**: Phase 10 driver HAL modules will hook into BLMDE's relocation pipeline to trigger hardware DMA copy engines.
