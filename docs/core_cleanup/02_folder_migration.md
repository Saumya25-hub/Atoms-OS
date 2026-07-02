# Folder Migration

## Problem
Currently, there are over 30 folders in `kernel/`, many of which have overlapping responsibilities or confusing names (e.g., `driver` vs `drivers`, `bwe` vs `BOSurface` vs `bocompositor`).

## Cause
Lack of a strict directory standard in Phase 1 and 2.

## Solution
Consolidate the architecture into exactly these Foundation v2 pillars:
- `kernel/core/`: Boot, interrupts, memory, scheduler, lib, timer, syscall.
- `kernel/drivers/`: Hardware interfaces (display, keyboard, input abstraction).
- `kernel/wm/`: The core Window Manager (BOSurface, bocompositor, bwe_core).
- `kernel/ui/`: High-level UI framework (bwe_controls, bovisual, bofont, boasset).
- `kernel/engine/`: Horse Engine.
- `kernel/vfs/`: fs, storage, vfs.
- `kernel/shell/`: desktop_shell, taskbar, start_menu, conhost, apps.

## Risk
Moving files breaks all `#include` relative paths (`../Apps/terminal.h` will no longer exist).

## Verification
Run a mass search-and-replace for `#include` paths after moving files. Verify with `make` (or standard compiler check) that all headers can be resolved.

## Next Step
Perform the physical folder moving operations using `git mv` or standard OS file moves, then begin repairing the broken dependencies as defined in `03_dependency_cleanup.md`.
