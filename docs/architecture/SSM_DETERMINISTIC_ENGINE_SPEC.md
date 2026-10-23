# ATOMS OS — SSM DETERMINISTIC DECISION ENGINE SPECIFICATION (v1)
## Phase 7: Bounded Fixed-Point Task Scoring & Placement Engine

**Subsystem:** Core Scheduler Policy Engine  
**Target:** Kernel Ring 0 (`-ffreestanding -mno-red-zone -mno-sse -msoft-float`)  
**Design Authority:** Section 7 of Master Engineering Directive & Rule 0  
**Status:** SPECIFICATION APPROVED FOR HOST-SIDE VERIFICATION & SHADOW MODE  

---

### 1. Architectural Scope & Policy Boundary

The SSM (State Space Model / Scheduler Scoring Model) Deterministic Decision Engine acts strictly as a **policy evaluator** on top of the existing ATOMS OS scheduler. The core scheduler mechanism—context switching, hardware TSS, runqueue locks (`g_scheduler_lock`), interrupt entry/exit, and task lifecycle transitions—remains 100% untouched.

```
+-------------------------------------------------------------+
|                  CORE SCHEDULER MECHANISM                   |
|  (Locks, RunQueues, Context Switch, TSS, Interrupt Dispatch)|
+-------------------------------------------------------------+
                              │
                    Policy Query Interface
                    [Can be switched/shadowed]
                              ▼
+-------------------------------------------------------------+
|             SSM DETERMINISTIC DECISION ENGINE               |
|  - Fixed-Point Q16.16 Arithmetic (No float/double in Ring 0)|
|  - Zero Dynamic Allocation (No kmalloc/atom_alloc)          |
|  - Pure Reentrant Functions (No global mutable state)       |
|  - Explicit Clamping & Saturation (Zero Division by Zero)   |
|  - Shadow Mode Evaluation & Fail-Closed Fallback            |
+-------------------------------------------------------------+
```

---

### 2. Strict Arithmetic Constraints & Numeric Representation

1. **Numeric Format: Q16.16 Fixed-Point**:
   - Bit 31: Sign bit.
   - Bits 30..16: Integer portion (range: $-32768$ to $+32767$).
   - Bits 15..0: Fractional portion ($1 / 65536 \approx 0.00001525878$).
   - Scale factor: `SSM_FIXED_ONE = 65536 (1 << 16)`.
2. **Saturation & Clamping**:
   - All additions, subtractions, and multiplications detect integer overflow and saturate at `INT32_MAX` (`0x7FFFFFFF`) or `INT32_MIN` (`0x80000000`).
   - Division by zero is explicitly guarded: `if (denominator == 0) return SSM_FIXED_MAX;`.
3. **Zero Floating-Point Invariant**:
   - Zero `float`, zero `double`, zero SSE/AVX registers.
   - The userspace "Bishop Math" library is strictly prohibited and not linked.

---

### 3. Mathematical Scoring Formulation

For any candidate task $T$ considered for dispatch on core $C$:

$$\text{Score}(T, C) = S_{\text{priority}}(T) + S_{\text{aging}}(T) + S_{\text{affinity}}(T, C) + S_{\text{quantum}}(T) - S_{\text{migration\_penalty}}(T, C)$$

Where each term is computed in saturated Q16.16:

1. **Priority Component ($S_{\text{priority}}$)**:
   - Input: $T.\text{base\_priority} \in [0, 31]$.
   - Weight: $W_{\text{prio}} = 1.0$ (`0x00010000`).
   - $S_{\text{priority}} = T.\text{base\_priority} \times W_{\text{prio}}$. Range: $[0, 31.0]$.

2. **Aging & Starvation Component ($S_{\text{aging}}$)**:
   - Input: Wait time in ready queue: $\Delta_{\text{wait}} = \text{now} - T.\text{ready\_since\_tick}$.
   - Formula: Boost factor $B = \min\left(\frac{\Delta_{\text{wait}}}{250}, 31 - T.\text{base\_priority}\right)$.
   - Weight: $W_{\text{aging}} = 1.0$ (`0x00010000`).
   - $S_{\text{aging}} = B \times W_{\text{aging}}$. Range: $[0, 31.0]$.

3. **Cache Locality & Migration Penalty ($S_{\text{affinity}}$ vs $S_{\text{penalty}}$)**:
   - If $T.\text{last\_cpu} == C$:
     - Cache warm bonus: $+2.0$ (`0x00020000`).
   - If $T.\text{last\_cpu} \neq C$ and $T.\text{last\_cpu} \neq \text{NONE}$:
     - Cross-core migration penalty: $-1.5$ (`0x00018000`).

4. **Remaining Quantum Incentive ($S_{\text{quantum}}$)**:
   - Input: $T.\text{quantum} \in [1, 20]$.
   - Incentive: $\frac{T.\text{quantum}}{20} \times 0.5$. Range: $[0, 0.5]$.

5. **Tie-Breaking Rule (Deterministic Invariant)**:
   If $\text{Score}(T_A, C) == \text{Score}(T_B, C)$:
   - Pick task with earlier `ready_since_tick`.
   - If equal, pick task with smaller `id`.
   - **Guarantees: Same State + Same Inputs $\implies$ Exact Identical Decision.**

---

### 4. Non-Negotiable Safety Invariants

1. **Eligibility Filter (Hard Precondition)**:
   SSM evaluates **only** tasks that pass `task_eligible_on_cpu(task, cpu)`:
   - Task must not be null.
   - Task state must be `TASK_READY`.
   - CPU must be set in `task->affinity_mask`.
   - If `task->assigned_cpu` is pinned, it must match $C$.
   - **A high score NEVER overrides eligibility.**
2. **Candidate Evaluation Bound**:
   - The engine inspects at most `SSM_MAX_CANDIDATES = 32` runnable tasks per query.
   - Loop is strictly bounded ($O(N)$ with $N \le 32$). Maximum execution budget: **$\le 5\,\mu\text{s}$**.
3. **Fail-Closed Fallback Invariant**:
   - If queue is empty: return local idle task.
   - If SSM engine is disabled, corrupted, or encounters arithmetic saturation: fall back immediately to standard Round-Robin / Priority-Aging baseline.

---

### 5. Shadow Mode Architecture

Before SSM is granted execution authority, it runs in **Shadow Mode** (`SSM_MODE_SHADOW`):
1. The baseline scheduler selects `best_baseline`.
2. Concurrently, the SSM engine evaluates the candidates and computes `best_ssm`.
3. If `best_baseline == best_ssm`: Increment `g_ssm_telemetry.concordant_decisions`.
4. If `best_baseline != best_ssm`: Increment `g_ssm_telemetry.discordant_decisions`, record delta score.
5. The kernel dispatches `best_baseline` (zero live behavioral alteration).
6. Telemetry is surfaced on the Danger Monitor and via serial log.

---

### 6. Mode Switch States

```c
typedef enum {
    SSM_MODE_DISABLED = 0,    // Pure baseline scheduler (Round-Robin / Aging)
    SSM_MODE_SHADOW,          // SSM calculates score in background; baseline acts
    SSM_MODE_ACTIVE           // SSM decisions actively placed and dispatched
} ATOMS_SSMMode;
```

---

**SPECIFICATION APPROVAL:** Formally documented in [SSM_DETERMINISTIC_ENGINE_SPEC.md](file:///d:/Signatures_OS/docs/architecture/SSM_DETERMINISTIC_ENGINE_SPEC.md).
