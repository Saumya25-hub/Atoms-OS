# Risk Analysis

## 1. The "Union" God Object Memory Waste
**Problem**: The `BWE_Window` struct uses a massive `union { ... } control_data` containing arrays like `char items[16][64]` for ListViews.
**Impact**: Because it's a union, *every single control* (even a 10x10 pixel simple label) allocates the maximum possible size of that union.
**Risk**: Massive memory bloat. 1024 slots * sizeof(BWE_Window) is permanently allocated in the BSS section.

## 2. Hardcoded Maximums
- `BWE_MAX_CHILDREN 128`
- `BOCOMPOSITOR_MAX_SURFACES 128`
- `BWE_MAX_WINDOWS 1024`
**Risk**: A complex UI (like a file explorer with many icons) will easily blow past 128 children, causing silent failures or memory corruption if array bounds aren't strictly checked when adding to `children[]`.

## 3. Concurrency / Race Conditions
**Problem**: `g_update_lock` is defined but rarely used consistently across all APIs.
**Risk**: If a background thread (e.g., a network download finishing) calls `BWE_InvalidateWindow()` exactly while `BOCompositor_ComposeFrame()` is scanning the dirty rectangles, the damage array will be corrupted, leading to screen tearing or a hard page fault.

## 4. Circular Dependencies
**Problem**: `BWE` relies on `BOSurface`, but `BOSurface` depends on `bocompositor.h` and even includes `../Apps/terminal.h` and `kernel/conhost/conhost.h`.
**Risk**: Extreme spaghetti code. `BOSurface` is the lowest level of the UI stack, yet it is #including high-level applications. This makes testing or ripping out modules impossible without breaking everything.

## 5. Z-Order Sorting Vulnerability
**Problem**: `BOCompositorStack_GetSortedVisible()` uses an in-place insertion sort every frame.
**Risk**: Insertion sort is O(N^2) worst case. If the OS has 128 windows open, and they are constantly shifting, this could bottleneck the CPU.

## 6. Zero-Allocation Myth
**Problem**: The compositor boasts "Zero Allocations!", which is true because it relies on static pools.
**Risk**: Static pools mean the OS cannot scale dynamically. If 1025 windows are needed, the OS will silently fail to create the 1025th window, leading to unpredictable application behavior.
