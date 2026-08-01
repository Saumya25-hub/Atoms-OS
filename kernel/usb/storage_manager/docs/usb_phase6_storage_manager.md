# SIGNATURES OS — USB PHASE 6: USB STORAGE DEVICE MANAGER & VFS INTEGRATION SPECIFICATION

## Overview
The **USB Storage Device Manager & VFS Integration (USM)** connects low-level USB block devices to the Signatures OS file system and kernel VFS layers. It manages disk registration, MBR/GPT partition parsing, volume letter assignment (`U:`, `V:`, `W:`), drive mounting/unmounting, LRU sector caching, elevator I/O scheduling, hot-plug auto-mount, safe eject, and structured JSON telemetry.

---

## 🏛️ Subsystem Architecture

```
                               ┌─────────────────────────────┐
                               │     Signatures OS VFS       │
                               └──────────────┬──────────────┘
                                              │
                               ┌──────────────▼──────────────┐
                               │      VFS Bridge Layer       │
                               └──────────────┬──────────────┘
                                              │
                 ┌────────────────────────────┴────────────────────────────┐
                 │                                                         │
      ┌──────────▼──────────┐                                   ┌──────────▼──────────┐
      │   Mount Manager     │                                   │ LRU Sector Cache    │
      │   Drive Letter (U:) │                                   │ Elevator Scheduler  │
      └──────────┬──────────┘                                   └─────────────────────┘
                 │
      ┌──────────▼──────────┐
      │ Partition Scanner   │
      │   MBR / GPT Parser  │
      └──────────┬──────────┘
                 │
      ┌──────────▼──────────┐
      │ Logical Disk Mgr    │
      └─────────────────────┘
```

---

## 📊 Telemetry & JSON Schema

```json
{
  "usm_subsystem_report": {
    "total_disks": 1,
    "total_partitions": 1,
    "total_volumes": 1,
    "mounted_drives": ["U:"],
    "cache_hit_ratio": 0.984,
    "status": "PRODUCTION_READY"
  }
}
```
