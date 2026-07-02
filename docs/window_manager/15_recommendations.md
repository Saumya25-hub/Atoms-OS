# Recommendations (Architectural Transition to Foundation v2)

This document purely recommends architectural fixes. **No code has been modified.**

## Priority 1: Break Circular Dependencies
- **Problem**: `BOSurface` includes `Apps/terminal.h` and `conhost.h`.
- **Root Cause**: Poor separation of concerns in the legacy codebase.
- **Impact**: You cannot compile the low-level UI without compiling the high-level apps.
- **Recommended Fix**: `BOSurface` should only know about rectangles and IDs. Applications should register themselves dynamically.
- **Difficulty**: Moderate
- **Expected Stability Gain**: High (Clean compiles, modularity).

## Priority 2: Fix the `BWE_Window` God Object
- **Problem**: `BWE_Window` struct is massive due to a mega-union containing data for every possible control.
- **Root Cause**: Trying to achieve OOP polymorphism in C without using vtables or opaque `void*` data pointers.
- **Impact**: Wastes massive amounts of static memory. Limits scalability.
- **Recommended Fix**: Change `control_data` to a `void* user_data` pointer. Let specific controls (`Button`, `ListView`) allocate their own specific structs and attach them. 
- **Difficulty**: High
- **Expected Stability Gain**: High (Memory footprint shrinks drastically).

## Priority 3: Eliminate `g_update_lock` and Unsafe Globals
- **Problem**: Inconsistent locking and heavy reliance on global states (like `bwe_is_dragging`).
- **Root Cause**: Lack of a proper thread-safe event loop.
- **Impact**: High risk of race conditions if interrupts occur during a render pass.
- **Recommended Fix**: Move dragging state into a unified struct. Enforce that all UI state mutations happen *only* during the `BOS_ProcessEvent()` drain phase of the main loop.
- **Difficulty**: Moderate
- **Expected Stability Gain**: Critical (Prevents random crashes).

## Priority 4: Standardize the UI Module Path
- **Problem**: Code is split between `kernel/BOSurface`, `kernel/bwe`, and `kernel/bocompositor`.
- **Root Cause**: Iterative, unplanned growth.
- **Impact**: Confusing for new developers.
- **Recommended Fix**: This aligns perfectly with the Foundation v2 Plan: Consolidate the useful parts into `kernel/wm/` (Window Manager / Compositor) and `kernel/ui/` (Controls and Shell).
- **Difficulty**: Low (Just moving files and updating #includes).
- **Expected Stability Gain**: Low runtime gain, but Massive maintainability gain.
