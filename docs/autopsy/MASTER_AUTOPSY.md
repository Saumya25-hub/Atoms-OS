# Master Investigation Status
Current Phase: Phase 03 (Input Pipeline) - COMPLETED
Finished Phases: Phase 01 (Heap Audit), Phase 03 (Input Pipeline)

# Current Root Causes
- None Proven.

# Proven Facts
- Desktop Shell hardcodes `1280x720` resolution based on a Display Intelligence Engine (DIE) evaluation that favors software-copy bandwidth.
- VBE Graphics Driver fails to change physical mode from `1920x1080` to `1280x720` due to a stubbed `SetMode`.
- `kmalloc` panics due to rear canary corruption on a `3,686,400` byte allocation.
- `Allocation Caller` records an impossible RIP (`0x12D79E` which is `popq %rbp`), implying metadata corruption.

# Hypotheses

**Hypothesis 1: Renderer OOB Write**
A rendering pipeline or buffer copy writes past the `3686399` byte boundary of the 3.6MB desktop surface buffer, smashing the heap metadata and canary.

**Evidence**
✔ Allocation size matches (3686400)
✔ GUI starts exactly when crash happens
✔ Canary overwritten exactly after allocation bounds
✖ Exact instruction/loop not found

**Evidence Score**: 4/5 (Very likely)

**Evidence Score**: 3/5 (Possible)

**Hypothesis 3: Missing Visual Cursor (The "Input Frozen" Illusion)**
The mouse and keyboard are fundamentally working and processing events flawlessly. However, the Hardware Cursor initialization fails (since VBE does not support it). The system falls back to a software cursor (`cursor_renderer_draw_software` / `BSPE_CursorPresenter_OnCompositorRedraw`), which was completely orphaned during the V3 Architecture transition and is never invoked by the compositor. Because the cursor is completely invisible and keyboard highlights are not triggered until arrow keys are pressed, the user perceives the system as completely frozen.

**Evidence**
✔ The entire event pipeline (IRQ -> InputCore -> BWE_PumpEvents -> HitTest -> DesktopShell) was traced and proven to be processing events.
✔ `BWE_PumpEvents` successfully registers hits on `BWE_DESKTOP_ID` because the pointer initializes at `(640, 360)`.
✔ The software cursor rendering function `BSPE_CursorPresenter_OnCompositorRedraw` is never called anywhere in the codebase.
✔ Keyboard events trigger `BWE_InvalidateWindow`, confirming functionality despite the visual lack of response.

**Evidence Score**: 5/5 (Proven)

# Fixed Problems
- None.

# Remaining Problems
- [Problem 001: Heap Corruption](file:///d:/Signatures_OS/docs/autopsy/problems/problem_001_heap_corruption.md)
- [Problem 002: Invisible Cursor](file:///d:/Signatures_OS/docs/autopsy/problems/problem_002_invisible_cursor.md)

# Current Blocking Issue
- We need to prove the exact line of code that writes the corrupted byte to promote Hypothesis 1 to CONFIRMED.
- We need to fix the orphaned software cursor rendering pass so the user can interact visually.

# Phases
- [Phase 01: Heap Audit](file:///d:/Signatures_OS/docs/autopsy/phase_01_heap_audit.md)
- [Phase 03: Input Pipeline](file:///d:/Signatures_OS/docs/autopsy/phase_03_input_pipeline.md)
- [Root Cause Tree](file:///d:/Signatures_OS/docs/autopsy/root_cause_tree.md)

# Main Workflow Dependency Graph

Bootloader
↓ [PASS]
Framebuffer
↓ [PASS]
VBE
↓ [UNKNOWN] (SetMode fails, physical stuck at 1920x1080)
Display Driver
↓ [FAIL]
DIE
↓ [PASS] (Correctly limits bandwidth)
AGDAE
↓ [NOT TESTED]
Desktop
↓ [UNKNOWN] (Likely causes OOB write)
BWE
↓ [UNKNOWN]
Compositor
↓ [PASS]
Present
↓ [UNKNOWN]
Mouse
↓ [PASS] (InputCore and Pipeline process flawlessly)
Cursor Rendering
↓ [FAIL] (Software fallback is orphaned, cursor invisible)
DOOM Window
↓ [UNKNOWN]
DOOM Rendering
↓ [FAIL]

# Investigation Timeline

2026-07-11
- Heap corruption discovered
- DOOM SMP race condition hypothesis formed

2026-07-12
- Allocation size (3686400 bytes) identified for Heap Panic
- DIE capability score limits resolution to 1280x720

2026-07-13
- VBE `SetMode` stub discovered in AGDPE Driver
- `Allocation Caller` found pointing to `popq %rbp` in `rook_page_login_get`, proving metadata is corrupted and not just the payload
- Documented Phase 01: Heap Audit
- Investigated Input Pipeline (Phase 03).
- Confirmed input stack is perfectly functional (IRQ -> DesktopShell).
- Discovered "frozen" input illusion is actually a visual bug caused by an orphaned Software Cursor Rendering pass in `CursorEngine`.

2026-07-13
- Phase 04 Cursor Presentation Autopsy completed
- Discovered async race condition between BWE Compositor and AGDTE Presenter
- Discovered 0-damage culling bug dropping stationary cursor frames
