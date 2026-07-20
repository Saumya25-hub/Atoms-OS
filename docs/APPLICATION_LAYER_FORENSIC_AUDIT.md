# APPLICATION LAYER FORENSIC AUDIT

## 1. EXECUTIVE SUMMARY
A comprehensive forensic investigation of the ATOMS OS application layer has been completed. The visual inconsistencies, clipping failures, and functional breakdowns observed are caused by a mixture of **shared architectural failures** and **legacy application API usage**. Specifically, the Window Engine (BWE) and the new graphics rendering pipelines (BOImage v2.5 / BOFont v2) have become completely decoupled with regards to clipping boundaries. Furthermore, window coordinate calculations improperly include window decorators (titlebar/borders) in the client layout, misplacing all application controls. Finally, the Terminal application is entirely fake, consisting of hardcoded `strcmp` checks instead of a shell backend.

## 2. APPLICATION ARCHITECTURE MAP
The general path for application execution and window creation:
1. **Entry Point:** App launched via shell (`apps.c`).
2. **Window Creation:** Apps call `BOS_CreateWindow()` (or `BOS_CreateSurface()`).
3. **Window Registration:** `BOS_CreateSurface()` allocates a slot in `g_windows` and pushes the window ID onto the Z-order stack (`g_z_order_stack`).
4. **Rendering Callbacks:** Windows register an `on_render` callback (e.g., `bwe_panel_render`).
5. **Compositor Pipeline:** `bwe_compositor.c:BWE_ComposeFrame()` iterates over `g_z_order_stack`, pushes a clipping rectangle (`BWE_ClipPush()`), and calls `compose_window_recursive()`.
6. **Display Pipeline:** The backbuffer RAM framebuffer is transferred to the physical VBE back page via `BOVISUAL_Graphics_SwapFull()`.

## 3. CURRENT WINDOW CONTRACT
Applications are expected to use `BOS_CreateWindow` for top-level windows and `BOS_CreatePanel` / `BOS_CreateLabel` for child controls. The coordinate system implies that child controls are positioned relative to the parent window. However, the system currently fails to delineate between the **absolute window frame** (which includes borders and title bars) and the **client area**.

## 4. EXPECTED VS ACTUAL COORDINATE SYSTEM
* **Expected:** `child->screen_bounds.x = parent->screen_bounds.x + THEME_BORDER_WIDTH + child->local_bounds.x;` (and similarly for Y, accounting for the title bar).
* **Actual (`bwe_window.c:164`):** `win->screen_bounds.x = parent->screen_bounds.x + win->local_bounds.x;`
Because `parent->screen_bounds.y` is the absolute top of the window, a child placed at `y = 15` will draw **over the titlebar** (which occupies the first 30 pixels). 

## 5. SHARED ROOT CAUSES
1. **Coordinate Misalignment:** `BOS_CreateSurface` places all child controls relative to the parent's outer bounds rather than its client bounds.
2. **Clipping Engine Bypass:** The most critical rendering engines (`BOImage` and `BOFont`) write directly to the screen via `BOVISUAL_Graphics_PutPixel` and completely ignore the BWE clipping stack.

## 6. SETTINGS OVERFLOW ROOT CAUSE
The Settings application relies heavily on `BOS_CreateLabel` and `BOImage_BatchDrawSpriteTinted` for text and cursor sprites. 
While solid fills (via `BWE_FillRect`) respect the BWE clipping stack, `BOFont_DrawText` and `BOImage_AtlasDrawEx` explicitly do not call `BWE_GetClip()`. This allows all text and icon sprites to render completely outside the physical bounds of the Settings window, explaining why scroll content and overflowing lists are visible across the desktop.

## 7. TERMINAL COMMAND FAILURE ROOT CAUSE
The Terminal command interaction is completely non-functional because it is mocked.
In `apps.c`, `terminal_textbox_event_callback()` intercepts the Enter key and executes hardcoded string comparisons (`strcmp(cmd, "help")`, `"ls"`, `"neofetch"`, `"clear"`, `"exit"`). It never passes the buffer to `shell_execute()` or standard OS pipes.

## 8. PER-APPLICATION DAMAGE MATRIX
* **Terminal:** Complete backend failure (mocked UI).
* **Settings:** Heavy visual overflow; controls placed under the titlebar due to coordinate layout bug.
* **Calculator:** Grid overlaps with window decorations due to `BOS_CreateSurface` coordinate misalignment.
* **Music Player:** Layout shifted upward into title bar area.

## 9. INPUT / FOCUS / EVENT ROUTING FINDINGS
Input hit-testing relies on absolute screen bounds (`bwe_window.c:BWE_HitTest`). Since controls draw outside their client areas but the hit-testing logic expects them to be strictly bounded, mouse clicks on overflowing elements (like Settings sprites) will fail to register or will erroneously hit the desktop.

## 10. CLIPPING FINDINGS
* `BWE_Paint.c` primitives (`BWE_FillRect`, `BWE_DrawLine`) **correctly** invoke `BWE_GetClip()` and clip rendering.
* `BOFont_DrawText` (`bofont.c`) and `BOImage_BatchDrawSpriteTinted` (`boimage.c`) **ignore** `BWE_GetClip()` and use unconditional `BOVISUAL_Graphics_PutPixel()` loops, breaking encapsulation.

## 11. DISPLAY MIGRATION FINDINGS
Legacy coordinate assumptions remain hardcoded across the GUI layer:
* `apps.c` explicitly prints `"Display: 1280x720 VBE v3.0"` in neofetch.
* `bwe_compositor.c` continues to rely on `g_kernel_screen_width` and `g_kernel_screen_height` globals rather than requesting metrics from the new AGDAE capability-based policy structure.

## 12. MEMORY / ABI SAFETY FINDINGS
In `bwe_window.c`, `BOS_DestroySurface` manually collapses child arrays. Deeply nested UI trees being rapidly created/destroyed during the Stress Test run the risk of iterator invalidation, leading to orphan surface leaks in `g_windows` and memory leaks of `user_data` buffers.

## 13. GIT REGRESSION TIMELINE
The UI breakage clearly correlates with the introduction of the **BOIMAGE v2.5 Quality Engine** and **BOFONT v2**. These modules were heavily optimized for scaling/filtering but lost the compositor clipping integration that the earlier generic `BWE_Paint` paths possessed. 

## 14. ROOT CAUSE TREE
```
APPLICATION LAYER FAILURE
├── Shared Window Contract
│   └── BOS_CreateSurface applies local bounds to absolute outer window bounds (overlaps titlebar).
├── Rendering / Clipping
│   ├── BWE Compositor clips client area accurately.
│   └── BOImage & BOFont completely ignore BWE_GetClip() and write directly to BOVISUAL.
├── Input / Focus
│   └── Controls overflowing bounds are unclickable because BWE_HitTest expects them inside.
└── Legacy Application APIs
    └── Terminal shell is a hardcoded mock UI inside apps.c with no backend.
```

## 15. SEVERITY TABLE
* **P0 (Architecture-breaking):** BOImage / BOFont clipping bypass (`bofont.c` / `boimage.c`).
* **P0 (Architecture-breaking):** Window client layout origin mismatch (`bwe_window.c:164`).
* **P1 (Major Bug):** Fake Terminal backend logic (`apps.c:124`).
* **P2 (App-specific):** Stale 1280x720 assumptions (`apps.c`).

## 16. RECOMMENDED FIX ORDER
1. **P0 FIX:** Update `BOImage_SamplePixel` / `BOImage_AtlasDrawEx` and `bofont_layout_callback` (or the underlying BOVISUAL pixel plotting) to fetch and respect `BWE_GetClip()`.
2. **P0 FIX:** Update `bwe_window.c:BOS_CreateSurface()` to offset `screen_bounds.x` and `screen_bounds.y` by the parent's border width and titlebar height if the parent has decorations.
3. **P1 FIX:** Connect Terminal frontend in `apps.c` to actual `shell_execute()` backend.

## 17. EXACT FILES AND FUNCTIONS RESPONSIBLE

**BUG 1: Clipping Bypass**
* **FILE:** `kernel/ui/boimage/boimage.c`
* **FUNCTION:** `BOImage_FlushBatch` & `BOImage_AtlasDrawEx`
* **LINE / REGION:** `BOVISUAL_Graphics_PutPixel(s->x + dx, s->y + dy, color);`
* **CURRENT BEHAVIOR:** Writes directly to framebuffer regardless of current compositing clip stack.
* **EXPECTED BEHAVIOR:** Should fetch `BWE_GetClip()` and intersect bounds before modifying pixels.
* **AFFECTED APPS:** Settings, Explorer, File Manager (any app using text/sprites).
* **SEVERITY:** P0

**BUG 2: Client Origin Misalignment**
* **FILE:** `kernel/wm/bwe/src/bwe_window.c`
* **FUNCTION:** `BOS_CreateSurface`
* **LINE / REGION:** `win->screen_bounds.y = parent->screen_bounds.y + win->local_bounds.y;`
* **CURRENT BEHAVIOR:** Fails to account for parent window decorators (30px titlebar).
* **EXPECTED BEHAVIOR:** Should add titlebar offset if parent has decorations.
* **AFFECTED APPS:** Calculator, Settings, Music Player (all apps rendering inside a framed window).
* **SEVERITY:** P0

**BUG 3: Fake Terminal Shell**
* **FILE:** `kernel/shell/desktop_shell/apps.c`
* **FUNCTION:** `terminal_textbox_event_callback`
* **LINE / REGION:** `if (strcmp(cmd, "help") == 0) ...`
* **CURRENT BEHAVIOR:** Intercepts input strings and returns hardcoded fake responses.
* **EXPECTED BEHAVIOR:** Should route `cmd` buffer to standard shell processor and capture stdout.
* **AFFECTED APPS:** Terminal.
* **SEVERITY:** P1
