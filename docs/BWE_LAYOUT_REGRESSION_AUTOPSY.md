# BWE Layout Regression Autopsy

## Root Cause Analysis

I have investigated the layout regression using the runtime evidence and source code diffs. I found several root causes for the layout failure.

### ROOT CAUSE #1: Double Application of Titlebar Offsets
Before my changes, child coordinates were calculated relative to the parent window's **outer frame**. Applications like Explorer and Calculator manually added titlebar offsets (e.g. `y=40`) to avoid overlapping the titlebar. 
When I introduced `BWE_Geometry_CalculateClientBounds` and modified `BOS_SetBounds` / `BOS_CreateSurface` to use the true client origin (offsetting by `+35` for the titlebar), the existing applications were still applying their manual offsets. This caused double-offsets (e.g. `35 + 40 = 75`), pushing content down and clipping it.

### ROOT CAUSE #2: Docking vs Anchor Misunderstanding
I implemented a `BWE_DOCK_MODE` layout engine (similar to WinForms `Dock` property: Top, Bottom, Left, Right, Fill), which blindly stacks controls based on sibling order. 
However, the design goal was an **Anchor-based** layout engine (preserving margins: Top, Bottom, Left, Right). Because of this, applications that used custom grids (like Calculator and Music Player) were either not migrated or forcefully stretched, breaking their intentional structure.

### ROOT CAUSE #3: Missing Layout Trigger on Maximize
When `BWE_WindowMaximize` is called, it correctly updates the top-level window's screen bounds. However, for applications that were not explicitly migrated to use the (incorrect) Docking engine, their children remained at their original absolute bounds. There was no Anchor math to preserve margins and resize them correctly alongside the parent's client area.

## Planned Fixes (Shared Layout Pipeline)

1. **Remove Docking, Implement Anchors**: Rip out `bwe_layout.c`'s `BWE_DOCK_*` math. Introduce `BWE_ANCHOR_LEFT`, `BWE_ANCHOR_RIGHT`, `BWE_ANCHOR_TOP`, `BWE_ANCHOR_BOTTOM`.
2. **Anchor Baseline**: When a control is created, capture its baseline margins relative to the parent's *client size*.
3. **Layout Pass**: In `BWE_UpdateLayout`, only adjust children based on their Anchor Flags and their captured baseline margins.
4. **Fix Double Offsets**: Go through all applications (`apps.c`, `explorer_ui.c`) and remove the legacy manual titlebar/border offsets since the Window Engine now handles client origin correctly.

## Explorer Test Matrix Baseline (Current State)
- DEFAULT: FAIL (Double offsets cause clipping)
- MAXIMIZE: FAIL (Children do not resize)
- RESTORE: FAIL (Layout is broken)
