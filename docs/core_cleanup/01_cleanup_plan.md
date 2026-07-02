# Core Cleanup Plan

## Problem
The ATOMS OS legacy architecture is monolithic, heavily entangled, and relies on circular dependencies where low-level systems (BOSurface) include high-level applications (Terminal). The folder structure is erratic, spreading Window Manager logic across `bwe`, `BOSurface`, and `bocompositor`.

## Cause
Organic, unplanned growth. Features were added by modifying the nearest available structure (`BWE_Window`), leading to God Objects and spaghetti include chains.

## Solution
Phase 3 is an Architecture Cleanup Mission. We will strictly enforce the Foundation v2 folder layout: `core/`, `drivers/`, `wm/`, `ui/`, `engine/`, `vfs/`, `shell/`. We will break circular dependencies by moving high-level applications (`Terminal`, `Explorer`) completely out of the low-level rendering stack. We will consolidate global variables into a `WMContext` structure to eliminate unsafe global states.

## Risk
- **Build Breakage**: Moving files and breaking includes will temporarily destroy the build.
- **Behavior Change**: Over-refactoring might introduce bugs. The strict rule is: *No behavioral changes, only structural ones.*

## Verification
The kernel must compile successfully after every major folder move. `#include` checks will be run to ensure no low-level module includes a high-level application.

## Next Step
Execute `02_folder_migration.md` to establish the new directory hierarchy and move the files into their correct module domains.
