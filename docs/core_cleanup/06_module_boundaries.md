# Module Boundaries

## Problem
Modules in ATOMS OS often handle responsibilities outside their domain. For instance, `BOSurface` manages both low-level bounding boxes and high-level drag-and-drop state. `Desktop Shell` manages the wallpaper *and* application execution paths.

## Cause
Lack of strict architectural guidelines during early development. The line between Window Manager and User Shell was never drawn.

## Solution
Enforce strict rules for each module:

- **`kernel/core/`**: Memory allocation, tasks, basic libs. Knows absolutely nothing about UI.
- **`kernel/drivers/`**: Translates hardware signals into pure data (e.g. `BVEvent`). Knows nothing about Windows or Surfaces.
- **`kernel/wm/`**: Mathematical composition (`bocompositor`), bounds tracking, Z-order, and input hit-testing. Knows nothing about applications or visual styles (colors, fonts).
- **`kernel/ui/`**: Controls (Buttons, Textboxes) and drawing primitives (Paint, Theme). Understands visual styling but does not handle occlusion math.
- **`kernel/shell/`**: High-level OS behaviors. The Taskbar, Desktop Wallpaper, App Registry, and launching mechanisms.

Every module will receive a standard `README.md` containing its Purpose, Responsibilities, Public API, and Dependencies to ensure future developers do not violate these boundaries.

## Risk
Refactoring logic out of `BOSurface` and into `UI` or `Shell` requires surgical code extraction, which could introduce subtle bugs if not done carefully.

## Verification
Code review of each module's `module.c` and `module.h`. A module should only expose functions relevant to its defined single responsibility.

## Next Step
Document the legacy systems and folders that will be entirely removed or deprecated during this transition (`07_removed_legacy.md`).
