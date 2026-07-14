# Purpose
To investigate the Heap Manager (v1), specifically focusing on the mechanisms tracking allocations, metadata integrity, and the root cause behind the `[HEAP CORRUPTION DETECTED]` kernel panic observed during the desktop GUI initialization.

# Current State
The kernel uses a custom Heap implementation (`heap.c`) that supports canary-based overflow detection, allocation tracking (recording the caller RIP), and block validation.

# Architecture
- Every block is allocated with metadata containing the size, caller RIP, and magic numbers.
- A "Rear Canary" (a byte sequence) is placed immediately after the allocated payload.
- Upon block operations (like `kfree` or `krealloc_tracked` or periodic heap audits), the heap manager validates the metadata and the canaries.
- If a canary mismatch or metadata corruption is detected, `heap.c` panics via `validate_block_or_panic`, logs diagnostics to the serial port, calls `cli` (disabling interrupts), and halts the CPU.

# Pipeline
`kmalloc` (Frontend) -> `allocate_block` (Backend) -> `set_canary` (Metadata) -> `track_caller` (Diagnostics) -> `validate_block_or_panic` (Auditor) -> `kfree` / `krealloc` (Reclaimer)

# Runtime Evidence
The OS experiences a fatal lockup (mouse/keyboard freeze) because the Heap Manager throws a panic and halts the CPU:
- **Panic Message**: `[HEAP CORRUPTION DETECTED]` (Triggers panic handler)
- **Allocation Address**: `0x811E2D38`
- **Allocation Size**: `3686400` bytes (exactly 1280x720x4).
- **Allocation Caller**: `0x12D79E`
- **Corrupted Offset**: `+3686400 (Rear Canary Byte 0)`
- **Symbol Resolution for 0x12D79E**: Maps exactly to `rook_page_login_get() + 0xE` (which corresponds to `popq %rbp`), an instruction that does NOT call `kmalloc`.

# Strengths
- **Canary System is Working**: The Heap successfully detects out-of-bounds writes using rear canaries.
- **Diagnostics**: The heap panic provides rich information (Allocation Address, Size, Caller RIP).
- **Safety Mechanism**: By panicking and halting on corruption, it prevents cascading undefined behavior (even though it manifests as a "freeze" to the user).

# Weaknesses
- **Unreliable Caller RIP**: The `Allocation Caller` recorded by `__builtin_return_address(0)` points to `0x12D79E`, which resolves to the end of `rook_page_login_get`. This indicates that either the metadata itself was overwritten with garbage, or the RIP tracking is fundamentally flawed due to inline functions or linking issues.
- **Rigid Panic Strategy**: The immediate `hlt` and `cli` completely lock up the user input (IRQ 1 and IRQ 12 are masked).

# Practical Problems
- The system permanently freezes when the GUI starts due to this panic.
- Tracking down the actual culprit is hindered by the unreliable `Allocation Caller` metadata.

# Root Cause
ROOT CAUSE NOT YET PROVEN.

While runtime evidence proves that a 3.6MB buffer (intended for a 1280x720 surface) was corrupted exactly at the rear canary byte, an exhaustive static analysis of rendering loops (`BOS_SurfacePresent`, `desktop_shell`, `page_login.c`) revealed that all known copy loops respect their bounds (`i < w * h`). The fact that the caller RIP metadata is also anomalous strongly suggests that the overflow is not a simple `[+1]` array write, but a deeper memory management alignment bug, an SSE/SIMD bulk copy overwriting the header, or a stray pointer.

# Root Cause Confidence

**Evidence**
✔ Allocation size matches exactly 1280x720x4 (3686400)
✔ Crash happens immediately after Desktop / Welcome Screen allocates the surface
✔ Rear Canary is the exact corrupted element
✖ Exact function or `dst[i]` loop performing the out-of-bounds write is not yet found

**Evidence Score**: 4/5 (Very likely)

# Files Investigated
- `kernel/core/memory/heap/src/heap.c`
- `kernel/core/memory/heap/include/heap.h`
- `kernel/wm/bwe/src/bwe_window.c`
- `kernel/shell/desktop_shell/desktop_shell.c`
- `kernel/shell/rook/pages/page_login.c`
- `build/kernel.map`

# Functions Investigated
- `krealloc_tracked`, `validate_block_or_panic`, `kmalloc`
- `BOS_SurfacePresent`
- `Shell_DrawWallpaper`
- `desktop_shell_render`
- `login_on_enter`, `draw_visible_spiral_galaxy`, `login_putpixel`
- `rook_page_login_get`

# Experiments Performed
1. Cross-referenced `Allocation Caller: 0x12D79E` with `kernel.map`. Discovered it points to an impossible caller (`popq %rbp` in `rook_page_login_get`), indicating metadata corruption or an unwinding bug.
2. Verified all loops iterating over the 3,686,400 byte buffer in `bwe_window.c` and `page_login.c`. Verified bounds mathematically; they correctly stop at `921599` (byte offset 3686396). No simple OOB write was found.

# Results
FAIL

# Next Investigation
Phase 2: Display Pipeline

# Still Unknown
- Exact instruction triggering the OOB write
- Exact CPU running the offending code (BSP vs AP)
- Exact frame number when the corruption happens
- Exact subsystem (Desktop vs Welcome vs Compositor vs Memory SSE copy)
