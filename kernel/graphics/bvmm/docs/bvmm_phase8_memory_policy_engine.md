# BOS VRAM Memory Manager (BVMM) Phase 8 Architecture
## Production Memory Policy, Residency & Eviction Engine (BMPRE V1.0) Specification

### 1. Overview & Architectural Role
The **BOS Memory Policy, Residency & Eviction Engine (BMPRE V1.0)** is the intelligent memory policy authority of the BOS GPU stack. It continuously monitors VRAM pressure, budget limits, working set footprints, priority classes, fence readiness, and resource access aging scores to make deterministic residency, eviction, and migration decisions.

```
+-------------------------------------------------------+
|  Applications / OpenGL / BOCompositor / Decoders      |
+-------------------------------------------------------+
                           |
                           v
+-------------------------------------------------------+
|  Memory Policy, Residency & Eviction Engine (BMPRE)   |
|  - Global Residency Manager (VRAM, GTT, System, etc)  |
|  - VRAM Budget Engine (Total, Resident, Free, Peak)   |
|  - Memory Pressure Engine (Normal, Low -> Emergency)  |
|  - Eviction Policy Engine (Priority Candidate Scoring)|
|  - Working Set Manager & Resource Aging Engine        |
|  - Migration Scheduler Queue                          |
|  - Memory Advisor Output Engine                       |
+-------------------------------------------------------+
                           |
                           v
+-------------------------------------------------------+
|  Phase 7 Fence & Synchronization Engine               |
+-------------------------------------------------------+
                           |
                           v
+-------------------------------------------------------+
|  Phase 6 Reference, Residency & Lifetime Engine       |
+-------------------------------------------------------+
```

---

### 2. Eviction Candidate Scoring Formula
Resources are selected for eviction strictly based on candidate score ranking. Higher scores are evicted first:

$$\text{CandidateScore} = \text{Age} + (6 - \text{Priority}) \times 1000 - \text{RefCount} \times 100$$

> [!IMPORTANT]
> **Immunity Invariant**: Resources with `PinState != UNPINNED` (e.g. `SCANOUT`, `CURSOR`, `SYSTEM_PIN`) or resources with pending timeline fences (`!fence.is_signaled`) have $\text{CandidateScore} = -\infty$ and are strictly non-evictable.

---

### 3. Memory Pressure Levels Matrix

| Pressure Level | Trigger Conditions | Policy Action |
| :--- | :--- | :--- |
| **NORMAL** | Free VRAM $> 60\%$ | Standard residency in VRAM |
| **LOW** | Free VRAM $40\% - 60\%$ | Stream background assets to GTT |
| **MEDIUM** | Free VRAM $25\% - 40\%$ | Evict cold streaming buffers |
| **HIGH** | Free VRAM $15\% - 25\%$ | Evict low priority textures |
| **CRITICAL** | Free VRAM $5\% - 15\%$ | Evict all non-essential resources |
| **EMERGENCY** | Free VRAM $< 5\%$ | Reclaim all idle working set memory |

---

### 4. Integration Dependencies
- **Phase 7 Sync Engine (BFSE)**: Pending fence query before eviction (`bfse_wait()`).
- **Phase 6 Lifetime Engine (BRRLE)**: Priority and pin status query (`brrle_pin()`).
- **Phase 9 Eviction & Defragmentation Controller (Next)**: Phase 9's defragmenter will invoke BMPRE's candidate scoring to execute live VRAM defragmentation.
