# PATCH REPORT: ATRIX MINIMAL BROWSER & IRETQ #GP(0) FIX

## 1. Files Changed
1. rch/x86_64/interrupt/isr_stubs.asm
2. kernel/browser_engine/layout/abe_render_tree.c
3. kernel/apps/atrix/minbrow_probe.c

## 2. Functions Changed
- isr_common_stub (in rch/x86_64/interrupt/isr_stubs.asm)
  - Added strict canonical CS=0x08 and SS=0x10 descriptor loading on Ring 0 interrupt return frames.
  - Added strict arithmetic bitmask nd qword [rsp + 152], 0x00000CD5 / or qword [rsp + 152], 0x00000002 to clear illegal NT (Nested Task 0x4000) and VM (0x20000) flags from the stack before executing iretq.
- BuildRenderNodeRecursive (in kernel/browser_engine/layout/abe_render_tree.c)
  - Added filter if (dom_node->parent && dom_node->parent->type == ABE_NODE_ELEMENT && IsNonRenderableTag(dom_node->parent->tag_name)) return NULL; to prevent text children of <style>, <script>, <head>, <title>, <meta>, <link> from being inserted into the render tree.
- minbrow_paint_node_recursive (in kernel/apps/atrix/minbrow_probe.c)
  - Added semantic typography role selection (BOFONT_ROLE_TITLE, BOFONT_ROLE_UI_BOLD, BOFONT_ROLE_UI_REGULAR) based on computed style ont_size and ont_weight.

## 3. Lines Changed
- rch/x86_64/interrupt/isr_stubs.asm: 8 lines added.
- kernel/browser_engine/layout/abe_render_tree.c: 4 lines added.
- kernel/apps/atrix/minbrow_probe.c: 6 lines modified.