# PERFORMANCE PATCH REPORT — 4/5 APP MULTI-WINDOW STABILITY & VCPU PROTECTION
**Author:** ATOMS OS Patch Execution Team  
**Input:** `PERFORMANCE_FORENSIC_REPORT.md`, `PERFORMANCE_ARCHITECTURE_PLAN.md`  
**Target:** 4/5 Application Scaling, BSPE Partial VRAM Presentation, Mouse Coalescing, Profiler Synchronization  
**Status:** TASK 3 COMPLETE — Surgical Patch Implemented & Clean Build Succeeded

---

## 1. Summary of Modified Files

| File Path | Function(s) Changed | Lines | Nature of Modification |
| :--- | :--- | :--- | :--- |
| `bovisual/Graphics/graphics.c` | `BOVISUAL_Graphics_SwapFull()` | ~6 lines | Enabled `bspe_use_partial_present = true;` and hooked `bos_profiler_record_mem_copy()`. |
| `kernel/wm/bwe/renderer/bwe_compositor.c` | `BWE_ComposeFrame()` | +10 lines | Added `s_is_composing` re-entrancy protection guard and hooked `bos_profiler_record_dirty_rect()`. |
| `kernel/wm/bwe/src/bwe_core.c` | `BWE_PumpEvents()` | +8 lines | Added mouse move peek-coalescing loop to prevent multi-drag event backlog. |
| `kernel/performance/statistics/stats_profiler.c` | `bos_prof_stats_update()` | +14 lines | Added rolling window arrays (`s_rolling_dirty`, `s_rolling_vram`) for true rolling average dirty area and bandwidth metrics. |

---

## 2. Surgical Source Code Diffs

### 1. `bovisual/Graphics/graphics.c`
```diff
 void BOVISUAL_Graphics_SwapFull(const BVFramebuffer* hw_fb) {
     ...
     /* Phase 2: Enable BSPE Partial VRAM Copying by forwarding compositor damage */
     extern bool bspe_use_partial_present;
-    bspe_use_partial_present = false;
+    bspe_use_partial_present = true;
     ...
+    extern void bos_profiler_record_mem_copy(uint64_t bytes, bool is_vram);
+    bos_profiler_record_mem_copy((uint64_t)bytes, true);
 }
```

### 2. `kernel/wm/bwe/renderer/bwe_compositor.c`
```diff
 void BWE_ComposeFrame(void) {
+    static bool s_is_composing = false;
+    if (s_is_composing) return;
+    s_is_composing = true;
     ...
     if (g_dirty_rect_count == 0) {
+        s_is_composing = false;
         return;
     }
     ...
     for (uint32_t d = 0; d < g_dirty_rect_count; d++) {
         BWE_Rect current_dirty = g_dirty_rects[d];
+        extern void bos_profiler_record_dirty_rect(int32_t w, int32_t h);
+        bos_profiler_record_dirty_rect(current_dirty.width, current_dirty.height);
     ...
     g_dirty_rect_count = 0;
     bos_profiler_frame_end();
+    s_is_composing = false;
 }
```

### 3. `kernel/wm/bwe/src/bwe_core.c`
```diff
 void BWE_PumpEvents(void) {
     ...
     while (processed < budget && BWE_EventQueue_Pop(&bwe_ev) == BWE_SUCCESS) {
         processed++;
         if (bwe_ev.type == BWE_EVENT_MOUSE_MOVE || bwe_ev.type == BWE_EVENT_MOUSE_DOWN || bwe_ev.type == BWE_EVENT_MOUSE_UP) {
+            // Coalesce consecutive mouse move events so we only compute layout/drag for the newest position
+            if (bwe_ev.type == BWE_EVENT_MOUSE_MOVE) {
+                BWE_Event next_ev;
+                while (BWE_EventQueue_Peek(&next_ev) == BWE_SUCCESS && next_ev.type == BWE_EVENT_MOUSE_MOVE) {
+                    BWE_EventQueue_Pop(&bwe_ev);
+                    processed++;
+                }
+            }
             g_bwe_update_calls_count++;
```

### 4. `kernel/performance/statistics/stats_profiler.c`
```diff
+static uint32_t     s_rolling_dirty[BOS_PROFILER_ROLLING_WINDOW];
+static uint32_t     s_rolling_vram[BOS_PROFILER_ROLLING_WINDOW];
 ...
 void bos_prof_stats_update(const BOS_FrameMetrics* frame) {
     ...
     s_rolling_times[s_rolling_head] = frame_time_us;
+    s_rolling_dirty[s_rolling_head] = frame->dirty_rect_area;
+    s_rolling_vram[s_rolling_head]  = (uint32_t)frame->vram_bytes_copied;
     ...
+    uint64_t dirty_sum = 0;
+    uint64_t vram_sum = 0;
     for (uint32_t r = 0; r < s_rolling_count; r++) {
         rolling_sum += s_rolling_times[r];
+        dirty_sum   += s_rolling_dirty[r];
+        vram_sum    += s_rolling_vram[r];
     }
     s_global_stats.avg_frame_time_us = (uint32_t)(rolling_sum / s_rolling_count);
+    s_global_stats.avg_dirty_area = (uint32_t)(dirty_sum / s_rolling_count);
+    s_global_stats.avg_vram_copy_bytes = (uint32_t)(vram_sum / s_rolling_count);
     s_global_stats.current_fps = (s_global_stats.avg_frame_time_us > 0) ? (1000000U / s_global_stats.avg_frame_time_us) : 0;
+    s_global_stats.total_memory_bandwidth_bytes_per_sec = (uint64_t)s_global_stats.avg_vram_copy_bytes * s_global_stats.current_fps;
```

---

## 3. Build & Compilation Verification
- **Command:** `powershell.exe -ExecutionPolicy Bypass -File .\build.ps1`
- **Result:** Exit Code `0` (Clean Compilation, Zero Warnings/Errors)
- **Artifacts:** `build/OS.img`, `build/SignaturesOS.vdi`, `build/SignaturesOS.vmdk`, `build/BOOTX64.EFI`

---
*End of PERFORMANCE_PATCH_REPORT.md — Ready for Task 4: Certification Phase (`PERFORMANCE_CERTIFICATION_REPORT.md`)*
