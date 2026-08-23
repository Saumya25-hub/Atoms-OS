# ATOMS OS — WINDOW RESIZE DOUBLE #PF PATCH PLAN

## 1. Plan Overview & Objectives
- **Objective**: Eliminate Double `#PF` Page Fault and Page Table corruption during window resizing across all applications (File Explorer, Settings, Terminal, Calculator, etc.).
- **Scope**: Kernel Heap VMM binding, Page Table User/Supervisor flag isolation, Surface Reallocation throttling/buffering, and exception handler safety.

---

## 2. Modifications Breakdown

### A. Kernel Heap Master PML4 Enforcement
1. **Target File**: `kernel/core/memory/heap/src/heap.c`
   - **Functions**: `heap_expand_locked()`, `heap_init()`
   - **Modification**: Replace `vmm_get_active_pml4()` with `vmm_get_kernel_pml4()`.
   - **Why**: Kernel Heap (`0xC0000000ULL`) belongs to Ring 0 kernel address space. Heap pages must ALWAYS be mapped into `g_kernel_pml4` so all CPU cores and context switches see a consistent, permanent heap mapping.

### B. Correct User/Supervisor Paging Classification
1. **Target File**: `kernel/core/memory/vmm/src/paging.c`
   - **Functions**: `vmm_get_pt_entry()`
   - **Modification**: Fix the `is_user` check so that Kernel Heap (`0xC0000000ULL`), Kernel Code, Framebuffers (`0x80000000`), and Kernel Stacks are NEVER tagged as `PAGE_USER`.
   - **Why**: Prevents user-accessible bits and table corruption on kernel internal page directory structures.

### C. Surface Reallocation Capacity Throttling / Hysteresis
1. **Target File**: `kernel/wm/bwe/renderer/bwe_compositor.c`
   - **Functions**: `Surface_CreateForWindow()`
2. **Target File**: `kernel/wm/bwe/src/bwe_window.c`
   - **Functions**: `BOS_SurfacePresent()`
   - **Modification**: Only reallocate surface memory when the new dimensions exceed the current buffer's `memory_size` capacity. Round allocation requests up to 64/128-pixel boundaries.
   - **Why**: Eliminates hundreds of 2MB `kfree`/`kmalloc` cycles during smooth drag/resize gestures, reducing heap churn by > 98%.

### D. Safe Physical Page Table Traversal in Exception Handler
1. **Target File**: `kernel/core/interrupt/src/exception.c`
   - **Functions**: `exception_dispatch()`
   - **Modification**: Validate physical address bounds before attempting to dereference page table physical pointers during fault diagnostics.
   - **Why**: Prevents a secondary `#PF` when diagnostic routines inspect page table entries pointing to memory frames outside the 1:1 identity map.

---

## 3. Expected Result
- Window resizing across Explorer, Settings, Terminal, and Calculator executes smoothly without `#PF` faults.
- Zero page table divergence between user PML4 and kernel PML4.
- Memory allocation during resize is stable, fast, and leak-free.

---

## 4. Verification & Certification Plan
1. **Compilation**: Clean build via `./build.ps1` with zero errors.
2. **QEMU Pure UEFI Pre-Flight**: Boot in QEMU UEFI mode with ABDE diagnostic validation.
3. **VMware Multi-Window Test**:
   - Open Explorer, Settings, Terminal, and Calculator.
   - Perform slow and rapid continuous resizing on all 4 windows.
   - Verify zero `#PF` / `#GP` faults and zero vCPU shutdown events.
