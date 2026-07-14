# Description
A `[HEAP CORRUPTION DETECTED]` kernel panic occurs shortly after the GUI begins to load, permanently freezing the OS.

# Symptoms
- System halts (`hlt`) completely.
- Mouse and Keyboard input stops working due to `cli` (cleared interrupts) in the panic handler.
- Serial console displays an Out of Memory / Corrupted Canary message.

# Evidence
- **Corrupted Offset**: `+3686400 (Rear Canary Byte 0)`
- **Allocation Size**: `3,686,400` bytes (exactly 1280x720 32-bit ARGB pixels)
- **Allocation Caller RIP**: `0x12D79E` (Resolves incorrectly to `popq %rbp` inside `rook_page_login_get`, implying tracking metadata is also smashed)
- `desktop_shell` is rendering a `1280x720` canvas while VBE physical hardware remains at `1920x1080`.

# Files
- `kernel/core/memory/heap/src/heap.c`
- `kernel/shell/desktop_shell/desktop_shell.c`
- `kernel/wm/bwe/src/bwe_window.c`

# Functions
- `krealloc_tracked` / `kfree`
- `BOS_SurfacePresent`
- `validate_block_or_panic`

# Root Cause
NOT YET PROVEN. 
Hypothesis: Buffer overflow by exactly 1 byte or SSE block overwrite smashing heap headers of a `1280x720` buffer.

# Confidence

**Evidence**
✔ Allocation size matches exactly 1280x720x4 (3686400)
✔ Crash happens immediately after Desktop / Welcome Screen allocates the surface
✔ Rear Canary is the exact corrupted element
✖ Exact function or `dst[i]` loop performing the out-of-bounds write is not yet found

**Evidence Score**: 4/5 (Very likely)

# Status
ACTIVE / BLOCKING

# Possible Fixes
- Identify the exact rendering loop writing out-of-bounds.
- Align `g_kernel_screen_width` perfectly with the actual physical `VBE` mode instead of a virtual soft-resolution.

# Dependencies
- Phase 02 (Display Pipeline)
- Phase 05 (Desktop Shell)
