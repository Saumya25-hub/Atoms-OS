# Dependency Cleanup

## Problem
The `BOSurface` component includes `terminal.h`, `conhost.h`, `text_viewer.h`, and `explorer.h`. This is a catastrophic architectural violation: the compositor engine is tightly coupled to the applications it is supposed to be managing generically.

## Cause
Legacy code used `BOSurface` to directly launch or handle specific app behaviors rather than using a proper decoupled event system or shell registry.

## Solution
1. Remove all high-level `#include` directives from `surface.c` and `surface.h`.
2. Move the `app_manager` out of `BOSurface` and into `kernel/shell/`.
3. Applications (Terminal, Explorer) must register themselves with the `Shell` during boot, completely bypassing the lower-level Window Manager (`wm/`).
4. Enforce strict dependency direction: `wm/` -> `ui/` -> `shell/` -> `apps`. Downward inclusion is strictly forbidden.

## Risk
The OS might fail to launch apps if the registration mechanism is not properly re-wired during the move.

## Verification
A grep scan of `kernel/wm/` should yield zero matches for `#include` strings targeting anything in `kernel/shell/` or `apps`.

## Next Step
Analyze the `#include` graph to identify all remaining circular references, as detailed in `04_include_graph.md`.
