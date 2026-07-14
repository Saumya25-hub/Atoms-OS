# Root Cause Tree

## Problem 002: Invisible Cursor (Input Frozen Illusion)

```text
User perceived "Input Frozen"
│
├── Input Pipeline works flawlessly (IRQ -> InputCore -> BWE)
│   ├── Keyboard works but user receives no visual feedback without pressing arrows
│   └── Mouse works but pointer is invisible
│
├── Why is the pointer invisible?
│   ├── Hardware Cursor is unsupported (VBE Driver limitation)
│   ├── System falls back to Software Cursor Overlay
│   └── Software Cursor Overlay is never invoked
│
└── Why is Software Cursor Overlay never invoked?
    └── Phase 2 (Graphics Presentation Engine V2) decoupled cursor state.
    └── Phase 3 (V3 Architecture) expected Compositor to call `BSPE_CursorPresenter_OnCompositorRedraw`.
    └── Compositor `display_layout.c` / `bwe_core.c` missed this hook.
    └── Orphaned rendering pass causes 100% invisible cursor.
```


Invisible Cursor
|
+-- BVCursor_Draw renders to ram_fb
|
+-- BOVISUAL_Graphics_SwapFull queues ram_fb to AGDTE asynchronously
|
+-- BWE_ComposeFrame immediately runs next frame
|
+-- Desktop background drawn over cursor in ram_fb
|
+-- AGDTE copies overwritten ram_fb to VRAM
