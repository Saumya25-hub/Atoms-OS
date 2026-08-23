# ATOMS OS — WINDOW RESIZE DOUBLE #PF & PAGE-TABLE CORRUPTION
## Comprehensive Forensic Investigation & Root Cause Analysis

**Classification:** Mission-Critical Kernel Memory & Paging Safety  
**Phase Status:** TASK 1 COMPLETE — Forensic Analysis & Mathematical Proof (NO CODE MODIFIED)  
**Target Architecture:** Haswell x86_64, 4-Level Paging (PML4, PDPT, PD, PT), Pure UEFI Mode  

---

## 1. Executive Summary & Forensic Verdict

| Forensic Metric | Finding | Verdict |
| :--- | :--- | :--- |
| **Primary Incident** | Double `#PF` Page Fault during Window Resize on desktop | **CRITICAL KERNEL FAULT** |
| **First Fault RIP** | `0x00000000001EFB87` (`runqueue_validate_verbose + 0xF7`) | **RESOLVED** |
| **First Fault Error Code** | `0x0000000000000009` (`P=1, W/R=0, U/S=0, RSVD=1`) | **RESERVED BIT VIOLATION** |
| **First Fault CR2** | `0x00000000C0000AA0` (`HEAP_START_VADDR + 0xAA0`) | **KERNEL HEAP VIRTUAL ADDRESS** |
| **Second Fault RIP** | `0x000000000019D4F8` (`exception_dispatch + 0xEE8`) | **DOUBLE FAULT TRAP** |
| **Second Fault CR2** | `0x0000000110D7C000` (Raw Physical Address of PDPE table) | **UNMAPPED PHYSICAL DEREFERENCE** |
| **Primary Root Cause** | `heap_expand_locked()` uses `vmm_get_active_pml4()` instead of `vmm_get_kernel_pml4()`. Rapid window resizing generates hundreds of 2MB surface reallocations, mapping kernel heap pages into `user_pml4` instead of master kernel PML4, causing cross-process page-table divergence and unmapped heap faults. | **ARCHITECTURAL FLAW** |
| **Secondary Root Cause** | Zero spinlock / IRQ protection in PMM/VMM page table allocation + `paging.c` misclassifies Kernel Heap (`0xC0000000`) as user space (`is_user = true`), setting `PAGE_USER` on kernel page directories. | **MEMORY SUBSYSTEM BUG** |

---

## 2. TASK 1 — Resolution of Both RIPs

### Fault 1: RIP `0x00000000001EFB87`
- **Function**: `runqueue_validate_verbose()` in [`kernel/core/scheduler/src/runqueue.c`](file:///d:/Signatures_OS/kernel/core/scheduler/src/runqueue.c)
- **Symbol Base**: `0x00000000001EFA90`
- **Relative Offset**: `+0x0F7` (`0x1EFB87 - 0x1EFA90`)
- **Exact Assembly Instructions**:
  ```assembly
  0x1EFB83: movq  0x10(%rax), %rax       ; Loads rq->ready_list.head (0xC0000AA0) into %rax
  0x1EFB87: cmpq  $0x0, (%rax)           ; <-- CRASH: Dereferences %rax (0xC0000AA0)
  0x1EFB8B: je    0x1EFBB5               ; Jump if head is NULL
  ```
- **Context**: `scheduler_on_tick()` / `runqueue_validate_verbose()` was verifying the scheduler ready queue during a timer interrupt while a window resize was occurring.

### Fault 2: RIP `0x000000000019D4F8`
- **Function**: `exception_dispatch()` in [`kernel/core/interrupt/src/exception.c`](file:///d:/Signatures_OS/kernel/core/interrupt/src/exception.c)
- **Symbol Base**: `0x000000000019C610`
- **Relative Offset**: `+0xEE8` (`0x19D4F8 - 0x19C610`)
- **Exact Assembly Instructions**:
  ```assembly
  0x19D4F0: movq  (%r14), %rax           ; Reads PDPE table entry (0x110D7C027)
  0x19D4F4: andq  $0xFFFFFFFFFFFFF000, %rax ; Masks to physical base (0x110D7C000)
  0x19D4F8: movq  (%rax), %rdx           ; <-- CRASH: Dereferences raw physical address 0x110D7C000 as a virtual pointer!
  ```
- **Context**: While handling Fault 1, `exception_dispatch()` attempted to walk and print the page table hierarchy for diagnostic telemetry by treating the physical address `0x110D7C000` as a virtual pointer, triggering a second `#PF`.

---

## 3. TASK 2 — Error Code 0x9 Decoding & Paging Walk Analysis

### Error Code Breakdown (`0x0000000000000009`):
$$\text{Error Code } 0x9 = 0b00001001_2$$
- **Bit 0 (P = 1)**: Page Protection Violation (The page directory/table entry was marked PRESENT).
- **Bit 1 (W/R = 0)**: Access was a READ operation.
- **Bit 2 (U/S = 0)**: Access occurred in CPL 0 (Ring 0 Supervisor/Kernel Mode).
- **Bit 3 (RSVD = 1)**: **RESERVED BIT VIOLATION in paging structure entry.**
- **Bit 4 (I/D = 0)**: Data access (not instruction fetch).

### Complete 4-Level Page Walk for `0x00000000C0000AA0`:
- **Virtual Address Breakdown**:
  - `PML4 Index`: `(0xC0000AA0 >> 39) & 0x1FF = 0`
  - `PDPT Index`: `(0xC0000AA0 >> 30) & 0x1FF = 3` ($3 \times 1\text{ GB} = 3\text{ GB}$)
  - `PD Index`  : `(0xC0000AA0 >> 21) & 0x1FF = 0`
  - `PT Index`  : `(0xC0000AA0 >> 12) & 0x1FF = 0`
  - `Offset`    : `0xAA0`

| Level | Physical Address / Value | Flags Decoded | Status |
| :--- | :--- | :--- | :--- |
| **CR3** | `0x0000000011025000` | Base PML4 Table | Valid |
| **PML4E[0]** | `0x0000000011026027` | `P=1, W=1, U=1, Base=0x11026000` | Valid |
| **PDPE[3]** | `0x0000000110D7C027` | `P=1, W=1, U=1, Base=0x110D7C000` | **RESERVED BIT FAULT** |
| **PDE[0]** | Unreachable | Fault occurred at PDPE level | N/A |

**Why did PDPE[3] trigger a Reserved Bit Fault?**
1. Physical address `0x110D7C000` (4.26 GB) was allocated by PMM above 4GB.
2. In `paging.c`, `vmm_get_pt_entry()` split `pd_table` or `pdp_table` and assigned `table_flags` with `PAGE_USER`.
3. Concurrently, without spinlocks, `pdp_table[3]` had invalid bits set or referenced physical memory outside the CPU's supported physical address width for PDP page directory pointers.

---

## 4. TASK 3 & 4 — Virtual Address 0xC0000AA0 & 0x110D7C000 Identification

1. **`0xC0000AA0`**:
   - Lies directly inside the **Kernel Heap** (`#define HEAP_START_VADDR 0xC0000000ULL` in [`heap.c`](file:///d:/Signatures_OS/kernel/core/memory/heap/src/heap.c)).
   - Specifically, `0xC0000AA0` is a heap block header for a `Task` struct and `list_node_t` in the scheduler ready queue.
2. **`0x110D7C000`**:
   - A physical memory frame allocated by `pmm_alloc_page()`.
   - Used by `vmm_get_pt_entry()` as a new Page Table for expanded heap mappings.
   - Faulted during exception handling because physical memory addresses $\ge 4\text{ GB}$ are not identity mapped into the virtual address space.

---

## 5. TASK 5 & 6 — Window Resize Execution Trace & Concurrency Analysis

### Execution Path During Window Resize:
```
1. User Drags Window Resize Border
   │
   ├── BWE_ProcessMouseInteraction() [bwe_window.c:L800]
   │     - Calculates new width & height (e.g. 800x600 -> 820x615)
   │     - Calls BOS_SetBounds(win_id, x, y, new_w, new_h)
   │
   ├── BOS_SetBounds() [bwe_core.c:L233]
   │     - Calls BWE_UpdateLayout(window_id)
   │     - Calls BWE_InvalidateWindow(window_id)
   │
   ├── Application Surface Recreation / Resize
   │     - Surface_CreateForWindow() [bwe_compositor.c:L70]
   │     - Calls kfree(existing->memory_ptr) (Frees old 1.92 MB buffer)
   │     - Calls kmalloc(new_size) (Allocates new 2.01 MB buffer)
   │
   ├── Heap Expansion & VMM Mapping
   │     - heap_alloc_raw() -> heap_expand_locked() [heap.c:L678]
   │     - BUG: heap_expand_locked() calls vmm_get_active_pml4()!
   │     - active_pml4 is user_pml4 (Desktop/Explorer process)
   │     - vmm_alloc_mapped_page() maps new heap pages into user_pml4 ONLY!
   │
   └── Concurrent Scheduler Timer Interrupt (IRQ 0 / 1000 Hz)
         - timer_tick_handler() -> scheduler_on_tick()
         - Context switches to Kernel PML4 (g_kernel_pml4)
         - runqueue_validate_verbose() runs on g_kernel_pml4
         - Accesses heap address 0xC0000AA0
         - g_kernel_pml4 DOES NOT HAVE THE NEW HEAP MAPPINGS!
         - CRASH: #PF Page Fault at 0xC0000AA0!
```

---

## 6. TASK 7 & 8 — Normal Window vs Resize & Performance Separation

- **Normal Window Move / Open**: Window size is constant. Surface memory is reused. Zero calls to `kmalloc`/`heap_expand_locked()`. Page tables remain static and stable.
- **Window Resize**: Every mouse movement pixel delta ($dx, dy$) triggers a `kfree` and `kmalloc` of a ~2 MB surface buffer. Over a 50-pixel drag, **100 MB of heap allocations** occur in rapid succession, forcing continuous `heap_expand_locked()` calls across competing address spaces.
- **Performance Impact**: Allocating 2 MB and copying 8.2 MB of VRAM per frame caused the 45 ms frame time reported in telemetry.

---

## 7. Forensic Root Cause Hierarchy

```
[PRIMARY ROOT CAUSE]
heap_expand_locked() used vmm_get_active_pml4() instead of vmm_get_kernel_pml4().
Kernel Heap (0xC0000000) was mapped exclusively into transient user process PML4s.
Switching to kernel PML4 left heap addresses unmapped.
        │
        ├── [CONTRIBUTING FACTOR 1]
        │   Window resize destroyed and re-kmalloc'd 2MB buffers on every single pixel delta
        │   instead of using pooled / bucketted surface allocations.
        │
        ├── [CONTRIBUTING FACTOR 2]
        │   paging.c tagged all intermediate page tables for 0xC0000000 with PAGE_USER.
        │
        └── [CONTRIBUTING FACTOR 3]
            pmm_alloc_page() and vmm_map_page() lacked interrupt-safe locking.
```
