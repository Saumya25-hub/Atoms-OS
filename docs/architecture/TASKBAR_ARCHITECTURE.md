# ATOMS OS — Taskbar & Start Desktop Integration
## Architecture Specification Document (TASKBAR_ARCHITECTURE.md)

---

### 1. Architectural Philosophy & Objectives

The **ATOMS Taskbar Engine** is designed as an integral Desktop UI component that bridges the user experience with the underlying **BWE (BOSurface Window Engine)**, **Horse Engine (App Registry & Launcher)**, and **Process Manager**.

#### Core Architectural Guarantees:
- **No Kernel Redesign**: Leverages the existing BWE window hierarchy and event loop.
- **No Second Window Manager**: Uses authoritative BWE window IDs, Z-order stack, and focus state.
- **No Duplicate Process Tracker**: Queries authoritative BWE and Process Manager state.
- **No Fake Application State**: State reflects real runtime window and process lifecycle.
- **Zero Heap Allocations Per Frame**: All geometry calculations, item arrays, and strings reside in statically allocated memory.
- **Dirty-Region Optimization**: Invalidation is strictly confined to the taskbar and start panel bounding boxes.

---

### 2. Taskbar Visual Design & Geometry System

```
  ┌─────────────────────────────────────────────────────────────────────────────────────────────┐
  │                                      DESKTOP WORKSPACE                                      │
  │                                                                                             │
  │                                                                                             │
  │    ┌───────────────────────────────────────────────────────────────────────────────────┐    │
  │    │ [START] │ [Exp] [Notes] [Calc] [Term] [Set] [Media] [Music] [ATRIX] [TM] [CP] [DOOM] │ 12:45 PM │ 22 Aug │    │
  │    │  (Logo) │  -•-   ---     -•-    ---    ---    ---     ---     ---    ---  ---  ---  │          │        │    │
  │    └───────────────────────────────────────────────────────────────────────────────────┘    │
  └─────────────────────────────────────────────────────────────────────────────────────────────┘
```

#### Geometry Specifications:
- **Position**: Bottom-centered, floating above screen bottom.
- **Height**: 54 px (comfortable touch and click height).
- **Y-Offset**: `screen_height - taskbar_height - 12 px`.
- **Width**: Responsive to content:
  `total_width = pad_left (16) + start_width (40) + sep (12) + (11 * (cell_w 40 + spacing 8)) + sep (12) + clock_width (110) + pad_right (16)`.
- **Corner Radius**: 18 px (rounded pill/capsule shape).
- **Glassmorphism Styling**:
  - Background Tint: `0xE60F172A` (Deep Slate Translucent).
  - Border Highlight: `1px 0x33FFFFFF` (Subtle crystal/glass rim).
  - Inner Glow: Procedural soft edge blending.
  - Drop Shadow: Procedural dark outer halo.

---

### 3. Application Registry & 11 Core ATOMS Applications

The Taskbar integrates directly with the **Horse Engine** registry (`kernel/engine/horse_engine.h`).

| # | Application | App ID | Constant | Icon Asset ID | Display Name | Launch Target |
|---|---|---|---|---|---|---|
| 1 | **Explorer** | 2 | `APP_ID_EXPLORER` | `ICON_EXPLORER` (104) | "File Explorer" | `explorer_init` |
| 2 | **Notes** | 17 | `APP_ID_NOTES` | `ICON_FILE` (102) | "Notes" | `notes_app_launch` |
| 3 | **Calculator** | 4 | `APP_ID_CALCULATOR` | `ICON_CALCULATOR` (109) | "Calculator" | `calculator_init_v2` |
| 4 | **Terminal** | 3 | `APP_ID_TERMINAL` | `ICON_TERMINAL` (103) | "Terminal" | `terminal_init_v2` |
| 5 | **Settings** | 5 | `APP_ID_SETTINGS` | `ICON_SETTINGS` (105) | "Settings" | `settings_init_v2` |
| 6 | **Media Player** | 8 | `APP_ID_MUSIC` | `ICON_MUSIC` (111) | "Media Player" | `bos_media_player_launch` |
| 7 | **Music** | 8 | `APP_ID_MUSIC` | `ICON_MUSIC` (111) | "Music" | `music_init_v2` |
| 8 | **ATRIX** | 12 | `APP_ID_ATRIX` | `ICON_ATRIX` (114) | "ATRIX Browser" | `atrix_browser_launch` |
| 9 | **Task Manager** | 14 | `APP_ID_TMH` | `ICON_TMH` (116) | "Task Manager" | `tmh_app_init` |
| 10 | **Control Panel** | 16 | `APP_ID_CONTROLPANEL` | `ICON_SETTINGS` (105) | "Control Panel" | `settings_init_v2` |
| 11 | **DOOM** | 10 | `APP_ID_DOOM` | `ICON_DOOM` (112) | "DOOM" | `doom_launch_wrapper` |

---

### 4. Authoritative Running State Machine

Every application icon in the Taskbar displays one of four distinct states based on real BWE window and process queries:

```
                  ┌───────────────┐
                  │  NOT_RUNNING  │ (No window exists, no running indicator)
                  └───────┬───────┘
                          │ User clicks icon (horse_launch)
                          ▼
                  ┌───────────────┐
                  │    ACTIVE     │ (Window visible + BOS_GetFocus() == win_id)
                  └───────┬───────┘ (Wide 20px accent pill indicator)
                     ▲    │
     Focus gained    │    │ Focus lost / Minimized
                     │    ▼
                  ┌───────────────┐
                  │    RUNNING    │ (Window exists in background / minimized)
                  └───────┬───────┘ (Small 6px accent dot indicator)
                          │
                          │ Window closed / Process exit (BOS_CloseSurfacesByPID)
                          ▼
                  ┌───────────────┐
                  │  NOT_RUNNING  │ (Indicator removed immediately)
                  └───────────────┘
```

#### Visual Indicator Specifications:
- **Active Window**: 20px wide × 3px high accent rounded bar directly under the app icon.
- **Running (Background / Minimized)**: 6px wide × 3px high accent rounded dot directly under the app icon.
- **Not Running**: No indicator.
- **Hover**: 36×36 rounded highlight card (`0x33FFFFFF` or `0x443B82F6`) behind the icon.

---

### 5. Click & Window Interaction Protocol

When an application icon is clicked:

```
                          Icon Clicked
                               │
                ┌──────────────┴──────────────┐
                │ Is App Window Currently     │
                │ Registered in BWE?          │
                └──────────────┬──────────────┘
                               │
               NO              │              YES
        ┌──────────────────────┘       ┌────────────────────────┐
        ▼                              ▼                        ▼
horse_launch(app_id)           Is Window Active?        Is Window Inactive?
                                       │                        │
                                      YES                       │
                                       ▼                        ▼
                               Minimize / Hide          Show, Focus & Bring to
                               (BOS_Hide)               Front (BOS_Show,
                                                        BOS_SetFocus,
                                                        BWE_BringToFront)
```

---

### 6. Start Panel Architecture

- **Window ID**: `g_start_menu_win_id` (BWE Panel Window).
- **Dimensions**: 600 px width × 420 px height.
- **Position**: Centered horizontally, positioned 10 px above the Taskbar.
- **Layout Structure**:
  1. **Header**: Search bar with live placeholder and focus detection.
  2. **Section Title**: "Pinned Applications" (Bofont Bold, `0xFF94A3B8`).
  3. **Applications Grid**: 4 columns × 3 rows of rich application cards (Icon + Typography Label).
  4. **Footer**:
     - User Avatar & Name ("Saumya" / "ATOMS Session").
     - Power Options Button (Opens Sleep, Restart, Power Off flyout).
- **Smooth Toggle**: Clicking Start toggles open/close state. Clicking outside closes the panel immediately.

---

### 7. Right-Side RTC Time & Date Architecture

- **Data Source**: CMOS Hardware RTC via `rtc_read_datetime(&dt)`.
- **Layout**:
  - Top Line: `HH:MM AM/PM` (Bofont Bold, `0xFFFFFFFF`).
  - Bottom Line: `DD MMM` (e.g., `22 Aug`) (Bofont Regular, `0xFF94A3B8`).
- **Update Cadence**: Evaluated every frame, but formatted and rendered via BWE text without full-screen invalidation.

---

### 8. Performance & Safety Verification Plan

1. **Rendering Performance**:
   - The taskbar and start panel draw using direct RAM framebuffer compositing.
   - Clipping is strictly enforced to `self->screen_bounds`.
2. **Memory Safety**:
   - Zero heap allocations (`kmalloc`/`malloc`) inside `task_panel_render_callback` or `start_menu_render_callback`.
3. **Subsystem Isolation**:
   - If an application faults, BWE cleans up its window table slot, and `TaskPanel_Update()` automatically clears its running indicator on the next frame.
