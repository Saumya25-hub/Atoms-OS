# ATOMS OS — SSM ADAPTIVE / NEURAL ENGINE SPECIFICATION (v1)
## Phase 8: Telemetry-Driven Adaptive Parameter Scaling & Policy Optimization

**Subsystem:** Core Scheduler Adaptive Governor  
**Target:** Kernel Ring 0 (`-ffreestanding -mno-red-zone -mno-sse -msoft-float`)  
**Design Authority:** Section 8 of Master Engineering Directive & Rule 0  
**Entry Gate:** Ph6 & Ph7 Formal Acceptance Required for Live Runtime Activation  
**Status:** SPECIFICATION, OFFLINE HARNESS, AND PRE-REGISTERED BENEFIT BAR DEFINED  

---

### 1. Architectural Model & Safety Hierarchy

The Adaptive Engine operates strictly as an **outer-loop supervisory governor**. It never dispatches tasks, never touches context switches, and never makes direct placement choices. It strictly observes system-wide queue pressure, runqueue wait distributions, and CPU utilization over a bounded observation window (e.g. 500 ms) and computes clamped tuning parameters for the underlying **Phase 7 SSM Deterministic Engine**:

```
+─────────────────────────────────────────────────────────────+
|            LAYER 0: PHYSICAL SILICON & INTERRUPTS           |
+─────────────────────────────────────────────────────────────+
                              ▲
                              │ Mechanism (Untouched)
+─────────────────────────────────────────────────────────────+
|            LAYER 1: CORE SCHEDULER DISPATCH ENGINE          |
|    (Locks, Context Switch, RunQueues, TSS, Idle Loop)       |
+─────────────────────────────────────────────────────────────+
                              ▲
                              │ Policy Decision (Single-digit μs)
+─────────────────────────────────────────────────────────────+
|        LAYER 2: SSM DETERMINISTIC ENGINE (PHASE 7)          |
|    (Bounded Q16.16 Task Scoring, Eligibility Invariants)    |
+─────────────────────────────────────────────────────────────+
                              ▲
                              │ Clamped Parameter Updates (500 ms)
+─────────────────────────────────────────────────────────────+
|         LAYER 3: SSM ADAPTIVE ENGINE (PHASE 8)              |
|  - Integer-Quantized Inference / EWMA Statistics            |
|  - Pre-Registered Benefit Bar: >= 15% Improvement           |
|  - Fail-Closed: Automatically Reverts to Deterministic SSM  |
+─────────────────────────────────────────────────────────────+
```

---

### 2. Pre-Registered Benefit Bar & Acceptance Criteria

Under Section 8.3 of the Master Directive:
> **Pre-registered benefit bar:** Adopt a learned model only if it shows a **repeatable, measured improvement (target $\ge 15\%$ on the chosen scheduling latency / throughput metric)** over deterministic SSM, after accounting for its runtime cycle and memory cost. Otherwise keep it disabled/rejected and say so.

1. **Target Benchmark Workload**:
   - 8 compute-bound concurrent tasks (`SMP_Worker_0..7`).
   - 8 high-frequency ping-pong IPC tasks.
   - Contention churn: 10,000 context switches under 8-core concurrency.
2. **Key Metric**:
   - Mean Scheduling Turnaround Latency (cycles from `ready_since_tick` to first dispatch).
3. **Threshold**:
   - If Adaptive Model Mean Latency $\le 0.85 \times \text{Deterministic Baseline}$ (a $15\%$ improvement or greater), the model passes the gate.
   - If improvement is $< 15\%$, or if inference overhead exceeds the gain, **the learned model is rejected**, and the system defaults to Deterministic SSM.

---

### 3. Model Architecture & Offline Training Provenance

1. **Offline Training (Python Host Environment)**:
   - Training runs strictly offline in `tools/ssm/train_evaluate_ssm_adaptive.py` using genuine recorded runtime telemetry traces (`build/uefi_smp8.log` and `build/freebsd_hypervisor_serial.log`).
   - Synthetic or fabricated traces are strictly prohibited.
2. **Quantized Integer Inference (Ring 0 Safe)**:
   - Inputs: 4 normalized telemetry features:
     1. $F_0$: Runqueue ready depth (clamped $0..32$).
     2. $F_1$: Mean wait time in ready queue ($0..1000\text{ ms}$).
     3. $F_2$: CPU utilization percentage ($0..100\%$).
     4. $F_3$: Lock contention rate ($0..100\%$).
   - Static Weights: 8-bit signed integer quantized matrix ($4 \times 4 \times 2$).
   - Total arithmetic budget: 32 integer MAC (multiply-accumulate) operations.
   - Zero dynamic allocation, zero floating-point, zero division.

---

### 4. Bounded Output Clamping & Rollback Invariants

Any parameter output suggested by the adaptive layer must be strictly clamped before applying to Layer 2:
- Weight $W_{\text{prio}}$: Clamped to $[0.5, 2.0]$ in Q16.16 (`0x00008000` to `0x00020000`).
- Weight $W_{\text{wait}}$: Clamped to $[0.5, 2.5]$ in Q16.16 (`0x00008000` to `0x00028000`).
- Locality Bonus $W_{\text{loc}}$: Clamped to $[1.0, 3.0]$ in Q16.16 (`0x00010000` to `0x00030000`).
- Max adjustment per interval: $\le 10\%$ delta from current value (anti-flapping damping).
- Automatic Fail-Closed: If telemetry age $> 1000\text{ ms}$ or inference produces an out-of-range value, parameters instantly reset to static defaults ($W_{\text{prio}} = 1.0, W_{\text{wait}} = 1.0, W_{\text{loc}} = 2.0$).

---

**SPECIFICATION APPROVAL:** Formally documented in [SSM_ADAPTIVE_ENGINE_SPEC.md](file:///d:/Signatures_OS/docs/architecture/SSM_ADAPTIVE_ENGINE_SPEC.md).
