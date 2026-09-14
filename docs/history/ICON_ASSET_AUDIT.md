# ATOMS OS — ICON ASSET AUDIT REPORT

## 1. Executive Summary
This document provides a comprehensive audit and classification of the ATOMS OS icon asset system. All duplicate, multi-resolution (`_16`, `_20`, `_24`), and inconsistent legacy assets have been safely archived into `assets/icons/_legacy/`. The new authoritative icon system resides in `assets/icons/{apps, system, navigation, branding}/` and is managed by `tools/icon_pipeline.py`.

---

## 2. Directory Architecture Comparison

### Legacy Structure (Deprecated)
```
assets/icons/
├── atrix.png
├── calculator.png
├── doom.png
├── explorer.png
├── graph3d.png
├── icon_system_battery_charging.png
├── icon_system_battery_charging_16.png
├── icon_system_battery_charging_20.png
├── icon_system_battery_charging_24.png
├── icon_system_battery_low.png
... (63 total unorganized files)
```

### New Authoritative Structure (Canonical V1.0)
```
assets/icons/
├── apps/
│   ├── atoms.png
│   ├── explorer.png
│   ├── terminal.png
│   ├── settings.png
│   ├── calculator.png
│   ├── notes.png
│   ├── graph3d.png
│   ├── doom.png
│   ├── inputlab.png
│   ├── tmh.png
│   ├── music.png
│   └── atrix.png
│
├── system/
│   ├── battery.png
│   ├── battery_charging.png
│   ├── battery_low.png
│   ├── volume.png
│   ├── volume_low.png
│   ├── volume_muted.png
│   ├── wifi.png
│   ├── wifi_weak.png
│   ├── wifi_disconnected.png
│   ├── notification.png
│   ├── notification_unread.png
│   ├── power.png
│   └── search.png
│
├── navigation/
│   ├── back.png
│   ├── forward.png
│   ├── up.png
│   ├── refresh.png
│   ├── home.png
│   └── folder.png
│
├── branding/
│   └── atoms_start.png
│
├── _legacy/
│   └── (63 archived legacy files preserved safely)
│
└── icon_preview.png  (Master Multi-Scale & Dark/Light Quality Preview Sheet)
```

---

## 3. Legacy Asset Classification & Migration Ledger

| Old Asset | Status | Canonical Replacement | Category | Notes |
| :--- | :---: | :--- | :--- | :--- |
| `atrix.png` | **REPLACED** | `assets/icons/apps/atrix.png` | App | Modernized with ATOMS compass & globe styling |
| `calculator.png` | **REPLACED** | `assets/icons/apps/calculator.png` | App | Minimal slate calculator with cyan operator |
| `doom.png` | **REPLACED** | `assets/icons/apps/doom.png` | App | Stylized retro gaming shield emblem |
| `explorer.png` | **REPLACED** | `assets/icons/apps/explorer.png` | App | Geometric folder with layered slate & cyan |
| `graph3d.png` | **REPLACED** | `assets/icons/apps/graph3d.png` | App | Isometric 3D wireframe / prism benchmark |
| `inputlab.png` | **REPLACED** | `assets/icons/apps/inputlab.png` | App | Precision diagnostic reticle & pointer |
| `music.png` | **REPLACED** | `assets/icons/apps/music.png` | App | BOSpectra harmonic audio wave |
| `settings.png` | **REPLACED** | `assets/icons/apps/settings.png` | App | Technical precision gear & axle |
| `stresstest.png` | **REPLACED** | `assets/icons/apps/tmh.png` | App | Consolidated into Task Manager Heartbeat |
| `terminal.png` | **REPLACED** | `assets/icons/apps/terminal.png` | App | Dark slate terminal with cyan `>_` prompt |
| `tmh.png` | **REPLACED** | `assets/icons/apps/tmh.png` | App | High-precision ECG telemetry pulse |
| `icon_system_battery_charging.png` | **REPLACED** | `assets/icons/system/battery_charging.png` | System | Single master PNG (256x256) |
| `icon_system_battery_charging_16.png` | **ARCHIVED** | Downsampled via pipeline | System | Eliminated resolution duplication |
| `icon_system_battery_charging_20.png` | **ARCHIVED** | Downsampled via pipeline | System | Eliminated resolution duplication |
| `icon_system_battery_charging_24.png` | **ARCHIVED** | Downsampled via pipeline | System | Eliminated resolution duplication |
| `icon_system_battery_low.png` | **REPLACED** | `assets/icons/system/battery_low.png` | System | Single master PNG (256x256) |
| `icon_system_battery_low_16.png` | **ARCHIVED** | Downsampled via pipeline | System | Eliminated resolution duplication |
| `icon_system_battery_low_20.png` | **ARCHIVED** | Downsampled via pipeline | System | Eliminated resolution duplication |
| `icon_system_battery_low_24.png` | **ARCHIVED** | Downsampled via pipeline | System | Eliminated resolution duplication |
| `icon_system_battery_normal.png` | **REPLACED** | `assets/icons/system/battery.png` | System | Single master PNG (256x256) |
| `icon_system_battery_normal_16.png` | **ARCHIVED** | Downsampled via pipeline | System | Eliminated resolution duplication |
| `icon_system_battery_normal_20.png` | **ARCHIVED** | Downsampled via pipeline | System | Eliminated resolution duplication |
| `icon_system_battery_normal_24.png` | **ARCHIVED** | Downsampled via pipeline | System | Eliminated resolution duplication |
| `icon_system_notification_normal*.png` | **ARCHIVED** | `assets/icons/system/notification.png` | System | Consolidated 4 resolution files into 1 master |
| `icon_system_notification_unread*.png` | **ARCHIVED** | `assets/icons/system/notification_unread.png` | System | Consolidated 4 resolution files into 1 master |
| `icon_system_volume_low*.png` | **ARCHIVED** | `assets/icons/system/volume_low.png` | System | Consolidated 4 resolution files into 1 master |
| `icon_system_volume_muted*.png` | **ARCHIVED** | `assets/icons/system/volume_muted.png` | System | Consolidated 4 resolution files into 1 master |
| `icon_system_volume_normal*.png` | **ARCHIVED** | `assets/icons/system/volume.png` | System | Consolidated 4 resolution files into 1 master |
| `icon_system_wifi_connected*.png` | **ARCHIVED** | `assets/icons/system/wifi.png` | System | Consolidated 4 resolution files into 1 master |
| `icon_system_wifi_disconnected*.png` | **ARCHIVED** | `assets/icons/system/wifi_disconnected.png` | System | Consolidated 4 resolution files into 1 master |
| `icon_system_wifi_weak*.png` | **ARCHIVED** | `assets/icons/system/wifi_weak.png` | System | Consolidated 4 resolution files into 1 master |
| `sys_bat_low.png`, `sys_bat_norm.png` | **ARCHIVED** | `assets/icons/system/battery*.png` | System | Obsolete naming scheme |
| `sys_bell_dot.png`, `sys_bell_norm.png`| **ARCHIVED** | `assets/icons/system/notification*.png` | System | Obsolete naming scheme |
| `sys_vol_mute.png`, `sys_vol_norm.png` | **ARCHIVED** | `assets/icons/system/volume*.png` | System | Obsolete naming scheme |
| `sys_wifi.png`, `sys_wifi_off.png` | **ARCHIVED** | `assets/icons/system/wifi*.png` | System | Obsolete naming scheme |

---

## 4. Code & Build System References Updated
1. **`tools/icon_pipeline.py`**:
   - Master Python pipeline generating 256x256 RGBA assets with 4x supersampled vector geometry.
   - Synchronizes `kernel/ui/boasset/sys_icons_data_v11.h` and `sys_icons_data.h` automatically.
   - Exports multi-scale verification preview to `assets/icons/icon_preview.png`.
2. **`kernel/ui/boasset/`**:
   - Retains 100% ABI compatibility with `ICON_EXPLORER`, `ICON_TERMINAL`, `ICON_SETTINGS`, etc.
   - `sys_icons_data_v11.h` updated with refreshed 16x16 native arrays.
3. **`tools/image_builder.c`**:
   - VFS root disk packaging verified for FAT32 8.3 filenames (`EXPLORER.PNG`, `TERMINAL.PNG`, `SETTINGSPNG`, etc.).
