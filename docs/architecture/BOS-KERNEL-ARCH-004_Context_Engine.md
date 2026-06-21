# BOS-KERNEL-ARCH-004: Context Engine Architecture

**Status:** PERMANENT FOUNDATION (Frozen)

## 1. Purpose

The Context Architecture is the fundamental mechanism that allows SignaturesOS to pause a currently executing stream of instructions, securely store its exact state, and resume a different stream of instructions. It provides the illusion of concurrency by multiplexing CPU execution units across multiple tasks, threads, and processes.

## 2. Engine Segregation (The 3 Pillars)

To guarantee zero coupling, the overarching concept is strictly divided into three distinct components:

1. **Context Save Engine:** 
   * **Role:** Strictly `CPU Registers` → `Memory`. 
   * **Action:** Moves physical CPU state into the defined Context structure on the stack.
2. **Context Restore Engine:** 
   * **Role:** Strictly `Memory` → `CPU Registers`.
   * **Action:** Moves state from the saved Context structure back into physical hardware.
3. **Context Manager:**
   * **Role:** Context Lifecycle.
   * **Action:** Handles Creation, Initialization, Cloning, and Destruction. It acts as the C-level interface that the Scheduler interacts with.

## 3. Layered Architecture (Portability)

The architecture natively supports future ports (ARM64, RISC-V) by decoupling the generic context concept from hardware-specific layouts:

```text
Context (Generic)
  ↓
ArchContext (Abstract Architecture)
  ↓
x86_64 Context / ARM64 Context / RISCV Context
```

```c
typedef struct {
    void* arch_context; // Opaque pointer to hardware-specific layout
} Context;
```

## 4. CPU Local Data (SMP Readiness) ⭐⭐⭐⭐⭐

To ensure massive scalability and lock-free fast paths in SMP (Symmetric Multiprocessing), global state is eliminated. Every CPU manages its execution independently using **CPU Local Data** (accessed via `GS Base` on x86_64):

```text
CPU 0                          CPU 1
↓                              ↓
Local Current Task             Local Current Task
Local Idle Task                Local Idle Task
Local Scheduler Data           Local Scheduler Data
Local TSS                      Local TSS
GS Base pointer                GS Base pointer
```
*This eliminates global locks during context saving/restoring and guarantees thread-safe interrupt nesting per core.*

## 5. Ownership Boundaries

* **Hardware:** Triggers transitions.
* **Assembly Layer:** Only executes the Context Save/Restore Engines (moving bytes).
* **Context Manager:** Owns the memory layout definition (`ArchContext`).
* **Scheduler:** Decides policy and delegates state switching to the Context Manager.

```text
Hardware  →  Assembly  →  Context Engine  →  Scheduler
```

## 6. Zero-Copy Design & Task Relationship

**Decision:** Context is **Referenced**, not Embedded.

```text
Task  →  RSP  →  Context on Stack
```
The `Context` lives entirely on the Task's kernel stack. The `Task` structure only holds a `uint64_t rsp;` which points to the top of the saved `Context`. This is zero-copy, lightning-fast, and mirrors the philosophy of Linux, FreeBSD, and xv6.

## 7. Future Compatibility & Naming Evolution

The architecture is designed to naturally evolve its nomenclature as the OS matures:

```text
Task  →  Kernel Thread  →  User Thread  →  Process  →  CPU
```
Currently using `Task`, this will seamlessly rename to `Thread` as Process boundaries (CR3 isolation) are introduced.

## 8. BOS Rules for Context Management

* **Rule 41:** The Context Engine shall never dynamically allocate or free memory. It operates exclusively on pre-existing stacks.
* **Rule 42:** The `Context` struct definition MUST exactly mirror the layout of data pushed onto the stack by hardware and assembly stubs.
* **Rule 43:** Assembly stubs must never inspect the contents of the `Context`; they only move bytes between registers and the stack pointer.
* **Rule 44:** Context switching must be an entirely zero-copy operation; state is pushed, the stack pointer changes, and state is popped.
* **Rule 45:** A Context object shall never be directly modified by the Scheduler. Only the Context Manager may create, save, restore, or destroy Contexts.
* **Rule 46:** Assembly code shall contain zero scheduling policy. Assembly only moves CPU state. All decisions remain in C.

---
**Document Status:** Approved & Frozen. Proceed to next architectural phases. Do not alter without CTO-level architectural review.
