# Removed Legacy

## Problem
Old folders and files litter the kernel directory, causing confusion. The division between `bwe` and `BOSurface` is entirely artificial and historically driven.

## Cause
Code was added iteratively without a master architectural plan.

## Solution
The following conceptual and physical structures will be removed or merged:
- `kernel/BOSurface/` -> Merged entirely into `kernel/wm/` and `kernel/ui/`.
- `kernel/bwe/` -> Merged entirely into `kernel/wm/` and `kernel/ui/`.
- `kernel/boasset/`, `kernel/bofont/`, `kernel/boimage/`, `kernel/bovisual/` -> Moved into `kernel/ui/`.
- `kernel/driver/` and `kernel/drivers/` -> Consolidated into a single `kernel/drivers/` folder.

We will NOT rewrite the logic inside these files. We are only changing their physical location and repairing their includes.

## Risk
Git history might become confusing if files are moved and modified heavily in the same commit. We will use `git mv` where applicable and keep refactoring atomic.

## Verification
A physical inspection of the `kernel/` root directory should reveal exactly and only the target folders (core, drivers, wm, ui, engine, vfs, shell).

## Next Step
Outline the build recovery strategy in `08_build_recovery.md`.
